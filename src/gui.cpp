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

#include "common.h"
#include "gldraw.h"
#include "gui.h"
#include "file.h"
#include "config.h"
#include "misc.h"
#include "colors.h"
#include "history.h"
#include "widgets.h"
#include "addObjectDialog.h"
#include "messagelog.h"
#include "config.h"
#include "ui_ldforge.h"

static bool g_bSelectionLocked = false;

cfg (bool, lv_colorize, true);
cfg (str, gui_colortoolbar, "16:24:|:1:2:4:14:0:15:|:33:34:36:46");
extern_cfg (str, io_recentfiles);
extern_cfg (bool, gl_axes);
extern_cfg (str, gl_maincolor);
extern_cfg (float, gl_maincolor_alpha);
extern_cfg (bool, gl_wireframe);
extern_cfg (bool, gl_colorbfc);

#define act(N) extern_cfg (keyseq, key_##N);
#include "actions.h"

const char* g_modeActionNames[] = {
	"modeSelect",
	"modeDraw",
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ForgeWindow::ForgeWindow() {
	g_win = this;
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

void ForgeWindow::slot_action() {
	// Find out which action triggered this
#define act(N) if (sender() == ui->action##N) invokeAction (ui->action##N, &actiondef_##N);
#include "actions.h"
}

void ForgeWindow::invokeAction (QAction* act, void (*func) ()) {
	beginAction (act);
	(*func) ();
	endAction();
}

// =============================================================================
void ForgeWindow::slot_lastSecondCleanup() {
	delete m_renderer;
	delete ui;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::updateRecentFilesMenu() {
	// First, clear any items in the recent files menu
	for (QAction* recent : m_recentFiles)
		delete recent;
	m_recentFiles.clear();
	
	vector<str> files = container_cast<QStringList, vector<str>> (io_recentfiles.value.split ("@"));
	for (str file : c_rev<str> (files)) {
		QAction* recent = new QAction (getIcon ("open-recent"), file, this);
		
		connect (recent, SIGNAL (triggered()), this, SLOT (slot_recentFile()));
		ui->menuOpenRecent->addAction (recent);
		m_recentFiles << recent;
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
vector<LDQuickColor> quickColorsFromConfig() {
	vector<LDQuickColor> colors;
	
	for (str colorname : gui_colortoolbar.value.split (":")) {
		if (colorname == "|") {
			colors << LDQuickColor ({null, null, true});
		} else {
			LDColor* col = getColor (colorname.toLong());
			assert (col != null);
			colors << LDQuickColor ({col, null, false});
		}
	}
	
	return colors;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::updateToolBars() {
	m_colorButtons.clear();
	ui->colorToolbar->clear();
	
	for (LDQuickColor& entry : m_quickColors) {
		if (entry.isSeparator)
			ui->colorToolbar->addSeparator();
		else {
			QToolButton* colorButton = new QToolButton;
			colorButton->setIcon (makeColorIcon (entry.col, 22));
			colorButton->setIconSize (QSize (22, 22));
			colorButton->setToolTip (entry.col->name);
			
			connect (colorButton, SIGNAL (clicked()), this, SLOT (slot_quickColor()));
			ui->colorToolbar->addWidget (colorButton);
			m_colorButtons << colorButton;
			
			entry.btn = colorButton;
		}
	}
	
	updateGridToolBar();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::updateGridToolBar() {
	// Ensure that the current grid - and only the current grid - is selected.
	ui->actionGridCoarse->setChecked (grid == Grid::Coarse);
	ui->actionGridMedium->setChecked (grid == Grid::Medium);
	ui->actionGridFine->setChecked (grid == Grid::Fine);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::updateTitle() {
	str title = fmt (APPNAME " %1", fullVersionString());
	
	// Append our current file if we have one
	if (LDOpenFile::current()) {
		if (LDOpenFile::current()->name().length() > 0)
			title += fmt (": %1", basename (LDOpenFile::current()->name()));
		else
			title += fmt (": <anonymous>");
		
		if (LDOpenFile::current()->numObjs() > 0 &&
			LDOpenFile::current()->obj (0)->getType() == LDObject::Comment)
		{
			// Append title
			LDCommentObject* comm = static_cast<LDCommentObject*> (LDOpenFile::current()->obj (0));
			title += fmt (": %1", comm->text);
		}
		
		if (LDOpenFile::current()->history().pos() != LDOpenFile::current()->savePos())
			title += '*';
	}
	
	setWindowTitle (title);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
int ForgeWindow::deleteSelection()
{
	if( m_sel.size() == 0 )
		return 0;
	
	vector<LDObject*> selCopy = m_sel;
	int num = 0;
	
	// Delete the objects that were being selected
	for (LDObject* obj : selCopy) {
		LDOpenFile::current()->forgetObject (obj);
		++num;
		delete obj;
	}
	
	refresh();
	return num;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::buildObjList() {
	if (!LDOpenFile::current())
		return;
	
	// Lock the selection while we do this so that refreshing the object list
	// doesn't trigger selection updating so that the selection doesn't get lost
	// while this is done.
	g_bSelectionLocked = true;
	
	for (int i = 0; i < ui->objectList->count(); ++i)
		delete ui->objectList->item (i);
	
	ui->objectList->clear();
	
	for (LDObject* obj : LDOpenFile::current()->objs()) {
		str descr;
		
		switch (obj->getType()) {
		case LDObject::Comment:
			descr = static_cast<LDCommentObject*> (obj)->text;
			
			// Remove leading whitespace
			while (descr[0] == ' ')
				descr.remove (0, 1);
			break;
		
		case LDObject::Empty:
			break; // leave it empty
		
		case LDObject::Line:
		case LDObject::Triangle:
		case LDObject::Quad:
		case LDObject::CondLine:
			for (short i = 0; i < obj->vertices(); ++i) {
				if (i != 0)
					descr += ", ";
				
				descr += obj->getVertex (i).stringRep (true);
			}
			break;
		
		case LDObject::Error:
			descr = fmt ("ERROR: %1", obj->raw());
			break;
		
		case LDObject::Vertex:
			descr = static_cast<LDVertexObject*> (obj)->pos.stringRep (true);
			break;
		
		case LDObject::Subfile:
			{
				LDSubfileObject* ref = static_cast<LDSubfileObject*> (obj);
				
				descr = fmt ("%1 %2, (", ref->fileInfo()->name(),
					ref->position().stringRep (true));
				
				for (short i = 0; i < 9; ++i)
					descr += fmt ("%1%2", ftoa (ref->transform()[i]),
						(i != 8) ? " " : "");
				
				descr += ')';
			}
			break;
		
		case LDObject::BFC:
			descr = LDBFCObject::statements[static_cast<LDBFCObject*> (obj)->type];
		break;
		
		case LDObject::Overlay:
			{
				LDOverlayObject* ovl = static_cast<LDOverlayObject*>( obj );
				descr = fmt( "[%1] %2 (%3, %4), %5 x %6", g_CameraNames[ovl->camera()],
					basename( ovl->filename() ), ovl->x(), ovl->y(), ovl->width(), ovl->height() );
			}
			break;
		
		default:
			descr = obj->typeName();
			break;
		}
		
		// Put it into brackets if it's hidden
		if (obj->hidden()) {
			descr = fmt ("[[ %1 ]]", descr);
		}
		
		QListWidgetItem* item = new QListWidgetItem (descr);
		item->setIcon( getIcon( obj->typeName() ));
		
		// Color gibberish orange on red so it stands out.
		if (obj->getType() == LDObject::Error) {
			item->setBackground (QColor ("#AA0000"));
			item->setForeground (QColor ("#FFAA00"));
		} elif (lv_colorize && obj->isColored() &&
			obj->color() != maincolor && obj->color() != edgecolor)
		{
			// If the object isn't in the main or edge color, draw this
			// list entry in said color.
			LDColor* col = getColor (obj->color());
			if (col)
				item->setForeground (col->faceColor);
		}
		
		obj->qObjListEntry = item;
		ui->objectList->insertItem (ui->objectList->count(), item);
	}
	
	g_bSelectionLocked = false;
	updateSelection();
	scrollToSelection();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::scrollToSelection() {
	if (m_sel.size() == 0)
		return;
	
	LDObject* obj = m_sel[m_sel.size() - 1];
	ui->objectList->scrollToItem (obj->qObjListEntry);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::slot_selectionChanged() {
	if (g_bSelectionLocked == true || LDOpenFile::current() == null)
		return;
	
	// Update the shared selection array, though don't do this if this was
	// called during GL picking, in which case the GL renderer takes care
	// of the selection.
	if (m_renderer->picking())
		return;
	
	vector<LDObject*> priorSelection = m_sel;
	
	// Get the objects from the object list selection
	m_sel.clear();
	const QList<QListWidgetItem*> items = ui->objectList->selectedItems();
	
	for (LDObject* obj : LDOpenFile::current()->objs())
	for (QListWidgetItem* item : items) {
		if (item == obj->qObjListEntry) {
			m_sel << obj;
			break;
		}
	}
	
	// Update the GL renderer
	for (LDObject* obj : priorSelection) {
		obj->setSelected (false);
		m_renderer->compileObject (obj);
	}
	
	for (LDObject* obj : m_sel) {
		obj->setSelected (true);
		m_renderer->compileObject (obj);
	}
	
	m_renderer->update();
}

// =============================================================================
void ForgeWindow::slot_recentFile() {
	QAction* qAct = static_cast<QAction*> (sender());
	openMainFile (qAct->text());
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::slot_quickColor() {
	beginAction (null);
	QToolButton* button = static_cast<QToolButton*> (sender());
	LDColor* col = null;
	
	for (LDQuickColor entry : m_quickColors) {
		if (entry.btn == button) {
			col = entry.col;
			break;
		}
	}
	
	if (col == null)
		return;
	
	short newColor = col->index;
	
	for (LDObject* obj : m_sel) {
		if (obj->isColored() == false)
			continue; // uncolored object
		
		obj->setColor (newColor);
	}
	
	fullRefresh();
	endAction();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ulong ForgeWindow::getInsertionPoint() {
	if (m_sel.size() > 0) {
		// If we have a selection, put the item after it.
		return (m_sel[m_sel.size() - 1]->getIndex()) + 1;
	}
	
	// Otherwise place the object at the end.
	return LDOpenFile::current()->numObjs();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::fullRefresh() {
	buildObjList();
	m_renderer->hardRefresh();
}

void ForgeWindow::refresh() {
	buildObjList();
	m_renderer->update();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::updateSelection() {
	g_bSelectionLocked = true;
	
	for (LDObject* obj : LDOpenFile::current()->objs())
		obj->setSelected (false);
	
	ui->objectList->clearSelection();
	for (LDObject* obj : m_sel) {
		if( obj->qObjListEntry == null )
			continue;
		
		obj->qObjListEntry->setSelected (true);
		obj->setSelected (true);
	}
	
	g_bSelectionLocked = false;
	slot_selectionChanged();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
bool ForgeWindow::isSelected (LDObject* obj) {
	LDObject* needle = obj->topLevelParent();
	
	for (LDObject* hay : m_sel)
		if (hay == needle)
			return true;
	
	return false;
}

short ForgeWindow::getSelectedColor() {
	short result = -1;
	
	for (LDObject* obj : m_sel) {
		if (obj->isColored() == false)
			continue; // doesn't use color
		
		if (result != -1 && obj->color() != result)
			return -1; // No consensus in object color
		
		if (result == -1)
			result = obj->color();
	}
	
	return result;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
LDObject::Type ForgeWindow::uniformSelectedType() {
	LDObject::Type result = LDObject::Unidentified;
	
	for (LDObject* obj : m_sel) {
		if (result != LDObject::Unidentified && obj->color() != result)
			return LDObject::Unidentified;
		
		if (result == LDObject::Unidentified)
			result = obj->getType();
	}
	
	return result;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::closeEvent (QCloseEvent* ev) {
	// Check whether it's safe to close all files.
	if (!safeToCloseAll()) {
		ev->ignore();
		return;
	}
	
	// Save the configuration before leaving so that, for instance, grid choice
	// is preserved across instances.
	config::save();
	
	ev->accept();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::spawnContextMenu (const QPoint pos) {
	const bool single = (g_win->sel().size() == 1);
	LDObject* singleObj = (single) ? g_win->sel()[0] : null;
	
	QMenu* contextMenu = new QMenu;
	
	if (single && singleObj->getType() != LDObject::Empty) {
		contextMenu->addAction (ACTION (Edit));
		contextMenu->addSeparator();
	}
	
	contextMenu->addAction (ACTION (Cut));
	contextMenu->addAction (ACTION (Copy));
	contextMenu->addAction (ACTION (Paste));
	contextMenu->addAction (ACTION (Delete));
	contextMenu->addSeparator();
	contextMenu->addAction (ACTION (SetColor));
	
	if (single)
		contextMenu->addAction (ACTION (EditRaw));
	
	contextMenu->addAction (ACTION (Borders));
	contextMenu->addAction (ACTION (SetOverlay));
	contextMenu->addAction (ACTION (ClearOverlay));
	contextMenu->addAction (ACTION (ModeSelect));
	contextMenu->addAction (ACTION (ModeDraw));
	
	if (R()->camera() != GL::Free) {
		contextMenu->addSeparator();
		contextMenu->addAction (ACTION (SetDrawDepth));
	}
	
	contextMenu->exec (pos);
}

// =============================================================================
void ForgeWindow::deleteObjVector (vector<LDObject*> objs) {
	for (LDObject* obj : objs) {
		LDOpenFile::current()->forgetObject (obj);
		delete obj;
	}
}

// =============================================================================
void ForgeWindow::deleteByColor (const short colnum) {
	vector<LDObject*> objs;
	for (LDObject* obj : LDOpenFile::current()->objs()) {
		if (!obj->isColored() || obj->color() != colnum)
			continue;
		
		objs << obj;
	}
	
	deleteObjVector (objs);
}

// =============================================================================
void ForgeWindow::updateEditModeActions() {
	const EditMode mode = R()->editMode();
	ACTION (ModeSelect)->setChecked (mode == Select);
	ACTION (ModeDraw)->setChecked (mode == Draw);
}

void ForgeWindow::slot_editObject (QListWidgetItem* listitem) {
	LDObject* obj = null;
	for (LDObject* it : *LDOpenFile::current()) {
		if (it->qObjListEntry == listitem) {
			obj = it;
			break;
		}
	}
	
	AddObjectDialog::staticDialog (obj->getType(), obj);
}

void ForgeWindow::primitiveLoaderStart (ulong max) {
	m_primLoaderWidget->show();
	m_primLoaderBar->setRange (0, max);
	m_primLoaderBar->setValue (0);
	m_primLoaderBar->setFormat ("%p%");
}

void ForgeWindow::primitiveLoaderUpdate (ulong prog) {
	m_primLoaderBar->setValue (prog);
}

void ForgeWindow::primitiveLoaderEnd() {
	QTimer* hidetimer = new QTimer;
	connect (hidetimer, SIGNAL (timeout()), m_primLoaderWidget, SLOT (hide()));
	hidetimer->setSingleShot (true);
	hidetimer->start (1500);
	m_primLoaderBar->setFormat( tr( "Done" ));
	log (tr ("Primitives scanned: %1 primitives listed"), m_primLoaderBar->value());
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::save (LDOpenFile* f, bool saveAs) {
	str path = f->name();
	
	if (path.length() == 0 || saveAs) {
		path = QFileDialog::getSaveFileName (g_win, tr ("Save As"),
			LDOpenFile::current()->name(), tr ("LDraw files (*.dat *.ldr)"));
		
		if (path.length() == 0) {
			// User didn't give a file name. This happens if the user cancelled
			// saving in the save file dialog. Abort.
			return;
		}
	}
	
	if (f->save (path)) {
		f->setName (path);
		
		if (f == LDOpenFile::current())
			g_win->updateTitle();
		
		log ("Saved to %1.", path);
		
		// Add it to recent files
		addRecentFile (path);
	} else {
		str message = fmt (tr ("Failed to save to %1: %2"), path, strerror (errno));
		
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

void ForgeWindow::addMessage (str msg) {
	m_msglog->addLine (msg);
}

// ============================================================================
void ObjectList::contextMenuEvent (QContextMenuEvent* ev) {
	g_win->spawnContextMenu (ev->globalPos());
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
QPixmap getIcon (str iconName) {
	return (QPixmap (fmt (":/icons/%1.png", iconName)));
}

// =============================================================================
bool confirm (str msg) {
	return confirm (ForgeWindow::tr ("Confirm"), msg);
}

bool confirm (str title, str msg) {
	return QMessageBox::question (g_win, title, msg,
		(QMessageBox::Yes | QMessageBox::No), QMessageBox::No) == QMessageBox::Yes;
}

// =============================================================================
void critical (str msg) {
	QMessageBox::critical (g_win, ForgeWindow::tr("Error"), msg,
		(QMessageBox::Close), QMessageBox::Close);
}

// =============================================================================
QIcon makeColorIcon (LDColor* colinfo, const ushort size) {
	// Create an image object and link a painter to it.
	QImage img (size, size, QImage::Format_ARGB32);
	QPainter paint (&img);
	
	QColor col = colinfo->faceColor;
	if (colinfo->index == maincolor) {
		// Use the user preferences for main color here
		col = gl_maincolor.value;
		col.setAlphaF (gl_maincolor_alpha);
	}
	
	// Paint the icon
	paint.fillRect (QRect (0, 0, size, size), colinfo->edgeColor);
	paint.drawPixmap (QRect (1, 1, size - 2, size - 2), getIcon ("checkerboard"), QRect (0, 0, 8, 8));
	paint.fillRect (QRect (1, 1, size - 2, size - 2), col);
	return QIcon (QPixmap::fromImage (img));
}

// =============================================================================
void makeColorSelector (QComboBox* box) {
	std::map<short, ulong> counts;
	
	for (LDObject* obj : LDOpenFile::current()->objs()) {
		if (!obj->isColored())
			continue;
		
		if (counts.find (obj->color()) == counts.end())
			counts[obj->color()] = 1;
		else
			counts[obj->color()]++;
	}
	
	box->clear();
	ulong row = 0;
	for (const auto& pair : counts) {
		LDColor* col = getColor (pair.first);
		assert (col != null);
		
		QIcon ico = makeColorIcon (col, 16);
		box->addItem (ico, fmt ("[%1] %2 (%3 object%4)",
			pair.first, col->name, pair.second, plural (pair.second)));
		box->setItemData (row, pair.first);
		
		++row;
	}
}

void ForgeWindow::setStatusBarText (str text) {
	statusBar()->showMessage (text);
}

void ForgeWindow::clearSelection() {
	m_sel.clear();
}

Ui_LDForgeUI* ForgeWindow::interface() const {
	return ui;
}

#define act(N) QAction* ForgeWindow::action##N() { return ui->action##N; }
#include "actions.h"

void ForgeWindow::updateFileList() {
	ui->fileList->clear();
	
	for (LDOpenFile* f : g_loadedFiles) {
		/*
		if (f->implicit())
			continue;
		*/
		
		ui->fileList->addItem ("");
		QListWidgetItem* item = ui->fileList->item (ui->fileList->count() - 1);
		f->setListItem (item);
		
		updateFileListItem (f);
	}
}

void ForgeWindow::updateFileListItem (LDOpenFile* f) {
	if (f->listItem() == null) {
		// We don't have a list item for this file, so the list
		// doesn't exist yet. Create it - afterwards this will be
		// up to date.
		updateFileList();
		return;
	}
	
	str name;
	if (f->name() == "")
		name = "<anonymous>";
	else
		name = basename (f->name());
	
	if (f == LDOpenFile::current())
		ui->fileList->setCurrentItem (f->listItem());
	
	if (f->implicit())
		f->listItem()->setForeground (QColor (96, 96, 96));
	
	f->listItem()->setText (name);
	f->listItem()->setIcon (f->hasUnsavedChanges() ? getIcon ("file-save") : QIcon());
}

void ForgeWindow::beginAction (QAction* act) {
	// Open the history so we can record the edits done during this action.
	if (act != ACTION (Undo) && act != ACTION (Redo) && act != ACTION (Open))
		LDOpenFile::current()->openHistory();
}

void ForgeWindow::endAction() {
	// Close the history now.
	LDOpenFile::current()->closeHistory();
	updateFileListItem (LDOpenFile::current());
}

void ForgeWindow::changeCurrentFile() {
	LDOpenFile* f = null;
	QListWidgetItem* item = ui->fileList->currentItem();
	
	for (LDOpenFile* it : g_loadedFiles) {
		if (it->listItem() == item) {
			f = it;
			break;
		}
	}
	
	if (!f || f == LDOpenFile::current())
		return;
	
	clearSelection();
	LDOpenFile::setCurrent (f);
	
	log ("Changed file to %1", basename (f->name()));
	
	R()->setFile (f);
	R()->update();
	buildObjList();
}

void ForgeWindow::refreshObjectList() {
#if 0
	ui->objectList->clear();
	LDOpenFile* f = LDOpenFile::current();
	
	for (LDObject* obj : *f)
		ui->objectList->addItem (obj->qObjListEntry);
#endif
	
	buildObjList();
}

QImage imageFromScreencap (uchar* data, ushort w, ushort h) {
	// GL and Qt formats have R and B swapped. Also, GL flips Y - correct it as well.
	return QImage (data, w, h, QImage::Format_ARGB32).rgbSwapped().mirrored();
}