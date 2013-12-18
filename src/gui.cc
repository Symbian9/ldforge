/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 Santeri Piippo
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

#include "main.h"
#include "gldraw.h"
#include "gui.h"
#include "document.h"
#include "config.h"
#include "misc.h"
#include "colors.h"
#include "history.h"
#include "widgets.h"
#include "addObjectDialog.h"
#include "messagelog.h"
#include "config.h"
#include "ui_ldforge.h"
#include "moc_gui.cpp"

static bool g_isSelectionLocked = false;

cfg (Bool, lv_colorize, true);
cfg (String, gui_colortoolbar, "16:24:|:1:2:4:14:0:15:|:33:34:36:46");
cfg (Bool, gui_implicitfiles, false);
extern_cfg (List, io_recentfiles);
extern_cfg (Bool, gl_axes);
extern_cfg (String, gl_maincolor);
extern_cfg (Float, gl_maincolor_alpha);
extern_cfg (Bool, gl_wireframe);
extern_cfg (Bool, gl_colorbfc);

#define act(N) extern_cfg (KeySequence, key_##N);
#include "actions.h"

// =============================================================================
// -----------------------------------------------------------------------------
ForgeWindow::ForgeWindow()
{	g_win = this;
	m_renderer = new GLRenderer;

	ui = new Ui_LDForgeUI;
	ui->setupUi (this);

	// Stuff the renderer into its frame
	QVBoxLayout* rendererLayout = new QVBoxLayout (ui->rendererFrame);
	rendererLayout->addWidget (R());

	connect (ui->objectList, SIGNAL (itemSelectionChanged()), this, SLOT (slot_selectionChanged()));
	connect (ui->objectList, SIGNAL (itemDoubleClicked (QListWidgetItem*)), this, SLOT (slot_editObject (QListWidgetItem*)));
	connect (ui->fileList, SIGNAL (currentItemChanged (QListWidgetItem*, QListWidgetItem*)), this, SLOT (changeCurrentFile()));

	// Init message log manager
	m_msglog = new MessageManager;
	m_msglog->setRenderer (R());
	m_renderer->setMessageLog (m_msglog);
	m_quickColors = quickColorsFromConfig();
	slot_selectionChanged();
	setStatusBar (new QStatusBar);

	// Init primitive loader task stuff
	m_primLoaderBar = new QProgressBar;
	m_primLoaderWidget = new QWidget;
	QHBoxLayout* primLoaderLayout = new QHBoxLayout (m_primLoaderWidget);
	primLoaderLayout->addWidget (new QLabel ("Loading primitives:"));
	primLoaderLayout->addWidget (m_primLoaderBar);
	statusBar()->addPermanentWidget (m_primLoaderWidget);
	m_primLoaderWidget->hide();

	// Make certain actions checkable
	ui->actionAxes->setChecked (gl_axes);
	ui->actionWireframe->setChecked (gl_wireframe);
	ui->actionBFCView->setChecked (gl_colorbfc);
	updateGridToolBar();
	updateEditModeActions();
	updateRecentFilesMenu();
	updateToolBars();
	updateTitle();

	setMinimumSize (300, 200);

	connect (qApp, SIGNAL (aboutToQuit()), this, SLOT (slot_lastSecondCleanup()));

	// Connect all actions and set shortcuts
#define act(N) \
	connect (ui->action##N, SIGNAL (triggered()), this, SLOT (slot_action())); \
	ui->action##N->setShortcut (key_##N);
#include "actions.h"
}

// =============================================================================
// -----------------------------------------------------------------------------
void ForgeWindow::slot_action()
{	// Find out which action triggered this
#define act(N) if (sender() == ui->action##N) invokeAction (ui->action##N, &actiondef_##N);
#include "actions.h"
}

// =============================================================================
// -----------------------------------------------------------------------------
void ForgeWindow::invokeAction (QAction* act, void (*func)())
{
#ifdef DEBUG
	log ("Action %1 triggered", act->iconText());
#endif

	beginAction (act);
	(*func) ();
	endAction();
}

// =============================================================================
// -----------------------------------------------------------------------------
void ForgeWindow::slot_lastSecondCleanup()
{	delete m_renderer;
	delete ui;
}

// =============================================================================
// -----------------------------------------------------------------------------
void ForgeWindow::updateRecentFilesMenu()
{	// First, clear any items in the recent files menu
for (QAction * recent : m_recentFiles)
		delete recent;

	m_recentFiles.clear();

	QAction* first = null;

	for (const QVariant& it : io_recentfiles)
	{	str file = it.toString();
		QAction* recent = new QAction (getIcon ("open-recent"), file, this);

		connect (recent, SIGNAL (triggered()), this, SLOT (slot_recentFile()));
		ui->menuOpenRecent->insertAction (first, recent);
		m_recentFiles << recent;
		first = recent;
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
QList<LDQuickColor> quickColorsFromConfig()
{	QList<LDQuickColor> colors;

	for (str colorname : gui_colortoolbar.value.split (":"))
	{	if (colorname == "|")
			colors << LDQuickColor::getSeparator();
		else
		{	LDColor* col = getColor (colorname.toLong());

			if (col != null)
				colors << LDQuickColor (col, null);
		}
	}

	return colors;
}

// =============================================================================
// -----------------------------------------------------------------------------
void ForgeWindow::updateToolBars()
{	m_colorButtons.clear();
	ui->colorToolbar->clear();

	for (LDQuickColor& entry : m_quickColors)
	{	if (entry.isSeparator())
			ui->colorToolbar->addSeparator();
		else
		{	QToolButton* colorButton = new QToolButton;
			colorButton->setIcon (makeColorIcon (entry.getColor(), 22));
			colorButton->setIconSize (QSize (22, 22));
			colorButton->setToolTip (entry.getColor()->name);

			connect (colorButton, SIGNAL (clicked()), this, SLOT (slot_quickColor()));
			ui->colorToolbar->addWidget (colorButton);
			m_colorButtons << colorButton;

			entry.setToolButton (colorButton);
		}
	}

	updateGridToolBar();
}

// =============================================================================
// -----------------------------------------------------------------------------
void ForgeWindow::updateGridToolBar()
{	// Ensure that the current grid - and only the current grid - is selected.
	ui->actionGridCoarse->setChecked (grid == Grid::Coarse);
	ui->actionGridMedium->setChecked (grid == Grid::Medium);
	ui->actionGridFine->setChecked (grid == Grid::Fine);
}

// =============================================================================
// -----------------------------------------------------------------------------
void ForgeWindow::updateTitle()
{	str title = fmt (APPNAME " %1", fullVersionString());

	// Append our current file if we have one
	if (getCurrentDocument())
	{	if (getCurrentDocument()->getName().length() > 0)
			title += fmt (": %1", basename (getCurrentDocument()->getName()));
		else
			title += fmt (": <anonymous>");

		if (getCurrentDocument()->getObjectCount() > 0 &&
				getCurrentDocument()->getObject (0)->getType() == LDObject::Comment)
		{	// Append title
			LDComment* comm = static_cast<LDComment*> (getCurrentDocument()->getObject (0));
			title += fmt (": %1", comm->text);
		}

		if (getCurrentDocument()->getHistory()->getPosition() != getCurrentDocument()->getSavePosition())
			title += '*';
	}

#ifdef DEBUG
	title += " [debug build]";
#elif BUILD_ID != BUILD_RELEASE
	title += " [release build]";
#endif // DEBUG

	setWindowTitle (title);
}

// =============================================================================
// -----------------------------------------------------------------------------
int ForgeWindow::deleteSelection()
{	if (selection().isEmpty())
		return 0;

	QList<LDObject*> selCopy = selection();

	// Delete the objects that were being selected
	for (LDObject* obj : selCopy)
		delete obj;

	refresh();
	return selCopy.size();
}

// =============================================================================
// -----------------------------------------------------------------------------
void ForgeWindow::buildObjList()
{	if (!getCurrentDocument())
		return;

	// Lock the selection while we do this so that refreshing the object list
	// doesn't trigger selection updating so that the selection doesn't get lost
	// while this is done.
	g_isSelectionLocked = true;

	for (int i = 0; i < ui->objectList->count(); ++i)
		delete ui->objectList->item (i);

	ui->objectList->clear();

	for (LDObject* obj : getCurrentDocument()->getObjects())
	{	str descr;

		switch (obj->getType())
		{	case LDObject::Comment:
			{	descr = static_cast<LDComment*> (obj)->text;

				// Remove leading whitespace
				while (descr[0] == ' ')
					descr.remove (0, 1);
			} break;

			case LDObject::Empty:
				break; // leave it empty

			case LDObject::Line:
			case LDObject::Triangle:
			case LDObject::Quad:
			case LDObject::CondLine:
			{	for (int i = 0; i < obj->vertices(); ++i)
				{	if (i != 0)
						descr += ", ";

					descr += obj->getVertex (i).stringRep (true);
				}
			} break;

			case LDObject::Error:
			{	descr = fmt ("ERROR: %1", obj->raw());
			} break;

			case LDObject::Vertex:
			{	descr = static_cast<LDVertex*> (obj)->pos.stringRep (true);
			} break;

			case LDObject::Subfile:
			{	LDSubfile* ref = static_cast<LDSubfile*> (obj);

				descr = fmt ("%1 %2, (", ref->getFileInfo()->getName(), ref->getPosition().stringRep (true));

				for (int i = 0; i < 9; ++i)
					descr += fmt ("%1%2", ref->getTransform()[i], (i != 8) ? " " : "");

				descr += ')';
			} break;

			case LDObject::BFC:
			{	descr = LDBFC::statements[static_cast<LDBFC*> (obj)->type];
			} break;

			case LDObject::Overlay:
			{	LDOverlay* ovl = static_cast<LDOverlay*> (obj);
				descr = fmt ("[%1] %2 (%3, %4), %5 x %6", g_CameraNames[ovl->getCamera()],
					basename (ovl->getFileName()), ovl->getX(), ovl->getY(),
					ovl->getWidth(), ovl->getHeight());
			}
			break;

			default:
			{	descr = obj->getTypeName();
			} break;
		}

		QListWidgetItem* item = new QListWidgetItem (descr);
		item->setIcon (getIcon (obj->getTypeName()));
		
		// Use italic font if hidden
		if (obj->isHidden())
		{	QFont font = item->font();
			font.setItalic (true);
			item->setFont (font);
		}

		// Color gibberish orange on red so it stands out.
		if (obj->getType() == LDObject::Error)
		{	item->setBackground (QColor ("#AA0000"));
			item->setForeground (QColor ("#FFAA00"));
		}
		elif (lv_colorize && obj->isColored() && obj->getColor() != maincolor && obj->getColor() != edgecolor)
		{	// If the object isn't in the main or edge color, draw this
			// list entry in said color.
			LDColor* col = getColor (obj->getColor());

			if (col)
				item->setForeground (col->faceColor);
		}

		obj->qObjListEntry = item;
		ui->objectList->insertItem (ui->objectList->count(), item);
	}

	g_isSelectionLocked = false;
	updateSelection();
	scrollToSelection();
}

// =============================================================================
// -----------------------------------------------------------------------------
void ForgeWindow::scrollToSelection()
{	if (selection().isEmpty())
		return;

	LDObject* obj = selection().last();
	ui->objectList->scrollToItem (obj->qObjListEntry);
}

// =============================================================================
// -----------------------------------------------------------------------------
void ForgeWindow::slot_selectionChanged()
{	if (g_isSelectionLocked == true || getCurrentDocument() == null)
		return;

	// Update the shared selection array, though don't do this if this was
	// called during GL picking, in which case the GL renderer takes care
	// of the selection.
	if (m_renderer->isPicking())
		return;

	QList<LDObject*> priorSelection = selection();

	// Get the objects from the object list selection
	getCurrentDocument()->clearSelection();
	const QList<QListWidgetItem*> items = ui->objectList->selectedItems();

	for (LDObject* obj : getCurrentDocument()->getObjects())
	{	for (QListWidgetItem* item : items)
		{	if (item == obj->qObjListEntry)
			{	obj->select();
				break;
			}
		}
	}

	// Update the GL renderer
	QList<LDObject*> compound = priorSelection + selection();
	removeDuplicates (compound);

	for (LDObject* obj : compound)
		m_renderer->compileObject (obj);

	m_renderer->update();
}

// =============================================================================
// -----------------------------------------------------------------------------
void ForgeWindow::slot_recentFile()
{	QAction* qAct = static_cast<QAction*> (sender());
	openMainFile (qAct->text());
}

// =============================================================================
// -----------------------------------------------------------------------------
void ForgeWindow::slot_quickColor()
{	beginAction (null);
	QToolButton* button = static_cast<QToolButton*> (sender());
	LDColor* col = null;

	for (const LDQuickColor & entry : m_quickColors)
	{	if (entry.getToolButton() == button)
		{	col = entry.getColor();
			break;
		}
	}

	if (col == null)
		return;

	int newColor = col->index;

	for (LDObject* obj : selection())
	{	if (obj->isColored() == false)
			continue; // uncolored object

		obj->setColor (newColor);
		R()->compileObject (obj);
	}

	refresh();
	endAction();
}

// =============================================================================
// -----------------------------------------------------------------------------
int ForgeWindow::getInsertionPoint()
{	// If we have a selection, put the item after it.
	if (!selection().isEmpty())
		return selection().last()->getIndex() + 1;

	// Otherwise place the object at the end.
	return getCurrentDocument()->getObjectCount();
}

// =============================================================================
// -----------------------------------------------------------------------------
void ForgeWindow::doFullRefresh()
{	buildObjList();
	m_renderer->hardRefresh();
}

// =============================================================================
// -----------------------------------------------------------------------------
void ForgeWindow::refresh()
{	buildObjList();
	m_renderer->update();
}

// =============================================================================
// -----------------------------------------------------------------------------
void ForgeWindow::updateSelection()
{	g_isSelectionLocked = true;

	for (LDObject* obj : getCurrentDocument()->getObjects())
		obj->setSelected (false);

	ui->objectList->clearSelection();

	for (LDObject* obj : selection())
	{	if (obj->qObjListEntry == null)
			continue;

		obj->qObjListEntry->setSelected (true);
		obj->setSelected (true);
	}

	g_isSelectionLocked = false;
	slot_selectionChanged();
}

// =============================================================================
// -----------------------------------------------------------------------------
int ForgeWindow::getSelectedColor()
{	int result = -1;

	for (LDObject* obj : selection())
	{	if (obj->isColored() == false)
			continue; // doesn't use color

		if (result != -1 && obj->getColor() != result)
			return -1; // No consensus in object color

		if (result == -1)
			result = obj->getColor();
	}

	return result;
}

// =============================================================================
// -----------------------------------------------------------------------------
LDObject::Type ForgeWindow::getUniformSelectedType()
{	LDObject::Type result = LDObject::Unidentified;

	for (LDObject * obj : selection())
	{	if (result != LDObject::Unidentified && obj->getColor() != result)
			return LDObject::Unidentified;

		if (result == LDObject::Unidentified)
			result = obj->getType();
	}

	return result;
}

// =============================================================================
// -----------------------------------------------------------------------------
void ForgeWindow::closeEvent (QCloseEvent* ev)
{	// Check whether it's safe to close all files.
	if (!safeToCloseAll())
	{	ev->ignore();
		return;
	}

	// Save the configuration before leaving so that, for instance, grid choice
	// is preserved across instances.
	Config::save();

	ev->accept();
}

// =============================================================================
// -----------------------------------------------------------------------------
void ForgeWindow::spawnContextMenu (const QPoint pos)
{	const bool single = (selection().size() == 1);
	LDObject* singleObj = (single) ? selection()[0] : null;

	QMenu* contextMenu = new QMenu;

	if (single && singleObj->getType() != LDObject::Empty)
	{	contextMenu->addAction (ui->actionEdit);
		contextMenu->addSeparator();
	}

	contextMenu->addAction (ui->actionCut);
	contextMenu->addAction (ui->actionCopy);
	contextMenu->addAction (ui->actionPaste);
	contextMenu->addAction (ui->actionDelete);
	contextMenu->addSeparator();
	contextMenu->addAction (ui->actionSetColor);

	if (single)
		contextMenu->addAction (ui->actionEditRaw);

	contextMenu->addAction (ui->actionBorders);
	contextMenu->addAction (ui->actionSetOverlay);
	contextMenu->addAction (ui->actionClearOverlay);
	contextMenu->addAction (ui->actionModeSelect);
	contextMenu->addAction (ui->actionModeDraw);
	contextMenu->addAction (ui->actionModeCircle);

	if (R()->camera() != GL::EFreeCamera)
	{	contextMenu->addSeparator();
		contextMenu->addAction (ui->actionSetDrawDepth);
	}

	contextMenu->exec (pos);
}

// =============================================================================
// -----------------------------------------------------------------------------
void ForgeWindow::deleteObjects (QList<LDObject*> objs)
{	for (LDObject * obj : objs)
	{	getCurrentDocument()->forgetObject (obj);
		delete obj;
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
void ForgeWindow::deleteByColor (const int colnum)
{	QList<LDObject*> objs;

	for (LDObject* obj : getCurrentDocument()->getObjects())
	{	if (!obj->isColored() || obj->getColor() != colnum)
			continue;

		objs << obj;
	}

	deleteObjects (objs);
}

// =============================================================================
// -----------------------------------------------------------------------------
void ForgeWindow::updateEditModeActions()
{	const EditMode mode = R()->getEditMode();
	ui->actionModeSelect->setChecked (mode == ESelectMode);
	ui->actionModeDraw->setChecked (mode == EDrawMode);
	ui->actionModeCircle->setChecked (mode == ECircleMode);
}

// =============================================================================
// -----------------------------------------------------------------------------
void ForgeWindow::slot_editObject (QListWidgetItem* listitem)
{	LDObject* obj = null;

	for (LDObject* it : getCurrentDocument()->getObjects())
	{	if (it->qObjListEntry == listitem)
		{	obj = it;
			break;
		}
	}

	AddObjectDialog::staticDialog (obj->getType(), obj);
}

#if 0
// =============================================================================
// -----------------------------------------------------------------------------
void ForgeWindow::primitiveLoaderStart (int max)
{	m_primLoaderWidget->show();
	m_primLoaderBar->setRange (0, max);
	m_primLoaderBar->setValue (0);
	m_primLoaderBar->setFormat ("%p%");
}

// =============================================================================
// -----------------------------------------------------------------------------
void ForgeWindow::primitiveLoaderUpdate (int prog)
{	m_primLoaderBar->setValue (prog);
}

// =============================================================================
// -----------------------------------------------------------------------------
void ForgeWindow::primitiveLoaderEnd()
{	QTimer* hidetimer = new QTimer;
	connect (hidetimer, SIGNAL (timeout()), m_primLoaderWidget, SLOT (hide()));
	hidetimer->setSingleShot (true);
	hidetimer->start (1500);
	m_primLoaderBar->setFormat (tr ("Done"));
	log (tr ("Primitives scanned: %1 primitives listed"), m_primLoaderBar->value());
}
#endif

// =============================================================================
// -----------------------------------------------------------------------------
void ForgeWindow::save (LDDocument* f, bool saveAs)
{	str path = f->getName();

	if (saveAs || path.isEmpty())
	{	path = QFileDialog::getSaveFileName (g_win, tr ("Save As"),
			(f->getName().isEmpty()) ? f->getName() : f->getDefaultName(),
			tr ("LDraw files (*.dat *.ldr)"));

		if (path.isEmpty())
		{	// User didn't give a file name, abort.
			return;
		}
	}

	if (f->save (path))
	{	f->setName (path);

		if (f == getCurrentDocument())
			g_win->updateTitle();

		log ("Saved to %1.", path);

		// Add it to recent files
		addRecentFile (path);
	}
	else
	{	str message = fmt (tr ("Failed to save to %1: %2"), path, strerror (errno));

		// Tell the user the save failed, and give the option for saving as with it.
		QMessageBox dlg (QMessageBox::Critical, tr ("Save Failure"), message, QMessageBox::Close, g_win);

		// Add a save-as button
		QPushButton* saveAsBtn = new QPushButton (tr ("Save As"));
		saveAsBtn->setIcon (getIcon ("file-save-as"));
		dlg.addButton (saveAsBtn, QMessageBox::ActionRole);
		dlg.setDefaultButton (QMessageBox::Close);
		dlg.exec();

		if (dlg.clickedButton() == saveAsBtn)
			save (f, true); // yay recursion!
	}
}

void ForgeWindow::addMessage (str msg)
{	m_msglog->addLine (msg);
}

// ============================================================================
void ObjectList::contextMenuEvent (QContextMenuEvent* ev)
{	g_win->spawnContextMenu (ev->globalPos());
}

// =============================================================================
// -----------------------------------------------------------------------------
QPixmap getIcon (str iconName)
{	return (QPixmap (fmt (":/icons/%1.png", iconName)));
}

// =============================================================================
bool confirm (str msg)
{	return confirm (ForgeWindow::tr ("Confirm"), msg);
}

bool confirm (str title, str msg)
{	return QMessageBox::question (g_win, title, msg,
		(QMessageBox::Yes | QMessageBox::No), QMessageBox::No) == QMessageBox::Yes;
}

// =============================================================================
void critical (str msg)
{	QMessageBox::critical (g_win, ForgeWindow::tr ("Error"), msg,
		(QMessageBox::Close), QMessageBox::Close);
}

// =============================================================================
QIcon makeColorIcon (LDColor* colinfo, const int size)
{	// Create an image object and link a painter to it.
	QImage img (size, size, QImage::Format_ARGB32);
	QPainter paint (&img);
	QColor col = colinfo->faceColor;

	if (colinfo->index == maincolor)
	{	// Use the user preferences for main color here
		col = gl_maincolor.value;
		col.setAlphaF (gl_maincolor_alpha);
	}

	// Paint the icon border
	paint.fillRect (QRect (0, 0, size, size), colinfo->edgeColor);

	// Paint the checkerboard background, visible with translucent icons
	paint.drawPixmap (QRect (1, 1, size - 2, size - 2), getIcon ("checkerboard"), QRect (0, 0, 8, 8));

	// Paint the color above the checkerboard
	paint.fillRect (QRect (1, 1, size - 2, size - 2), col);
	return QIcon (QPixmap::fromImage (img));
}

// =============================================================================
void makeColorComboBox (QComboBox* box)
{	std::map<int, int> counts;

	for (LDObject * obj : getCurrentDocument()->getObjects())
	{	if (!obj->isColored())
			continue;

		if (counts.find (obj->getColor()) == counts.end())
			counts[obj->getColor()] = 1;
		else
			counts[obj->getColor()]++;
	}

	box->clear();
	int row = 0;

	for (const auto& pair : counts)
	{	LDColor* col = getColor (pair.first);
		assert (col != null);

		QIcon ico = makeColorIcon (col, 16);
		box->addItem (ico, fmt ("[%1] %2 (%3 object%4)",
			pair.first, col->name, pair.second, plural (pair.second)));
		box->setItemData (row, pair.first);

		++row;
	}
}

void ForgeWindow::setStatusBarText (str text)
{	statusBar()->showMessage (text);
}

Ui_LDForgeUI* ForgeWindow::interface() const
{	return ui;
}

#define act(N) QAction* ForgeWindow::action##N() { return ui->action##N; }
#include "actions.h"

void ForgeWindow::updateDocumentList()
{	ui->fileList->clear();

	for (LDDocument* f : g_loadedFiles)
	{	// Don't list implicit files unless explicitly desired.
		if (f->isImplicit() && !gui_implicitfiles)
			continue;

		// Add an item to the list for this file and store a pointer to it in
		// the file, so we can find files by the list item.
		ui->fileList->addItem ("");
		QListWidgetItem* item = ui->fileList->item (ui->fileList->count() - 1);
		f->setListItem (item);

		updateDocumentListItem (f);
	}
}

void ForgeWindow::updateDocumentListItem (LDDocument* f)
{	if (f->getListItem() == null)
	{	// We don't have a list item for this file, so the list either doesn't
		// exist yet or is out of date. Build the list now.
		updateDocumentList();
		return;
	}

	// If this is the current file, it also needs to be the selected item on
	// the list.
	if (f == getCurrentDocument())
		ui->fileList->setCurrentItem (f->getListItem());

	// If we list implicit files, draw them with a shade of gray to make them
	// distinct.
	if (f->isImplicit())
		f->getListItem()->setForeground (QColor (96, 96, 96));

	f->getListItem()->setText (f->getShortName());

	// If the document has unsaved changes, draw a little icon next to it to mark that.
	f->getListItem()->setIcon (f->hasUnsavedChanges() ? getIcon ("file-save") : QIcon());
}

void ForgeWindow::beginAction (QAction* act)
{	// Open the history so we can record the edits done during this action.
	if (act == ui->actionUndo || act == ui->actionRedo || act == ui->actionOpen)
		getCurrentDocument()->getHistory()->setIgnoring (true);
}

void ForgeWindow::endAction()
{	// Close the history now.
	getCurrentDocument()->addHistoryStep();

	// Update the list item of the current file - we may need to draw an icon
	// now that marks it as having unsaved changes.
	updateDocumentListItem (getCurrentDocument());
}

// =============================================================================
// A file is selected from the list of files on the left of the screen. Find out
// which file was picked and change to it.
void ForgeWindow::changeCurrentFile()
{	LDDocument* f = null;
	QListWidgetItem* item = ui->fileList->currentItem();

	// Find the file pointer of the item that was selected.
	for (LDDocument* it : g_loadedFiles)
	{	if (it->getListItem() == item)
		{	f = it;
			break;
		}
	}

	// If we picked the same file we're currently on, we don't need to do
	// anything.
	if (!f || f == getCurrentDocument())
		return;

	LDDocument::setCurrent (f);
}

void ForgeWindow::refreshObjectList()
{
#if 0
	ui->objectList->clear();
	LDDocument* f = getCurrentDocument();

for (LDObject * obj : *f)
		ui->objectList->addItem (obj->qObjListEntry);

#endif

	buildObjList();
}

void ForgeWindow::updateActions()
{	History* his = getCurrentDocument()->getHistory();
	int pos = his->getPosition();
	ui->actionUndo->setEnabled (pos != -1);
	ui->actionRedo->setEnabled (pos < (long) his->getSize() - 1);
	ui->actionAxes->setChecked (gl_axes);
	ui->actionBFCView->setChecked (gl_colorbfc);
}

QImage imageFromScreencap (uchar* data, int w, int h)
{	// GL and Qt formats have R and B swapped. Also, GL flips Y - correct it as well.
	return QImage (data, w, h, QImage::Format_ARGB32).rgbSwapped().mirrored();
}

// =============================================================================
// -----------------------------------------------------------------------------
LDQuickColor::LDQuickColor (LDColor* color, QToolButton* toolButton) :
	m_Color (color),
	m_ToolButton (toolButton) {}

LDQuickColor LDQuickColor::getSeparator()
{	return LDQuickColor (null, null);
}

bool LDQuickColor::isSeparator() const
{	return getColor() == null;
}