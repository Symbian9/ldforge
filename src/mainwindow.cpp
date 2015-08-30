/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Teemu Piippo
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QGridLayout>
#include <QMessageBox>
#include <QEvent>
#include <QContextMenuEvent>
#include <QMenuBar>
#include <QStatusBar>
#include <QSplitter>
#include <QListWidget>
#include <QToolButton>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QToolBar>
#include <QProgressBar>
#include <QLabel>
#include <QFileDialog>
#include <QPushButton>
#include <QCoreApplication>
#include <QTimer>
#include <QMetaMethod>
#include <QSettings>
#include "main.h"
#include "glRenderer.h"
#include "mainwindow.h"
#include "ldDocument.h"
#include "configuration.h"
#include "miscallenous.h"
#include "colors.h"
#include "editHistory.h"
#include "radioGroup.h"
#include "addObjectDialog.h"
#include "messageLog.h"
#include "configuration.h"
#include "ui_mainwindow.h"
#include "primitives.h"
#include "editmodes/abstractEditMode.h"

static bool g_isSelectionLocked = false;
static QMap<QAction*, QKeySequence> g_defaultShortcuts;

CFGENTRY (Bool, ColorizeObjectsList, true)
CFGENTRY (String, QuickColorToolbar, "4:25:14:27:2:3:11:1:22:|:0:72:71:15")
CFGENTRY (Bool, ListImplicitFiles, false)
CFGENTRY (List, HiddenToolbars, {})
EXTERN_CFGENTRY (List, RecentFiles)
EXTERN_CFGENTRY (Bool, DrawAxes)
EXTERN_CFGENTRY (String, MainColor)
EXTERN_CFGENTRY (Float, MainColorAlpha)
EXTERN_CFGENTRY (Bool, DrawWireframe)
EXTERN_CFGENTRY (Bool, BFCRedGreenView)
EXTERN_CFGENTRY (Bool, DrawAngles)
EXTERN_CFGENTRY (Bool, RandomColors)
EXTERN_CFGENTRY (Bool, DrawSurfaces)
EXTERN_CFGENTRY (Bool, DrawEdgeLines)
EXTERN_CFGENTRY (Bool, DrawConditionalLines)

// =============================================================================
//
MainWindow::MainWindow (QWidget* parent, Qt::WindowFlags flags) :
	QMainWindow (parent, flags),
	ui (*new Ui_MainWindow)
{
	g_win = this;
	ui.setupUi (this);
	m_updatingTabs = false;
	m_renderer = new GLRenderer (this);
	m_tabs = new QTabBar;
	m_tabs->setTabsClosable (true);
	ui.verticalLayout->insertWidget (0, m_tabs);

	// Stuff the renderer into its frame
	QVBoxLayout* rendererLayout = new QVBoxLayout (ui.rendererFrame);
	rendererLayout->addWidget (R());

	connect (ui.objectList, SIGNAL (itemSelectionChanged()), this, SLOT (slot_selectionChanged()));
	connect (ui.objectList, SIGNAL (itemDoubleClicked (QListWidgetItem*)), this, SLOT (slot_editObject (QListWidgetItem*)));
	connect (m_tabs, SIGNAL (currentChanged(int)), this, SLOT (changeCurrentFile()));
	connect (m_tabs, SIGNAL (tabCloseRequested (int)), this, SLOT (closeTab (int)));

	if (ActivePrimitiveScanner() != null)
		connect (ActivePrimitiveScanner(), SIGNAL (workDone()), this, SLOT (updatePrimitives()));
	else
		updatePrimitives();

	m_msglog = new MessageManager;
	m_msglog->setRenderer (R());
	m_renderer->setMessageLog (m_msglog);
	m_quickColors = LoadQuickColorList();
	slot_selectionChanged();
	setStatusBar (new QStatusBar);
	updateActions();

	// Connect all actions and save default sequences
	applyToActions ([&](QAction* act)
	{
		connect (act, SIGNAL (triggered()), this, SLOT (slot_action()));
		g_defaultShortcuts[act] = act->shortcut();
	});

	updateGridToolBar();
	updateEditModeActions();
	updateRecentFilesMenu();
	updateColorToolbar();
	updateTitle();
	loadShortcuts (Config::SettingsObject());
	setMinimumSize (300, 200);
	connect (qApp, SIGNAL (aboutToQuit()), this, SLOT (slot_lastSecondCleanup()));
	connect (ui.ringToolHiRes, SIGNAL (clicked (bool)), this, SLOT (ringToolHiResClicked (bool)));
	connect (ui.ringToolSegments, SIGNAL (valueChanged (int)),
		this, SLOT (circleToolSegmentsChanged()));
	circleToolSegmentsChanged(); // invoke it manually for initial label text

	for (QVariant const& toolbarname : cfg::HiddenToolbars)
	{
		QToolBar* toolbar = findChild<QToolBar*> (toolbarname.toString());

		if (toolbar != null)
			toolbar->hide();
	}
}

