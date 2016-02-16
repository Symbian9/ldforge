/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2016 Teemu Piippo
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
#include "miscallenous.h"
#include "colors.h"
#include "editHistory.h"
#include "radioGroup.h"
#include "addObjectDialog.h"
#include "messageLog.h"
#include "ui_mainwindow.h"
#include "primitives.h"
#include "editmodes/abstractEditMode.h"
#include "toolsets/extprogramtoolset.h"
#include "toolsets/toolset.h"
#include "dialogs/configdialog.h"
#include "guiutilities.h"
#include "glCompiler.h"
#include "documentmanager.h"
#include "ldobjectiterator.h"
#include "grid.h"
#include "ldObjectMath.h"

ConfigOption (bool ColorizeObjectsList = true)
ConfigOption (QString QuickColorToolbar = "4:25:14:27:2:3:11:1:22:|:0:72:71:15")
ConfigOption (bool ListImplicitFiles = false)
ConfigOption (QStringList HiddenToolbars)

// ---------------------------------------------------------------------------------------------------------------------
//
MainWindow::MainWindow(class Configuration& config, QWidget* parent, Qt::WindowFlags flags) :
	QMainWindow (parent, flags),
	m_config(config),
	m_guiUtilities (new GuiUtilities (this)),
	m_primitives(new PrimitiveManager(this)),
	m_grid(new Grid(this)),
	m_mathFunctions(new MathFunctions(this)),
	ui (*new Ui_MainWindow),
	m_externalPrograms (nullptr),
	m_settings (makeSettings (this)),
	m_documents (new DocumentManager (this)),
	m_currentDocument (nullptr),
	m_isSelectionLocked (false)
{
	g_win = this;
	ui.setupUi (this);
	m_updatingTabs = false;
	m_renderer = new GLRenderer (this);
	m_tabs = new QTabBar;
	m_tabs->setTabsClosable (true);
	ui.verticalLayout->insertWidget (0, m_tabs);
	createBlankDocument();
	m_renderer->setDocument (m_currentDocument);

	// Stuff the renderer into its frame
	QVBoxLayout* rendererLayout = new QVBoxLayout (ui.rendererFrame);
	rendererLayout->addWidget (renderer());

	connect (ui.objectList, SIGNAL (itemSelectionChanged()), this, SLOT (selectionChanged()));
	connect (ui.objectList, SIGNAL (itemDoubleClicked (QListWidgetItem*)), this,
			 SLOT (objectListDoubleClicked (QListWidgetItem*)));
	connect (m_tabs, SIGNAL (currentChanged(int)), this, SLOT (tabSelected()));
	connect (m_tabs, SIGNAL (tabCloseRequested (int)), this, SLOT (closeTab (int)));

	if (m_primitives->activeScanner())
		connect (m_primitives->activeScanner(), SIGNAL (workDone()), this, SLOT (updatePrimitives()));
	else
		updatePrimitives();

	m_quickColors = m_guiUtilities->loadQuickColorList();
	setStatusBar (new QStatusBar);
	updateActions();

	// Connect all actions and save default sequences
	applyToActions ([&](QAction* act)
	{
		connect (act, SIGNAL (triggered()), this, SLOT (actionTriggered()));
		m_defaultShortcuts[act] = act->shortcut();
	});

	updateGridToolBar();
	updateEditModeActions();
	updateRecentFilesMenu();
	updateColorToolbar();
	updateTitle();
	loadShortcuts();
	setMinimumSize (300, 200);
	connect (qApp, SIGNAL (aboutToQuit()), this, SLOT (doLastSecondCleanup()));
	connect (ui.ringToolHiRes, SIGNAL (clicked (bool)), this, SLOT (ringToolHiResClicked (bool)));
	connect (ui.ringToolSegments, SIGNAL (valueChanged (int)), this, SLOT (circleToolSegmentsChanged()));
	circleToolSegmentsChanged(); // invoke it manually for initial label text

	// Examine the toolsets and make a dictionary of tools
	m_toolsets = Toolset::createToolsets (this);

	QStringList ignore;
	for (int i = 0; i < Toolset::staticMetaObject.methodCount(); ++i)
	{
		QMetaMethod method = Toolset::staticMetaObject.method (i);
		ignore.append (QString::fromUtf8 (method.name()));
	}

	for (Toolset* toolset : m_toolsets)
	{
		const QMetaObject* meta = toolset->metaObject();

		if (qobject_cast<ExtProgramToolset*> (toolset))
			m_externalPrograms = static_cast<ExtProgramToolset*> (toolset);

		for (int i = 0; i < meta->methodCount(); ++i)
		{
			ToolInfo info;
			info.method = meta->method (i);
			info.object = toolset;
			QString methodName = QString::fromUtf8 (info.method.name());

			if (ignore.contains (methodName))
				continue; // The method was inherited from base classes

			QString actionName = "action" + methodName.left (1).toUpper() + methodName.mid (1);
			QAction* action = findChild<QAction*> (actionName);

			if (action == nullptr)
				print ("No action for %1::%2 (looked for %3)\n", meta->className(), methodName, actionName);
			else
				m_toolmap[action] = info;
		}
	}

	for (QVariant const& toolbarname : m_config.hiddenToolbars())
	{
		QToolBar* toolbar = findChild<QToolBar*> (toolbarname.toString());

		if (toolbar)
			toolbar->hide();
	}

	// If this is the first start, get the user to configuration. Especially point
	// them to the profile tab, it's the most important form to fill in.
	if (m_config.firstStart())
	{
		ConfigDialog* dialog = new ConfigDialog (this, ConfigDialog::ProfileTab);
		dialog->show();
		m_config.setFirstStart (false);
	}

	QMetaObject::invokeMethod (this, "finishInitialization", Qt::QueuedConnection);
}

