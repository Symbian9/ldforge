/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2018 Teemu Piippo
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

#include <QMessageBox>
#include <QContextMenuEvent>
#include <QToolButton>
#include <QFileDialog>
#include <QPushButton>
#include <QSettings>
#include "generics/reverse.h"
#include "main.h"
#include "canvas.h"
#include "mainwindow.h"
#include "lddocument.h"
#include "ui_mainwindow.h"
#include "primitives.h"
#include "editmodes/abstractEditMode.h"
#include "toolsets/toolset.h"
#include "dialogs/configdialog.h"
#include "linetypes/comment.h"
#include "guiutilities.h"
#include "glcompiler.h"
#include "documentmanager.h"
#include "ldobjectiterator.h"
#include "grid.h"
#include "editHistory.h"

struct MainWindow::ToolInfo
{
	QMetaMethod method;
	Toolset* object;
};

// ---------------------------------------------------------------------------------------------------------------------
//
MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags flags) :
	QMainWindow (parent, flags),
	m_guiUtilities (new GuiUtilities (this)),
	m_primitives(new PrimitiveManager(this)),
	m_grid(new Grid(this)),
	ui (*new Ui_MainWindow),
	m_externalPrograms (nullptr),
	m_documents (new DocumentManager (this)),
	m_currentDocument (nullptr)
{
	connect(
		&singleton<Printer>(),
		&Printer::linePrinted,
		[&](const QString& line)
		{
			this->statusBar()->showMessage(line, 5000);
		}
	);

	ui.setupUi (this);
	this->restoreGeometry(config::mainWindowGeometry());
	this->restoreState(config::mainWindowState());

	if (config::mainSplitterState().isEmpty())
		this->ui.splitter->setSizes({this->width() * 2 / 3, this->width() / 3});
	else
		this->ui.splitter->restoreState(config::mainSplitterState());

	m_updatingTabs = false;
	m_tabs = new QTabBar;
	m_tabs->setTabsClosable (true);
	ui.verticalLayout->insertWidget (0, m_tabs);
	ui.primitives->setModel(m_primitives);
	createBlankDocument();
	ui.rendererStack->setCurrentWidget(getRendererForDocument(m_currentDocument));

	connect (m_tabs, SIGNAL (currentChanged(int)), this, SLOT (tabSelected()));
	connect (m_tabs, SIGNAL (tabCloseRequested (int)), this, SLOT (closeTab (int)));
	connect(m_documents, &DocumentManager::documentCreated, this, &MainWindow::newDocument);
	connect(m_documents, SIGNAL(documentClosed(LDDocument*)), this, SLOT(documentClosed(LDDocument*)));

	m_quickColors = m_guiUtilities->loadQuickColorList();
	updateActions();

	// Connect all actions and save default sequences
	applyToActions([&](QAction* action)
	{
		connect(action, SIGNAL (triggered()), this, SLOT (actionTriggered()));
		m_defaultShortcuts[action] = action->shortcut();
	});
	connect(
		ui.header,
		&HeaderEdit::descriptionChanged,
		this,
		&MainWindow::updateTitle
	);

	updateGridToolBar();
	updateEditModeActions();
	updateRecentFilesMenu();
	updateColorToolbar();
	updateTitle();
	loadShortcuts();
	setMinimumSize (300, 200);
	connect(ui.circleToolSection, &CircularSectionEditor::sectionChanged, [&](){this->renderer()->update();});

	// Examine the toolsets and make a dictionary of tools
	m_toolsets = Toolset::createToolsets (this);

	QSet<QString> ignore;
	for (int i = 0; i < Toolset::staticMetaObject.methodCount(); ++i)
	{
		QMetaMethod method = Toolset::staticMetaObject.method (i);
		ignore.insert(QString::fromUtf8(method.name()));
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

	// If this is the first start, get the user to configuration. Especially point
	// them to the profile tab, it's the most important form to fill in.
	if (config::firstStart())
	{
		ConfigDialog* dialog = new ConfigDialog (this, ConfigDialog::ProfileTab);
		dialog->show();
		config::setFirstStart (false);
	}

	QMetaObject::invokeMethod (this, "finishInitialization", Qt::QueuedConnection);

	connect(
		this->ui.objectList,
		&QListView::doubleClicked,
		[&](const QModelIndex& index)
		{
			LDObject* object = currentDocument()->lookup(index);

			if (object)
				::editObject(this, object);
		}
	);
}

void MainWindow::finishInitialization()
{
	m_primitives->loadPrimitives();
}

MainWindow::~MainWindow()
{
	delete m_guiUtilities;
	delete m_primitives;
	delete m_grid;
	delete &ui;

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
void MainWindow::updateRecentFilesMenu()
{
	// First, clear any items in the recent files menu
	for (QAction* recent : m_recentFiles)
		delete recent;

	m_recentFiles.clear();

	QAction* first = nullptr;

	for (const QVariant& it : config::recentFiles())
	{
		QString file = it.toString();
		QAction* recent = new QAction (getIcon ("open-recent"), file, this);
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
			QToolButton* colorButton = new QToolButton {this};
			colorButton->setIcon(makeColorIcon(entry.color(), 16));
			colorButton->setIconSize({16, 16});
			colorButton->setToolTip(entry.color().name());
			colorButton->setStatusTip(format(
				tr("Changes the color of selected objects to %1"),
				entry.color().name()
			));
			ui.toolBarColors->addWidget (colorButton);
			m_colorButtons << colorButton;
			connect(colorButton,
				&QToolButton::clicked,
				[&]()
				{
					for (LDObject* object : selectedObjects())
					{
						if (object->isColored())
							object->setColor(entry.color());
					}

					endAction();
				}
			);
		}
	}

	updateGridToolBar();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::updateGridToolBar()
{
	// Ensure that the current grid - and only the current grid - is selected.
	int grid = config::grid();
	ui.actionGridCoarse->setChecked (grid == Grid::Coarse);
	ui.actionGridMedium->setChecked (grid == Grid::Medium);
	ui.actionGridFine->setChecked (grid == Grid::Fine);
	ui.actionPolarGrid->setChecked(m_grid->type() == Grid::Polar);
	emit gridChanged();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::updateTitle()
{
	QString title = APPNAME " " VERSION_STRING;

	// Append our current file if we have one
	if (m_currentDocument)
	{
		title += ": ";
		title += m_currentDocument->getDisplayName();

		if (not m_currentDocument->header.description.isEmpty())
			title += format (": %1", m_currentDocument->header.description);

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
	int count = 0;
	QModelIndexList thingsToRemove = ui.objectList->selectionModel()->selectedIndexes();

	for (QModelIndex index : reverse(thingsToRemove))
	{
		if (m_currentDocument->hasIndex(index.row(), index.column()))
		{
			m_currentDocument->removeAt(index);
			count += 1;
		}
	}

	return count;
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
// Returns the suggested position to place a new object at.
//
int MainWindow::suggestInsertPoint()
{
	// If we have a selection, put the item after it.
	// TODO: fix this properly!
	if (not selectedIndexes().isEmpty())
		return (*(selectedIndexes().end() - 1)).row() + 1;

	// Otherwise place the object at the end.
	return m_currentDocument->size();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::doFullRefresh()
{
	renderer()->update();
}

// ---------------------------------------------------------------------------------------------------------------------
//
// Builds the object list and tells the GL renderer to do a soft update.
//
void MainWindow::refresh()
{
	renderer()->update();
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
			return LDColor::nullColor; // No consensus in object color

		if (not result.isValid())
			result = obj->color();
	}

	return result;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::closeEvent(QCloseEvent* event)
{
	// Check whether it's safe to close all files.
	if (m_documents->isSafeToCloseAll())
	{
		// Store the state of the main window before closing.
		config::setMainWindowGeometry(this->saveGeometry());
		config::setMainWindowState(this->saveState());
		config::setMainSplitterState(this->ui.splitter->saveState());
		settingsObject().sync();
		event->accept();
	}
	else
	{
		event->ignore();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::spawnContextMenu (const QPoint& position)
{
	const bool single = (countof(selectedObjects()) == 1);
	LDObject* singleObj = single ? *selectedObjects().begin() : nullptr;

	bool hasSubfiles = false;

	for (LDObject* obj : selectedObjects())
	{
		if (obj->type() == LDObjectType::SubfileReference)
		{
			hasSubfiles = true;
			break;
		}
	}

	QMenu* contextMenu = new QMenu;

	if (single and singleObj->type() != LDObjectType::Empty)
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

	if (renderer()->camera() != Camera::Free)
	{
		contextMenu->addSeparator();
		contextMenu->addAction(ui.actionSetDrawPlane);
		contextMenu->addAction(ui.actionClearDrawPlane);
		contextMenu->addAction(ui.actionSetCullDepth);
		contextMenu->addAction(ui.actionClearCullDepth);
	}

	contextMenu->exec(position);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::deleteByColor (LDColor color)
{
	QVector<LDObject*> unwanted;

	for (LDObject* object : m_currentDocument->objects())
	{
		if (not object->isColored() or object->color() != color)
			continue;

		unwanted.append(object);
	}

	for (LDObject* obj : unwanted)
		m_currentDocument->remove(obj);
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
bool MainWindow::save (LDDocument* doc, bool saveAs)
{
	if (doc->isFrozen())
		return false;

	QString path = doc->fullPath();
	qint64 savesize;

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
	saveAsBtn->setIcon (getIcon ("file-save-as"));
	dlg.addButton (saveAsBtn, QMessageBox::ActionRole);
	dlg.setDefaultButton (QMessageBox::Close);
	dlg.exec();

	if (dlg.clickedButton() == saveAsBtn)
		return save (doc, true); // yay recursion!

	return false;
}

/*
 * Returns an icon from built-in resources.
 */
QPixmap MainWindow::getIcon(QString iconName)
{
	return {format(":/icons/%1.png", iconName)};
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::updateDocumentList()
{
	m_updatingTabs = true;

	while (m_tabs->count() > 0)
		m_tabs->removeTab (0);

	for (auto& document : *m_documents)
	{
		if (not document->isFrozen())
		{
			// Add an item to the list for this file and store the tab index
			// in the document so we can find documents by tab index.
			document->setTabIndex (m_tabs->addTab (""));
			updateDocumentListItem(document.get());
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
	m_tabs->setTabIcon (doc->tabIndex(), doc->hasUnsavedChanges() ? getIcon ("file-save") : QIcon());
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
	for (auto& document : *m_documents)
	{
		if (not document->isFrozen() and document->tabIndex() == tabIndex)
		{
			switchee = document.get();
			break;
		}
	}

	if (switchee and switchee != m_currentDocument)
		changeDocument (switchee);
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

	ui.actionWireframe->setChecked (config::drawWireframe());
	ui.actionAxes->setChecked (config::drawAxes());
	ui.actionBfcView->setChecked (config::bfcRedGreenView());
	ui.actionRandomColors->setChecked (config::randomColors());
	ui.actionDrawAngles->setChecked (config::drawAngles());
	ui.actionDrawSurfaces->setChecked (config::drawSurfaces());
	ui.actionDrawEdgeLines->setChecked (config::drawEdgeLines());
	ui.actionDrawConditionalLines->setChecked (config::drawConditionalLines());
	ui.actionLighting->setChecked(config::lighting());
}

// ---------------------------------------------------------------------------------------------------------------------
//
PrimitiveManager* MainWindow::primitives()
{
	return m_primitives;
}

// ---------------------------------------------------------------------------------------------------------------------
//
Canvas* MainWindow::renderer()
{
	Q_ASSERT(ui.rendererStack->count() > 0);
	return static_cast<Canvas*>(ui.rendererStack->currentWidget());
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::setQuickColors (const QVector<ColorToolbarItem>& colors)
{
	m_quickColors = colors;
	updateColorToolbar();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::closeTab (int tabindex)
{
	auto iterator = m_documents->findDocumentByName(m_tabs->tabData (tabindex).toString());

	if (iterator != m_documents->end())
		(*iterator)->close();
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
		QKeySequence seq = settingsObject().value("shortcut_" + act->objectName(), act->shortcut()).value<QKeySequence>();
		act->setShortcut (seq);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::saveShortcuts()
{
	applyToActions([&](QAction* action)
	{
		QString const key = "shortcut_" + action->objectName();

		if (m_defaultShortcuts[action] != action->shortcut())
			settingsObject().setValue(key, action->shortcut());
		else
			settingsObject().remove(key);
	});
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::applyToActions(function<void(QAction*)> function)
{
	for (QAction* act : findChildren<QAction*>())
	{
		if (not act->objectName().isEmpty())
			function (act);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
//
QKeySequence MainWindow::defaultShortcut (QAction* act)
{
	return m_defaultShortcuts[act];
}

CircularSection MainWindow::circleToolSection() const
{
	return ui.circleToolSection->section();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::createBlankDocument()
{
	// Create a new anonymous file and set it to our current
	LDDocument* document = m_documents->createNew(false);
	document->setName ("");
	document->setFrozen(false);
	changeDocument (document);
	updateActions();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::newDocument(LDDocument* document, bool cache)
{
	connect (document->history(), SIGNAL (undone()), this, SLOT (historyTraversed()));
	connect (document->history(), SIGNAL (redone()), this, SLOT (historyTraversed()));
	connect (document->history(), SIGNAL (stepAdded()), this, SLOT (updateActions()));

	if (not cache)
		openDocumentForEditing(document);
}

void MainWindow::openDocumentForEditing(LDDocument* document)
{
	if (document->isFrozen())
	{
		document->setFrozen(false);
		print ("Opened %1", document->name());
		Canvas* canvas = getRendererForDocument(document);
		updateDocumentList();
		connect(document, &LDDocument::windingChanged, canvas, &Canvas::fullUpdate);
	}
}

/*
 * Returns the currently open document.
 */
LDDocument* MainWindow::currentDocument()
{
	return m_currentDocument;
}

/*
 * Changes the current document to the specified document.
 */
void MainWindow::changeDocument(LDDocument* document)
{
	Q_ASSERT(document != nullptr);

	// Implicit files were loaded for caching purposes and may never be switched to.
	if (document->isFrozen())
		return;

	m_currentDocument = document;
	Canvas* renderer = getRendererForDocument(document);
	ui.rendererStack->setCurrentWidget(renderer);

	if (document)
	{
		// A ton of stuff needs to be updated
		updateDocumentListItem (document);
		updateTitle();
		print ("Changed document to %1", document->getDisplayName());
		ui.objectList->setModel(document);
		ui.header->setDocument(document);
		renderer->fullUpdate();
		QItemSelectionModel* selection = m_selections.value(document);

		if (selection == nullptr)
		{
			m_selections[document] = ui.objectList->selectionModel();
			renderer->setSelectionModel(m_selections[document]);
		}
		else
		{
			ui.objectList->setSelectionModel(selection);
		}
	}
}

/*
 * Returns the associated renderer for the given document
 */
Canvas* MainWindow::getRendererForDocument(LDDocument *document)
{
	Canvas* renderer = m_renderers.value(document);

	if (not renderer)
	{
		renderer = new Canvas {document, this};
		m_renderers[document] = renderer;
		ui.rendererStack->addWidget(renderer);
	}

	return renderer;
}

void MainWindow::documentClosed(LDDocument *document)
{
	print ("Closed %1", document->name());
	updateDocumentList();

	// If the current document just became implicit (i.e. user closed it), we need to get a new one to show.
	if (currentDocument() == document)
		currentDocumentClosed();

	Canvas* renderer = m_renderers.value(document);

	if (renderer)
	{
		ui.rendererStack->removeWidget(renderer);
		renderer->deleteLater();
	}

	m_renderers.remove(document);
}

QModelIndexList MainWindow::selectedIndexes() const
{
	return this->ui.objectList->selectionModel()->selectedIndexes();
}

// ---------------------------------------------------------------------------------------------------------------------
//
QSet<LDObject*> MainWindow::selectedObjects() const
{
	QSet<LDObject*> result;

	for (const QModelIndex& index : this->selectedIndexes())
		result.insert(m_currentDocument->lookup(index));

	return result;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MainWindow::currentDocumentClosed()
{
	LDDocument* old = currentDocument();

	// Find a replacement document to use
	for (auto &iterator : m_documents->allDocuments())
	{
		if (iterator.get() != old and not iterator->isFrozen())
		{
			changeDocument(iterator.get());
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

Grid* MainWindow::grid()
{
	return m_grid;
}

void MainWindow::clearSelection()
{
	m_selections[m_currentDocument]->clear();
}

void MainWindow::select(const QModelIndex &objectIndex)
{
	if (objectIndex.isValid() and objectIndex.model() == m_currentDocument)
		m_selections[m_currentDocument]->select(objectIndex, QItemSelectionModel::Select);
}

QItemSelectionModel* MainWindow::currentSelectionModel()
{
	return m_selections[m_currentDocument];
}

void MainWindow::replaceSelection(const QItemSelection& selection)
{
	m_selections[m_currentDocument]->select(selection, QItemSelectionModel::ClearAndSelect);
}

// ---------------------------------------------------------------------------------------------------------------------
//
ColorToolbarItem::ColorToolbarItem(LDColor color) :
	m_color (color) {}

ColorToolbarItem ColorToolbarItem::makeSeparator()
{
	return {LDColor::nullColor};
}

bool ColorToolbarItem::isSeparator() const
{
	return color() == LDColor::nullColor;
}

LDColor ColorToolbarItem::color() const
{
	return m_color;
}

void ColorToolbarItem::setColor (LDColor color)
{
	m_color = color;
}