MainWindow::~MainWindow()
{
	g_win = null;
}

// =============================================================================
//
void MainWindow::slot_action()
{
	// Get the name of the sender object and use it to compose the slot name,
	// then invoke this slot to call the action.
	QMetaObject::invokeMethod (this,
		qPrintable (format ("slot_%1", sender()->objectName())), Qt::DirectConnection);
	endAction();
}

// =============================================================================
//
void MainWindow::endAction()
{
	// Add a step in the history now.
	CurrentDocument()->addHistoryStep();

	// Update the list item of the current file - we may need to draw an icon
	// now that marks it as having unsaved changes.
	updateDocumentListItem (CurrentDocument());
	refresh();
}

// =============================================================================
//
void MainWindow::slot_lastSecondCleanup()
{
	delete m_renderer;
	delete &ui;
}

// =============================================================================
//
void MainWindow::updateRecentFilesMenu()
{
	// First, clear any items in the recent files menu
for (QAction * recent : m_recentFiles)
		delete recent;

	m_recentFiles.clear();

	QAction* first = null;

	for (const QVariant& it : cfg::RecentFiles)
	{
		QString file = it.toString();
		QAction* recent = new QAction (GetIcon ("open-recent"), file, this);

		connect (recent, SIGNAL (triggered()), this, SLOT (slot_recentFile()));
		ui.menuOpenRecent->insertAction (first, recent);
		m_recentFiles << recent;
		first = recent;
	}
}

// =============================================================================
//
QList<LDQuickColor> LoadQuickColorList()
{
	QList<LDQuickColor> colors;

	for (QString colorname : cfg::QuickColorToolbar.split (":"))
	{
		if (colorname == "|")
			colors << LDQuickColor::getSeparator();
		else
		{
			LDColor color = colorname.toInt();

			if (color.isValid())
				colors << LDQuickColor (color, null);
		}
	}

	return colors;
}

// =============================================================================
//
void MainWindow::updateColorToolbar()
{
	m_colorButtons.clear();
	ui.toolBarColors->clear();
	ui.toolBarColors->addAction (ui.actionUncolor);
	ui.toolBarColors->addSeparator();

	for (LDQuickColor& entry : m_quickColors)
	{
		if (entry.isSeparator())
		{
			ui.toolBarColors->addSeparator();
		}
		else
		{
			QToolButton* colorButton = new QToolButton;
			colorButton->setIcon (MakeColorIcon (entry.color(), 16));
			colorButton->setIconSize (QSize (16, 16));
			colorButton->setToolTip (entry.color().name());

			connect (colorButton, SIGNAL (clicked()), this, SLOT (slot_quickColor()));
			ui.toolBarColors->addWidget (colorButton);
			m_colorButtons << colorButton;

			entry.setToolButton (colorButton);
		}
	}

	updateGridToolBar();
}

// =============================================================================
//
void MainWindow::updateGridToolBar()
{
	// Ensure that the current grid - and only the current grid - is selected.
	ui.actionGridCoarse->setChecked (cfg::Grid == Grid::Coarse);
	ui.actionGridMedium->setChecked (cfg::Grid == Grid::Medium);
	ui.actionGridFine->setChecked (cfg::Grid == Grid::Fine);
}