void MainWindow::finishInitialization()
{
	m_primitives->loadPrimitives();
}

MainWindow::~MainWindow()
{
	g_win = nullptr;
	delete m_guiUtilities;
	delete m_primitives;
	delete m_grid;
	delete m_mathFunctions;
	delete &ui;
	delete m_settings;
	delete m_documents;

	for (Toolset* toolset : m_toolsets)
		delete toolset;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::actionTriggered()
{
	// Get the name of the sender object and use it to compose the slot name,
	// then invoke this slot to call the action.
	QAction* action = qobject_cast<QAction*> (sender());

	if (action)
	{
		if (m_toolmap.contains (action))
		{
			const ToolInfo& info = m_toolmap[action];
			info.method.invoke (info.object, Qt::DirectConnection);
		}
		else
			print ("No tool info for %1!\n", action->objectName());
	}

	endAction();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::endAction()
{
	m_currentDocument->addHistoryStep();
	updateDocumentListItem (m_currentDocument);
	refresh();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::doLastSecondCleanup()
{
	delete m_renderer;
	delete &ui;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::updateRecentFilesMenu()
{
	// First, clear any items in the recent files menu
	for (QAction* recent : m_recentFiles)
		delete recent;

	m_recentFiles.clear();

	QAction* first = nullptr;

	for (const QVariant& it : m_config.recentFiles())
	{
		QString file = it.toString();
		QAction* recent = new QAction (GetIcon ("open-recent"), file, this);
		connect (recent, SIGNAL (triggered()), this, SLOT (recentFileClicked()));
		ui.menuOpenRecent->insertAction (first, recent);
		m_recentFiles << recent;
		first = recent;
	}
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::updateColorToolbar()
{
	m_colorButtons.clear();
	ui.toolBarColors->clear();
	ui.toolBarColors->addAction (ui.actionUncolor);
	ui.toolBarColors->addSeparator();

	for (ColorToolbarItem& entry : m_quickColors)
	{
		if (entry.isSeparator())
		{
			ui.toolBarColors->addSeparator();
		}
		else
		{
			QToolButton* colorButton = new QToolButton;
			colorButton->setIcon (m_guiUtilities->makeColorIcon (entry.color(), 16));
			colorButton->setIconSize (QSize (16, 16));
			colorButton->setToolTip (entry.color().name());

			connect (colorButton, SIGNAL (clicked()), this, SLOT (quickColorClicked()));
			ui.toolBarColors->addWidget (colorButton);
			m_colorButtons << colorButton;

			entry.setToolButton (colorButton);
		}
	}

	updateGridToolBar();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::updateGridToolBar()
{
	// Ensure that the current grid - and only the current grid - is selected.
	int grid = m_config.grid();
	ui.actionGridCoarse->setChecked (grid == Grid::Coarse);
	ui.actionGridMedium->setChecked (grid == Grid::Medium);
	ui.actionGridFine->setChecked (grid == Grid::Fine);

	// Recompile all Bézier curves, the changing grid affects their precision.
	for (LDObjectIterator<LDBezierCurve> it (m_currentDocument); it.isValid(); ++it)
		renderer()->compileObject (it);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::updateTitle()
{
	QString title = format (APPNAME " " VERSION_STRING);

	// Append our current file if we have one
	if (m_currentDocument)
	{
		title += ": ";
		title += m_currentDocument->getDisplayName();

		if (m_currentDocument->getObjectCount() > 0 and
			m_currentDocument->getObject (0)->type() == OBJ_Comment)
		{
			// Append title
			LDComment* comm = static_cast <LDComment*> (m_currentDocument->getObject (0));
			title += format (": %1", comm->text());
		}

		if (m_currentDocument->hasUnsavedChanges())
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

// ---------------------------------------------------------------------------------------------------------------------
//
int MainWindow::deleteSelection()
{
	if (selectedObjects().isEmpty())
		return 0;

	LDObjectList selCopy = selectedObjects();

	// Delete the objects that were being selected
	for (LDObject* obj : selCopy)
		obj->destroy();

	refresh();
	return selCopy.size();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::buildObjectList()
{
	if (not m_currentDocument)
		return;

	// Lock the selection while we do this so that refreshing the object list
	// doesn't trigger selection updating so that the selection doesn't get lost
	// while this is done.
	m_isSelectionLocked = true;
	m_objectsInList.clear();

	for (int i = 0; i < ui.objectList->count(); ++i)
		delete ui.objectList->item (i);

	ui.objectList->clear();

	for (LDObject* obj : m_currentDocument->objects())
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
			case OBJ_BezierCurve:
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

			case OBJ_SubfileReference:
			{
				LDSubfileReference* ref = static_cast<LDSubfileReference*> (obj);

				descr = format ("%1 %2, (", ref->fileInfo()->getDisplayName(), ref->position().toString (true));

				for (int i = 0; i < 9; ++i)
					descr += format ("%1%2", ref->transform()[i], (i != 8) ? " " : "");

				descr += ')';
				break;
			}

			case OBJ_Bfc:
			{
				descr = static_cast<LDBfc*> (obj)->statementToString();
				break;
			}

			case OBJ_Overlay:
			{
				LDOverlay* ovl = static_cast<LDOverlay*> (obj);
				descr = format ("[%1] %2 (%3, %4), %5 x %6", renderer()->cameraName ((ECamera) ovl->camera()),
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
		else if (m_config.colorizeObjectsList()
			and obj->isColored()
			and obj->color().isValid()
			and obj->color() != MainColor
			and obj->color() != EdgeColor)
		{
			// If the object isn't in the main or edge color, draw this list entry in that color.
			item->setForeground (obj->color().faceColor());
		}

		m_objectsInList.insert (obj, item);
		ui.objectList->insertItem (ui.objectList->count(), item);
	}

	m_isSelectionLocked = false;
	updateSelection();
	scrollToSelection();
}

// ---------------------------------------------------------------------------------------------------------------------
//
// Scrolls the object list so that it points to the first selected object.
//
void MainWindow::scrollToSelection()
{
	if (selectedObjects().isEmpty())
		return;

	LDObject* obj = selectedObjects().first();
	ui.objectList->scrollToItem (m_objectsInList[obj]);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::selectionChanged()
{
	if (m_isSelectionLocked == true or m_currentDocument == nullptr)
		return;

	LDObjectList priorSelection = selectedObjects();

	// Get the objects from the object list selection
	m_currentDocument->clearSelection();
	const QList<QListWidgetItem*> items = ui.objectList->selectedItems();

	for (LDObject* obj : m_currentDocument->objects())
	{
		for (QListWidgetItem* item : items)
		{
			if (item == m_objectsInList[obj])
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
	LDObjectList compound = priorSelection + selectedObjects();
	removeDuplicates (compound);

	for (LDObject* obj : compound)
		renderer()->compileObject (obj);

	renderer()->update();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::recentFileClicked()
{
	QAction* qAct = static_cast<QAction*> (sender());
	documents()->openMainModel (qAct->text());
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::quickColorClicked()
{
	QToolButton* button = static_cast<QToolButton*> (sender());
	LDColor color = LDColor::nullColor();

	for (const ColorToolbarItem& entry : m_quickColors)
	{
		if (entry.toolButton() == button)
		{
			color = entry.color();
			break;
		}
	}

	if (not color.isValid())
		return;

	for (LDObject* obj : selectedObjects())
	{
		if (not obj->isColored())
			continue; // uncolored object

		obj->setColor (color);
		renderer()->compileObject (obj);
	}

	endAction();
	refresh();
}

// ---------------------------------------------------------------------------------------------------------------------
//
// Returns the suggested position to place a new object at.
//
int MainWindow::suggestInsertPoint()
{
	// If we have a selection, put the item after it.
	if (not selectedObjects().isEmpty())
		return selectedObjects().last()->lineNumber() + 1;

	// Otherwise place the object at the end.
	return m_currentDocument->getObjectCount();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::doFullRefresh()
{
	buildObjectList();
	m_renderer->hardRefresh();
}

// ---------------------------------------------------------------------------------------------------------------------
//
// Builds the object list and tells the GL renderer to do a soft update.
//
void MainWindow::refresh()
{
	buildObjectList();
	m_renderer->update();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::updateSelection()
{
	m_isSelectionLocked = true;
	QItemSelection itemselect;
	int top = -1;
	int bottom = -1;

	for (LDObject* obj : selectedObjects())
	{
		QListWidgetItem** itempointer = m_objectsInList.find (obj);

		if (not itempointer)
			continue;

		int row = ui.objectList->row (*itempointer);

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

	// Select multiple objects at once for performance reasons
	ui.objectList->selectionModel()->select (itemselect, QItemSelectionModel::ClearAndSelect);
	m_isSelectionLocked = false;
}

// ---------------------------------------------------------------------------------------------------------------------
//
// Returns the uniform selected color (i.e. 4 if everything selected is red), -1 if there is no such consensus.
//
LDColor MainWindow::getUniformSelectedColor()
{
	LDColor result;

	for (LDObject* obj : selectedObjects())
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

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::closeEvent (QCloseEvent* ev)
{
	// Check whether it's safe to close all files.
	if (not m_documents->isSafeToCloseAll())
	{
		ev->ignore();
		return;
	}

	// Save the toolbar set
	QStringList hiddenToolbars;

	for (QToolBar* toolbar : findChildren<QToolBar*>())
	{
		if (toolbar->isHidden())
			hiddenToolbars << toolbar->objectName();
	}

	// Save the configuration before leaving.
	m_config.setHiddenToolbars (hiddenToolbars);
	syncSettings();
	ev->accept();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::spawnContextMenu (const QPoint pos)
{
	const bool single = (selectedObjects().size() == 1);
	LDObject* singleObj = single ? selectedObjects().first() : nullptr;

	bool hasSubfiles = false;

	for (LDObject* obj : selectedObjects())
	{
		if (obj->type() == OBJ_SubfileReference)
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
	contextMenu->addAction (ui.actionRemove);
	contextMenu->addSeparator();
	contextMenu->addAction (ui.actionSetColor);

	if (single)
		contextMenu->addAction (ui.actionEditRaw);

	contextMenu->addAction (ui.actionMakeBorders);
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

	if (not selectedObjects().isEmpty())
	{
		contextMenu->addSeparator();
		contextMenu->addAction (ui.actionSubfileSelection);
	}

	if (renderer()->camera() != EFreeCamera)
	{
		contextMenu->addSeparator();
		contextMenu->addAction (ui.actionSetDrawDepth);
	}

	contextMenu->exec (pos);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::deleteByColor (LDColor color)
{
	LDObjectList objs;

	for (LDObject* obj : m_currentDocument->objects())
	{
		if (not obj->isColored() or obj->color() != color)
			continue;

		objs << obj;
	}

	for (LDObject* obj : objs)
		obj->destroy();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::updateEditModeActions()
{
	const EditModeType mode = renderer()->currentEditModeType();
	ui.actionModeSelect->setChecked (mode == EditModeType::Select);
	ui.actionModeDraw->setChecked (mode == EditModeType::Draw);
	ui.actionModeRectangle->setChecked (mode == EditModeType::Rectangle);
	ui.actionModeCircle->setChecked (mode == EditModeType::Circle);
	ui.actionModeMagicWand->setChecked (mode == EditModeType::MagicWand);
	ui.actionModeLinePath->setChecked (mode == EditModeType::LinePath);
	ui.actionModeCurve->setChecked (mode == EditModeType::Curve);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::objectListDoubleClicked (QListWidgetItem* listitem)
{
	LDObject* object = m_objectsInList.reverseLookup (listitem);
	AddObjectDialog::staticDialog (object->type(), object);
}

// ---------------------------------------------------------------------------------------------------------------------
//
bool MainWindow::save (LDDocument* doc, bool saveAs)
{
	if (doc->isCache())
		return false;

	QString path = doc->fullPath();
	int64 savesize;

	if (saveAs or path.isEmpty())
	{
		QString name = doc->defaultName();

		if (not doc->fullPath().isEmpty()) 
			name = doc->fullPath();
		else if (not doc->name().isEmpty())
			name = doc->name();

		name.replace ("\\", "/");
		path = QFileDialog::getSaveFileName (this, tr ("Save As"),
			name, tr ("LDraw files (*.dat *.ldr)"));

		if (path.isEmpty())
		{
			// User didn't give a file name, abort.
			return false;
		}
	}

	if (doc->save (path, &savesize))
	{
		if (doc == m_currentDocument)
			updateTitle();

		print ("Saved to %1 (%2)", path, formatFileSize (savesize));

		// Add it to recent files
		m_documents->addRecentFile (path);
		return true;
	}

	QString message = format (tr ("Failed to save to %1: %2"), path, strerror (errno));

	// Tell the user the save failed, and give the option for saving as with it.
	QMessageBox dlg (QMessageBox::Critical, tr ("Save Failure"), message, QMessageBox::Close, this);

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

// Adds a message to the renderer's message manager.
void MainWindow::addMessage (QString msg)
{
	m_renderer->messageLog()->addLine (msg);
}

// ============================================================================
void ObjectList::contextMenuEvent (QContextMenuEvent* ev)
{
	MainWindow* mainWindow = qobject_cast<MainWindow*>(parent());

	if (mainWindow)
		mainWindow->spawnContextMenu (ev->globalPos());
}

// ---------------------------------------------------------------------------------------------------------------------
//
QPixmap GetIcon (QString iconName)
{
	return (QPixmap (format (":/icons/%1.png", iconName)));
}

// ---------------------------------------------------------------------------------------------------------------------
//
bool Confirm (const QString& message)
{
	return Confirm (MainWindow::tr ("Confirm"), message);
}

// ---------------------------------------------------------------------------------------------------------------------
//
bool Confirm (const QString& title, const QString& message)
{
	return QMessageBox::question (g_win, title, message,
		(QMessageBox::Yes | QMessageBox::No), QMessageBox::No) == QMessageBox::Yes;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void Critical (const QString& message)
{
	QMessageBox::critical (g_win, MainWindow::tr ("Error"), message,
		(QMessageBox::Close), QMessageBox::Close);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void errorPrompt (QWidget* parent, const QString& message)
{
	QMessageBox::critical (parent, MainWindow::tr ("Error"), message,
		(QMessageBox::Close), QMessageBox::Close);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::updateDocumentList()
{
	m_updatingTabs = true;

	while (m_tabs->count() > 0)
		m_tabs->removeTab (0);

	for (LDDocument* document : m_documents->allDocuments())
	{
		if (not document->isCache())
		{
			// Add an item to the list for this file and store the tab index
			// in the document so we can find documents by tab index.
			document->setTabIndex (m_tabs->addTab (""));
			updateDocumentListItem (document);
		}
	}

	m_updatingTabs = false;
}

// ---------------------------------------------------------------------------------------------------------------------
//
// Update the given document's tab. If no such tab exists, the document list is rebuilt.
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
	if (doc == m_currentDocument)
		m_tabs->setCurrentIndex (doc->tabIndex());

	m_tabs->setTabText (doc->tabIndex(), doc->getDisplayName());

	// If the document.has unsaved changes, draw a little icon next to it to mark that.
	m_tabs->setTabIcon (doc->tabIndex(), doc->hasUnsavedChanges() ? GetIcon ("file-save") : QIcon());
	m_tabs->setTabData (doc->tabIndex(), doc->name());
	m_updatingTabs = oldUpdatingTabs;
}

// ---------------------------------------------------------------------------------------------------------------------
//
// A file is selected from the list of files on the left of the screen. Find out
// which file was picked and change to it.
//
void MainWindow::tabSelected()
{
	if (m_updatingTabs)
		return;

	LDDocument* switchee = nullptr;
	int tabIndex = m_tabs->currentIndex();

	// Find the file pointer of the item that was selected.
	for (LDDocument* document : m_documents->allDocuments())
	{
		if (not document->isCache() and document->tabIndex() == tabIndex)
		{
			switchee = document;
			break;
		}
	}

	if (switchee and switchee != m_currentDocument)
		changeDocument (switchee);
}

// ---------------------------------------------------------------------------------------------------------------------
//
// Updates the object list. Right now this just rebuilds it.
//
void MainWindow::refreshObjectList()
{
#if 0
	ui.objectList->clear();
	LDDocument* f = getm_currentDocument;

for (LDObject* obj : *f)
		ui.objectList->addItem (obj->qObjListEntry);

#endif

	buildObjectList();
}

// ---------------------------------------------------------------------------------------------------------------------
//
// Updates various actions, undo/redo are set enabled/disabled where appropriate, togglable actions are updated based
// on configuration, etc.
//
void MainWindow::updateActions()
{
	if (m_currentDocument and m_currentDocument->history())
	{
		EditHistory* his = m_currentDocument->history();
		int pos = his->position();
		ui.actionUndo->setEnabled (pos != -1);
		ui.actionRedo->setEnabled (pos < (long) his->size() - 1);
	}

	ui.actionWireframe->setChecked (m_config.drawWireframe());
	ui.actionAxes->setChecked (m_config.drawAxes());
	ui.actionBfcView->setChecked (m_config.bfcRedGreenView());
	ui.actionRandomColors->setChecked (m_config.randomColors());
	ui.actionDrawAngles->setChecked (m_config.drawAngles());
	ui.actionDrawSurfaces->setChecked (m_config.drawSurfaces());
	ui.actionDrawEdgeLines->setChecked (m_config.drawEdgeLines());
	ui.actionDrawConditionalLines->setChecked (m_config.drawConditionalLines());
}

// ---------------------------------------------------------------------------------------------------------------------
//
PrimitiveManager* MainWindow::primitives()
{
	return m_primitives;
}

// ---------------------------------------------------------------------------------------------------------------------
//
GLRenderer* MainWindow::renderer()
{
	return m_renderer;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::setQuickColors (const QList<ColorToolbarItem>& colors)
{
	m_quickColors = colors;
	updateColorToolbar();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::updatePrimitives()
{
	m_primitives->populateTreeWidget(ui.primitives);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::closeTab (int tabindex)
{
	LDDocument* doc = m_documents->findDocumentByName (m_tabs->tabData (tabindex).toString());

	if (doc)
		doc->close();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::historyTraversed()
{
	updateActions();
	refresh();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::loadShortcuts()
{
	for (QAction* act : findChildren<QAction*>())
	{
		QKeySequence seq = m_settings->value ("shortcut_" + act->objectName(), act->shortcut()).value<QKeySequence>();
		act->setShortcut (seq);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::saveShortcuts()
{
	applyToActions ([&](QAction* act)
	{
		QString const key = "shortcut_" + act->objectName();

		if (m_defaultShortcuts[act] != act->shortcut())
			m_settings->setValue (key, act->shortcut());
		else
			m_settings->remove (key);
	});
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::applyToActions (std::function<void(QAction*)> function)
{
	for (QAction* act : findChildren<QAction*>())
	{
		if (not act->objectName().isEmpty())
			function (act);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
//
QTreeWidget* MainWindow::getPrimitivesTree() const
{
	return ui.primitives;
}

// ---------------------------------------------------------------------------------------------------------------------
//
QKeySequence MainWindow::defaultShortcut (QAction* act)
{
	return m_defaultShortcuts[act];
}

// ---------------------------------------------------------------------------------------------------------------------
//
bool MainWindow::ringToolHiRes() const
{
	return ui.ringToolHiRes->isChecked();
}

// ---------------------------------------------------------------------------------------------------------------------
//
int MainWindow::ringToolSegments() const
{
	return ui.ringToolSegments->value();
}

// ---------------------------------------------------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::circleToolSegmentsChanged()
{
	int numerator (ui.ringToolSegments->value());
	int denominator (ui.ringToolHiRes->isChecked() ? HighResolution : LowResolution);
	simplify (numerator, denominator);
	ui.ringToolSegmentsLabel->setText (format ("%1 / %2", numerator, denominator));
}

// ---------------------------------------------------------------------------------------------------------------------
//
// Accessor to the settings object
//
QSettings* makeSettings (QObject* parent)
{
	QString path = qApp->applicationDirPath() + "/" UNIXNAME ".ini";
	return new QSettings (path, QSettings::IniFormat, parent);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::syncSettings()
{
	m_settings->sync();
}

// ---------------------------------------------------------------------------------------------------------------------
//
QVariant MainWindow::getConfigValue (QString name)
{
	QVariant value = m_settings->value (name, m_config.defaultValueByName (name));
	return value;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::createBlankDocument()
{
	// Create a new anonymous file and set it to our current
	LDDocument* f = newDocument();
	f->setName ("");
	changeDocument (f);
	closeInitialDocument();
	doFullRefresh();
	updateActions();
}

// ---------------------------------------------------------------------------------------------------------------------
//
LDDocument* MainWindow::newDocument (bool cache)
{
	LDDocument* document = m_documents->createNew();
	connect (document->history(), SIGNAL (undone()), this, SLOT (historyTraversed()));
	connect (document->history(), SIGNAL (redone()), this, SLOT (historyTraversed()));
	connect (document->history(), SIGNAL (stepAdded()), this, SLOT (updateActions()));

	if (not cache)
		document->openForEditing();

	return document;
}

// ---------------------------------------------------------------------------------------------------------------------
//
LDDocument* MainWindow::currentDocument()
{
	return m_currentDocument;
}

// ---------------------------------------------------------------------------------------------------------------------
//
// TODO: document may be null, this shouldn't be the case
//
void MainWindow::changeDocument (LDDocument* document)
{
	// Implicit files were loaded for caching purposes and may never be switched to.
	if (document and document->isCache())
		return;

	m_currentDocument = document;

	if (document)
	{
		// A ton of stuff needs to be updated
		updateDocumentListItem (document);
		buildObjectList();
		updateTitle();
		m_renderer->setDocument (document);
		m_renderer->compiler()->needMerge();
		print ("Changed document to %1", document->getDisplayName());
	}
}

// ---------------------------------------------------------------------------------------------------------------------
//
// This little beauty closes the initial file that was open at first when opening a new file over it.
//
void MainWindow::closeInitialDocument()
{
/*
	if (m_documents.size() == 2 and
		m_documents[0]->name().isEmpty() and
		not m_documents[1]->name().isEmpty() and
		not m_documents[0]->hasUnsavedChanges())
	{
		m_documents.first()->close();
	}
*/
}

// ---------------------------------------------------------------------------------------------------------------------
//
const LDObjectList& MainWindow::selectedObjects()
{
	return m_currentDocument->getSelection();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::currentDocumentClosed()
{
	LDDocument* old = currentDocument();

	// Find a replacement document to use
	for (LDDocument* doc : m_documents->allDocuments())
	{
		if (doc != old and not doc->isCache())
		{
			changeDocument (doc);
			break;
		}
	}

	if (currentDocument() == old)
	{
		// Failed to change to a suitable document, open a new one.
		createBlankDocument();
	}
}

ExtProgramToolset* MainWindow::externalPrograms()
{
	return m_externalPrograms;
}

GuiUtilities* MainWindow::guiUtilities()
{
	return m_guiUtilities;
}

Configuration* MainWindow::config()
{
	return &m_config;
}

Grid* MainWindow::grid()
{
	return m_grid;
}

MathFunctions* MainWindow::mathFunctions() const
{
	return m_mathFunctions;
}

// ---------------------------------------------------------------------------------------------------------------------
//
ColorToolbarItem::ColorToolbarItem (LDColor color, QToolButton* toolButton) :
	m_color (color),
	m_toolButton (toolButton) {}

ColorToolbarItem ColorToolbarItem::makeSeparator()
{
	return ColorToolbarItem (LDColor::nullColor(), nullptr);
}

bool ColorToolbarItem::isSeparator() const
{
	return color() == LDColor::nullColor();
}

LDColor ColorToolbarItem::color() const
{
	return m_color;
}

void ColorToolbarItem::setColor (LDColor color)
{
	m_color = color;
}

QToolButton* ColorToolbarItem::toolButton() const
{
	return m_toolButton;
}

void ColorToolbarItem::setToolButton (QToolButton* value)
{
	m_toolButton = value;
}