// =============================================================================
//
void MainWindow::updateTitle()
{
	QString title = format (APPNAME " " VERSION_STRING);

	// Append our current file if we have one
	if (CurrentDocument())
	{
		title += ": ";
		title += CurrentDocument()->getDisplayName();

		if (CurrentDocument()->getObjectCount() > 0 and
			CurrentDocument()->getObject (0)->type() == OBJ_Comment)
		{
			// Append title
			LDComment* comm = static_cast <LDComment*> (CurrentDocument()->getObject (0));
			title += format (": %1", comm->text());
		}

		if (CurrentDocument()->hasUnsavedChanges())
			title += '*';
	}

#ifdef DEBUG
	title += " [debug build]";
#elif BUILD_ID != BUILD_RELEASE
	title += " [pre-release build]";
#endif // DEBUG

	if (strlen (commitTimeString()))
		title += format (" (%1)", QString::fromUtf8 (commitTimeString()));

	setWindowTitle (title);
}

// =============================================================================
//
int MainWindow::deleteSelection()
{
	if (Selection().isEmpty())
		return 0;

	LDObjectList selCopy = Selection();

	// Delete the objects that were being selected
	for (LDObject* obj : selCopy)
		obj->destroy();

	refresh();
	return selCopy.size();
}

// =============================================================================
//
void MainWindow::buildObjList()
{
	if (not CurrentDocument())
		return;

	// Lock the selection while we do this so that refreshing the object list
	// doesn't trigger selection updating so that the selection doesn't get lost
	// while this is done.
	g_isSelectionLocked = true;

	for (int i = 0; i < ui.objectList->count(); ++i)
		delete ui.objectList->item (i);

	ui.objectList->clear();

	for (LDObject* obj : CurrentDocument()->objects())
	{
		QString descr;

		switch (obj->type())
		{
			case OBJ_Comment:
			{
				descr = static_cast<LDComment*> (obj)->text();

				// Remove leading whitespace
				while (descr[0] == ' ')
					descr.remove (0, 1);

				break;
			}

			case OBJ_Empty:
				break; // leave it empty

			case OBJ_Line:
			case OBJ_Triangle:
			case OBJ_Quad:
			case OBJ_CondLine:
			{
				for (int i = 0; i < obj->numVertices(); ++i)
				{
					if (i != 0)
						descr += ", ";

					descr += obj->vertex (i).toString (true);
				}
				break;
			}

			case OBJ_Error:
			{
				descr = format ("ERROR: %1", obj->asText());
				break;
			}

			case OBJ_Subfile:
			{
				LDSubfile* ref = static_cast<LDSubfile*> (obj);

				descr = format ("%1 %2, (", ref->fileInfo()->getDisplayName(), ref->position().toString (true));

				for (int i = 0; i < 9; ++i)
					descr += format ("%1%2", ref->transform()[i], (i != 8) ? " " : "");

				descr += ')';
				break;
			}

			case OBJ_BFC:
			{
				descr = LDBFC::StatementStrings[int (static_cast<LDBFC*> (obj)->statement())];
				break;
			}

			case OBJ_Overlay:
			{
				LDOverlay* ovl = static_cast<LDOverlay*> (obj);
				descr = format ("[%1] %2 (%3, %4), %5 x %6", g_CameraNames[ovl->camera()],
					Basename (ovl->fileName()), ovl->x(), ovl->y(),
					ovl->width(), ovl->height());
				break;
			}

			default:
			{
				descr = obj->typeName();
				break;
			}
		}

		QListWidgetItem* item = new QListWidgetItem (descr);
		item->setIcon (GetIcon (obj->typeName()));

		// Use italic font if hidden
		if (obj->isHidden())
		{
			QFont font = item->font();
			font.setItalic (true);
			item->setFont (font);
		}

		// Color gibberish orange on red so it stands out.
		if (obj->type() == OBJ_Error)
		{
			item->setBackground (QColor ("#AA0000"));
			item->setForeground (QColor ("#FFAA00"));
		}
		else if (cfg::ColorizeObjectsList and obj->isColored() and
			obj->color().isValid() and obj->color() != MainColor and obj->color() != EdgeColor)
		{
			// If the object isn't in the main or edge color, draw this list entry in that color.
			item->setForeground (obj->color().faceColor());
		}

		obj->qObjListEntry = item;
		ui.objectList->insertItem (ui.objectList->count(), item);
	}

	g_isSelectionLocked = false;
	updateSelection();
	scrollToSelection();
}

// =============================================================================
//
void MainWindow::scrollToSelection()
{
	if (Selection().isEmpty())
		return;

	LDObject* obj = Selection().last();
	ui.objectList->scrollToItem (obj->qObjListEntry);
}

// =============================================================================
//
void MainWindow::slot_selectionChanged()
{
	if (g_isSelectionLocked == true or CurrentDocument() == null)
		return;

	LDObjectList priorSelection = Selection();

	// Get the objects from the object list selection
	CurrentDocument()->clearSelection();
	const QList<QListWidgetItem*> items = ui.objectList->selectedItems();

	for (LDObject* obj : CurrentDocument()->objects())
	{
		for (QListWidgetItem* item : items)
		{
			if (item == obj->qObjListEntry)
			{
				obj->select();
				break;
			}
		}
	}

	// The select() method calls may have selected additional items (i.e. invertnexts)
	// Update it all now.
	updateSelection();

	// Update the GL renderer
	LDObjectList compound = priorSelection + Selection();
	RemoveDuplicates (compound);

	for (LDObject* obj : compound)
		R()->compileObject (obj);

	R()->update();
}

// =============================================================================
//
void MainWindow::slot_recentFile()
{
	QAction* qAct = static_cast<QAction*> (sender());
	OpenMainModel (qAct->text());
}

// =============================================================================
//
void MainWindow::slot_quickColor()
{
	QToolButton* button = static_cast<QToolButton*> (sender());
	LDColor color = LDColor::nullColor();

	for (const LDQuickColor& entry : m_quickColors)
	{
		if (entry.toolButton() == button)
		{
			color = entry.color();
			break;
		}
	}

	if (not color.isValid())
		return;

	for (LDObject* obj : Selection())
	{
		if (not obj->isColored())
			continue; // uncolored object

		obj->setColor (color);
		R()->compileObject (obj);
	}

	endAction();
	refresh();
}

// =============================================================================
//
int MainWindow::getInsertionPoint()
{
	// If we have a selection, put the item after it.
	if (not Selection().isEmpty())
		return Selection().last()->lineNumber() + 1;

	// Otherwise place the object at the end.
	return CurrentDocument()->getObjectCount();
}

// =============================================================================
//
void MainWindow::doFullRefresh()
{
	buildObjList();
	m_renderer->hardRefresh();
}

// =============================================================================
//
void MainWindow::refresh()
{
	buildObjList();
	m_renderer->update();
}

// =============================================================================
//
void MainWindow::updateSelection()
{
	g_isSelectionLocked = true;
	QItemSelection itemselect;
	int top = -1;
	int bottom = -1;

	for (LDObject* obj : Selection())
	{
		if (obj->qObjListEntry == null)
			continue;

		int row = ui.objectList->row (obj->qObjListEntry);

		if (top == -1)
		{
			top = bottom = row;
		}
		else
		{
			if (row != bottom + 1)
			{
				itemselect.select (ui.objectList->model()->index (top, 0),
					ui.objectList->model()->index (bottom, 0));
				top = -1;
			}

			bottom = row;
		}
	}

	if (top != -1)
	{
		itemselect.select (ui.objectList->model()->index (top, 0),
			ui.objectList->model()->index (bottom, 0));
	}

	ui.objectList->selectionModel()->select (itemselect, QItemSelectionModel::ClearAndSelect);
	g_isSelectionLocked = false;
}

// =============================================================================
//
LDColor MainWindow::getSelectedColor()
{
	LDColor result;

	for (LDObject* obj : Selection())
	{
		if (not obj->isColored())
			continue; // This one doesn't use color so it doesn't have a say

		if (result.isValid() and obj->color() != result)
			return LDColor::nullColor(); // No consensus in object color

		if (not result.isValid())
			result = obj->color();
	}

	return result;
}

// =============================================================================
//
void MainWindow::closeEvent (QCloseEvent* ev)
{
	// Check whether it's safe to close all files.
	if (not IsSafeToCloseAll())
	{
		ev->ignore();
		return;
	}

	// Save the toolbar set
	cfg::HiddenToolbars.clear();

	for (QToolBar* toolbar : findChildren<QToolBar*>())
	{
		if (toolbar->isHidden())
			cfg::HiddenToolbars << toolbar->objectName();
	}

	// Save the configuration before leaving.
	Config::Save();
	ev->accept();
}

// =============================================================================
//
void MainWindow::spawnContextMenu (const QPoint pos)
{
	const bool single = (Selection().size() == 1);
	LDObject* singleObj = single ? Selection().first() : nullptr;

	bool hasSubfiles = false;

	for (LDObject* obj : Selection())
	{
		if (obj->type() == OBJ_Subfile)
		{
			hasSubfiles = true;
			break;
		}
	}

	QMenu* contextMenu = new QMenu;

	if (single and singleObj->type() != OBJ_Empty)
	{
		contextMenu->addAction (ui.actionEdit);
		contextMenu->addSeparator();
	}

	contextMenu->addAction (ui.actionCut);
	contextMenu->addAction (ui.actionCopy);
	contextMenu->addAction (ui.actionPaste);
	contextMenu->addAction (ui.actionDelete);
	contextMenu->addSeparator();
	contextMenu->addAction (ui.actionSetColor);

	if (single)
		contextMenu->addAction (ui.actionEditRaw);

	contextMenu->addAction (ui.actionBorders);
	contextMenu->addAction (ui.actionSetOverlay);
	contextMenu->addAction (ui.actionClearOverlay);

	if (hasSubfiles)
	{
		contextMenu->addSeparator();
		contextMenu->addAction (ui.actionOpenSubfiles);
	}

	contextMenu->addSeparator();
	contextMenu->addAction (ui.actionModeSelect);
	contextMenu->addAction (ui.actionModeDraw);
	contextMenu->addAction (ui.actionModeCircle);

	if (not Selection().isEmpty())
	{
		contextMenu->addSeparator();
		contextMenu->addAction (ui.actionSubfileSelection);
	}

	if (R()->camera() != EFreeCamera)
	{
		contextMenu->addSeparator();
		contextMenu->addAction (ui.actionSetDrawDepth);
	}

	contextMenu->exec (pos);
}

// =============================================================================
//
void MainWindow::deleteByColor (LDColor color)
{
	LDObjectList objs;

	for (LDObject* obj : CurrentDocument()->objects())
	{
		if (not obj->isColored() or obj->color() != color)
			continue;

		objs << obj;
	}

	for (LDObject* obj : objs)
		obj->destroy();
}

// =============================================================================
//
void MainWindow::updateEditModeActions()
{
	const EditModeType mode = R()->currentEditModeType();
	ui.actionModeSelect->setChecked (mode == EditModeType::Select);
	ui.actionModeDraw->setChecked (mode == EditModeType::Draw);
	ui.actionModeRectangle->setChecked (mode == EditModeType::Rectangle);
	ui.actionModeCircle->setChecked (mode == EditModeType::Circle);
	ui.actionModeMagicWand->setChecked (mode == EditModeType::MagicWand);
	ui.actionModeLinePath->setChecked (mode == EditModeType::LinePath);
}

// =============================================================================
//
void MainWindow::slot_editObject (QListWidgetItem* listitem)
{
	for (LDObject* it : CurrentDocument()->objects())
	{
		if (it->qObjListEntry == listitem)
		{
			AddObjectDialog::staticDialog (it->type(), it);
			break;
		}
	}
}

// =============================================================================
//
bool MainWindow::save (LDDocument* doc, bool saveAs)
{
	QString path = doc->fullPath();
	int64 savesize;

	if (saveAs or path.isEmpty())
	{
		QString name = doc->defaultName();

		if (not doc->fullPath().isEmpty()) 
			name = doc->fullPath();
		elif (not doc->name().isEmpty())
			name = doc->name();

		name.replace ("\\", "/");
		path = QFileDialog::getSaveFileName (g_win, tr ("Save As"),
			name, tr ("LDraw files (*.dat *.ldr)"));

		if (path.isEmpty())
		{
			// User didn't give a file name, abort.
			return false;
		}
	}

	if (doc->save (path, &savesize))
	{
		if (doc == CurrentDocument())
			updateTitle();

		print ("Saved to %1 (%2)", path, MakePrettyFileSize (savesize));

		// Add it to recent files
		AddRecentFile (path);
		return true;
	}

	QString message = format (tr ("Failed to save to %1: %2"), path, strerror (errno));

	// Tell the user the save failed, and give the option for saving as with it.
	QMessageBox dlg (QMessageBox::Critical, tr ("Save Failure"), message, QMessageBox::Close, g_win);

	// Add a save-as button
	QPushButton* saveAsBtn = new QPushButton (tr ("Save As"));
	saveAsBtn->setIcon (GetIcon ("file-save-as"));
	dlg.addButton (saveAsBtn, QMessageBox::ActionRole);
	dlg.setDefaultButton (QMessageBox::Close);
	dlg.exec();

	if (dlg.clickedButton() == saveAsBtn)
		return save (doc, true); // yay recursion!

	return false;
}

void MainWindow::addMessage (QString msg)
{
	m_msglog->addLine (msg);
}

// ============================================================================
void ObjectList::contextMenuEvent (QContextMenuEvent* ev)
{
	g_win->spawnContextMenu (ev->globalPos());
}

// =============================================================================
//
QPixmap GetIcon (QString iconName)
{
	return (QPixmap (format (":/icons/%1.png", iconName)));
}

// =============================================================================
//
bool Confirm (const QString& message)
{
	return Confirm (MainWindow::tr ("Confirm"), message);
}

// =============================================================================
//
bool Confirm (const QString& title, const QString& message)
{
	return QMessageBox::question (g_win, title, message,
		(QMessageBox::Yes | QMessageBox::No), QMessageBox::No) == QMessageBox::Yes;
}

// =============================================================================
//
void Critical (const QString& message)
{
	QMessageBox::critical (g_win, MainWindow::tr ("Error"), message,
		(QMessageBox::Close), QMessageBox::Close);
}

// =============================================================================
//
QIcon MakeColorIcon (LDColor colinfo, const int size)
{
	// Create an image object and link a painter to it.
	QImage img (size, size, QImage::Format_ARGB32);
	QPainter paint (&img);
	QColor col = colinfo.faceColor();

	if (colinfo == MainColor)
	{
		// Use the user preferences for main color here
		col = cfg::MainColor;
		col.setAlphaF (cfg::MainColorAlpha);
	}

	// Paint the icon border
	paint.fillRect (QRect (0, 0, size, size), colinfo.edgeColor());

	// Paint the checkerboard background, visible with translucent icons
	paint.drawPixmap (QRect (1, 1, size - 2, size - 2), GetIcon ("checkerboard"), QRect (0, 0, 8, 8));

	// Paint the color above the checkerboard
	paint.fillRect (QRect (1, 1, size - 2, size - 2), col);
	return QIcon (QPixmap::fromImage (img));
}

// =============================================================================
//
void MakeColorComboBox (QComboBox* box)
{
	std::map<LDColor, int> counts;

	for (LDObject* obj : CurrentDocument()->objects())
	{
		if (not obj->isColored() or not obj->color().isValid())
			continue;

		if (counts.find (obj->color()) == counts.end())
			counts[obj->color()] = 1;
		else
			counts[obj->color()]++;
	}

	box->clear();
	int row = 0;

	for (const auto& pair : counts)
	{
		QIcon ico = MakeColorIcon (pair.first, 16);
		box->addItem (ico, format ("[%1] %2 (%3 object%4)",
			pair.first, pair.first.name(), pair.second, Plural (pair.second)));
		box->setItemData (row, pair.first.index());

		++row;
	}
}

// =============================================================================
//
void MainWindow::updateDocumentList()
{
	m_updatingTabs = true;

	while (m_tabs->count() > 0)
		m_tabs->removeTab (0);

	for (LDDocument* f : LDDocument::explicitDocuments())
	{
		// Add an item to the list for this file and store the tab index
		// in the document so we can find documents by tab index.
		f->setTabIndex (m_tabs->addTab (""));
		updateDocumentListItem (f);
	}

	m_updatingTabs = false;
}

// =============================================================================
//
void MainWindow::updateDocumentListItem (LDDocument* doc)
{
	bool oldUpdatingTabs = m_updatingTabs;
	m_updatingTabs = true;

	if (doc->tabIndex() == -1)
	{
		// We don't have a list item for this file, so the list either doesn't
		// exist yet or is out of date. Build the list now.
		updateDocumentList();
		return;
	}

	// If this is the current file, it also needs to be the selected item on
	// the list.
	if (doc == CurrentDocument())
		m_tabs->setCurrentIndex (doc->tabIndex());

	m_tabs->setTabText (doc->tabIndex(), doc->getDisplayName());

	// If the document.has unsaved changes, draw a little icon next to it to mark that.
	m_tabs->setTabIcon (doc->tabIndex(), doc->hasUnsavedChanges() ? GetIcon ("file-save") : QIcon());
	m_tabs->setTabData (doc->tabIndex(), doc->name());
	m_updatingTabs = oldUpdatingTabs;
}

// =============================================================================
//
// A file is selected from the list of files on the left of the screen. Find out
// which file was picked and change to it.
//
void MainWindow::changeCurrentFile()
{
	if (m_updatingTabs)
		return;

	LDDocument* file = nullptr;
	int tabIndex = m_tabs->currentIndex();

	// Find the file pointer of the item that was selected.
	for (LDDocument* it : LDDocument::explicitDocuments())
	{
		if (it->tabIndex() == tabIndex)
		{
			file = it;
			break;
		}
	}

	// If we picked the same file we're currently on, we don't need to do
	// anything.
	if (file == null or file == CurrentDocument())
		return;

	LDDocument::setCurrent (file);
}

// =============================================================================
//
void MainWindow::refreshObjectList()
{
#if 0
	ui.objectList->clear();
	LDDocument* f = getCurrentDocument();

for (LDObject* obj : *f)
		ui.objectList->addItem (obj->qObjListEntry);

#endif

	buildObjList();
}

// =============================================================================
//
void MainWindow::updateActions()
{
	if (CurrentDocument() != null and CurrentDocument()->history() != null)
	{
		History* his = CurrentDocument()->history();
		int pos = his->position();
		ui.actionUndo->setEnabled (pos != -1);
		ui.actionRedo->setEnabled (pos < (long) his->getSize() - 1);
	}

	ui.actionWireframe->setChecked (cfg::DrawWireframe);
	ui.actionAxes->setChecked (cfg::DrawAxes);
	ui.actionBFCView->setChecked (cfg::BFCRedGreenView);
	ui.actionRandomColors->setChecked (cfg::RandomColors);
	ui.actionDrawAngles->setChecked (cfg::DrawAngles);
	ui.actionDrawSurfaces->setChecked (cfg::DrawSurfaces);
	ui.actionDrawEdgeLines->setChecked (cfg::DrawEdgeLines);
	ui.actionDrawConditionalLines->setChecked (cfg::DrawConditionalLines);
}

// =============================================================================
//
void MainWindow::updatePrimitives()
{
	PopulatePrimitives (ui.primitives);
}

// =============================================================================
//
void MainWindow::closeTab (int tabindex)
{
	LDDocument* doc = FindDocument (m_tabs->tabData (tabindex).toString());

	if (doc == null)
		return;

	doc->dismiss();
}

// =============================================================================
//
void MainWindow::loadShortcuts (QSettings const* settings)
{
	for (QAction* act : findChildren<QAction*>())
	{
		QKeySequence seq = settings->value ("shortcut_" + act->objectName(), act->shortcut()).value<QKeySequence>();
		act->setShortcut (seq);
	}
}

// =============================================================================
//
void MainWindow::saveShortcuts (QSettings* settings)
{
	applyToActions ([&](QAction* act)
	{
		QString const key = "shortcut_" + act->objectName();

		if (g_defaultShortcuts[act] != act->shortcut())
			settings->setValue (key, act->shortcut());
		else
			settings->remove (key);
	});
}

// =============================================================================
//
void MainWindow::applyToActions (std::function<void(QAction*)> function)
{
	for (QAction* act : findChildren<QAction*>())
	{
		if (not act->objectName().isEmpty())
			function (act);
	}
}

// =============================================================================
//
QTreeWidget* MainWindow::getPrimitivesTree() const
{
	return ui.primitives;
}

// =============================================================================
//
QKeySequence MainWindow::defaultShortcut (QAction* act) // [static]
{
	return g_defaultShortcuts[act];
}

// =============================================================================
//
bool MainWindow::ringToolHiRes() const
{
	return ui.ringToolHiRes->isChecked();
}

// =============================================================================
//
int MainWindow::ringToolSegments() const
{
	return ui.ringToolSegments->value();
}

// =============================================================================
//
void MainWindow::ringToolHiResClicked (bool checked)
{
	if (checked)
	{
		ui.ringToolSegments->setMaximum (HighResolution);
		ui.ringToolSegments->setValue (ui.ringToolSegments->value() * 3);
	}
	else
	{
		ui.ringToolSegments->setValue (ui.ringToolSegments->value() / 3);
		ui.ringToolSegments->setMaximum (LowResolution);
	}
}

// =============================================================================
//
void MainWindow::circleToolSegmentsChanged()
{
	int numerator (ui.ringToolSegments->value());
	int denominator (ui.ringToolHiRes->isChecked() ? HighResolution : LowResolution);
	Simplify (numerator, denominator);
	ui.ringToolSegmentsLabel->setText (format ("%1 / %2", numerator, denominator));
}

// =============================================================================
//
QImage GetImageFromScreencap (uchar* data, int w, int h)
{
	// GL and Qt formats have R and B swapped. Also, GL flips Y - correct it as well.
	return QImage (data, w, h, QImage::Format_ARGB32).rgbSwapped().mirrored();
}

// =============================================================================
//
LDQuickColor::LDQuickColor (LDColor color, QToolButton* toolButton) :
	m_color (color),
	m_toolButton (toolButton) {}

// =============================================================================
//
LDQuickColor LDQuickColor::getSeparator()
{
	return LDQuickColor (LDColor::nullColor(), null);
}

// =============================================================================
//
bool LDQuickColor::isSeparator() const
{
	return color() == LDColor::nullColor();
}

void PopulatePrimitives (QTreeWidget* tw, QString const& selectByDefault)
{
	tw->clear();

	for (PrimitiveCategory* cat : g_PrimitiveCategories)
	{
		SubfileListItem* parentItem = new SubfileListItem (tw, null);
		parentItem->setText (0, cat->name());
		QList<QTreeWidgetItem*> subfileItems;

		for (Primitive& prim : cat->prims)
		{
			SubfileListItem* item = new SubfileListItem (parentItem, &prim);
			item->setText (0, format ("%1 - %2", prim.name, prim.title));
			subfileItems << item;

			// If this primitive is the one the current object points to,
			// select it by default
			if (selectByDefault == prim.name)
				tw->setCurrentItem (item);
		}

		tw->addTopLevelItem (parentItem);
	}
}
