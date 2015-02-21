/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2015 Teemu Piippo
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

#pragma once
#include <functional>
#include <QMainWindow>
#include <QAction>
#include <QListWidget>
#include <QRadioButton>
#include "configuration.h"
#include "ldObject.h"
#include "ui_ldforge.h"
#include "colors.h"

class MessageManager;
class MainWindow;
class LDColorData;
class QToolButton;
class QDialogButtonBox;
class GLRenderer;
class QComboBox;
class QProgressBar;
class Ui_LDForgeUI;
class Primitive;

// Stuff for dialogs
#define IMPLEMENT_DIALOG_BUTTONS \
	bbx_buttons = new QDialogButtonBox (QDialogButtonBox::Ok | QDialogButtonBox::Cancel); \
	connect (bbx_buttons, SIGNAL (accepted()), this, SLOT (accept())); \
	connect (bbx_buttons, SIGNAL (rejected()), this, SLOT (reject())); \

class LDQuickColor
{
	PROPERTY (public,	LDColor,		color,		setColor,		STOCK_WRITE)
	PROPERTY (public,	QToolButton*,	toolButton,	setToolButton,	STOCK_WRITE)

	public:
		LDQuickColor (LDColor color, QToolButton* toolButton);
		bool isSeparator() const;

		static LDQuickColor getSeparator();
};

//
// Object list class for MainWindow
//
class ObjectList : public QListWidget
{
	Q_OBJECT

	protected:
		void contextMenuEvent (QContextMenuEvent* ev);
};

//
// LDForge's main GUI class.
//
class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow (QWidget* parent = null, Qt::WindowFlags flags = 0);
	~MainWindow();

	// Rebuilds the object list.
	void buildObjList();

	// Updates the window title.
	void updateTitle();

	// Builds the object list and tells the GL renderer to init a full
	// refresh.
	void doFullRefresh();

	// Builds the object list and tells the GL renderer to do a soft update.
	void refresh();

	// Returns the suggested position to place a new object at.
	int getInsertionPoint();

	// Updates the quick color toolbar
	void updateColorToolbar();

	// Rebuilds the recent files submenu
	void updateRecentFilesMenu();

	// Sets the selection based on what's selected in the object list.
	void updateSelection();

	// Updates the grids, selects the selected grid and deselects others.
	void updateGridToolBar();

	// Updates the edit modes, current one is selected and others are deselected.
	void updateEditModeActions();

	// Rebuilds the document tab list.
	void updateDocumentList();

	// Updates the document tab for \c doc. If no such tab exists, the
	// document list is rebuilt instead.
	void updateDocumentListItem (LDDocumentPtr doc);

	// Returns the uniform selected color (i.e. 4 if everything selected is
	// red), -1 if there is no such consensus.
	LDColor getSelectedColor();

	// Automatically scrolls the object list so that it points to the first
	// selected object.
	void scrollToSelection();

	// Spawns the context menu at the given position.
	void spawnContextMenu (const QPoint pos);

	// Deletes all selected objects, returns the count of deleted objects.
	int deleteSelection();

	// Deletes all objects by the given color number.
	void deleteByColor (LDColor color);

	// Tries to save the given document.
	bool save (LDDocumentPtr doc, bool saveAs);

	// Updates various actions, undo/redo are set enabled/disabled where
	// appropriate, togglable actions are updated based on configuration,
	// etc.
	void updateActions();

	// Returns a pointer to the renderer
	inline GLRenderer* R()
	{
		return m_renderer;
	}

	// Sets the quick color list to the given list of colors.
	inline void setQuickColors (const QList<LDQuickColor>& colors)
	{
		m_quickColors = colors;
		updateColorToolbar();
	}

	// Adds a message to the renderer's message manager.
	void addMessage (QString msg);

	// Updates the object list. Right now this just rebuilds it.
	void refreshObjectList();

	void endAction();

	inline QTreeWidget* getPrimitivesTree() const
	{
		return ui->primitives;
	}

	static QKeySequence defaultShortcut (QAction* act);
	void loadShortcuts (QSettings const* settings);
	void saveShortcuts (QSettings* settings);
	void applyToActions (std::function<void(QAction*)> function);

	bool ringToolHiRes() const;
	int ringToolSegments() const;

public slots:
	void updatePrimitives();
	void changeCurrentFile();
	void closeTab (int tabindex);
	void ringToolHiResClicked (bool clicked);
	void circleToolSegmentsChanged();
	void actionTriggered();
	void actionNew();
	void actionNewFile();
	void actionOpen();
	void actionDownloadFrom();
	void actionSave();
	void actionSaveAs();
	void actionSaveAll();
	void actionClose();
	void actionCloseAll();
	void actionInsertFrom();
	void actionExportTo();
	void actionSettings();
	void actionSetLDrawPath();
	void actionScanPrimitives();
	void actionExit();
	void actionResetView();
	void actionAxes();
	void actionWireframe();
	void actionBFCView();
	void actionSetOverlay();
	void actionClearOverlay();
	void actionScreenshot();
	void actionInsertRaw();
	void actionNewSubfile();
	void actionNewLine();
	void actionNewTriangle();
	void actionNewQuad();
	void actionNewCLine();
	void actionNewComment();
	void actionNewBFC();
	void actionUndo();
	void actionRedo();
	void actionCut();
	void actionCopy();
	void actionPaste();
	void actionDelete();
	void actionSelectAll();
	void actionSelectByColor();
	void actionSelectByType();
	void actionModeDraw();
	void actionModeSelect();
	void actionModeRectangle();
	void actionModeCircle();
	void actionModeMagicWand();
	void actionModeLinePath();
	void actionSetDrawDepth();
	void actionSetColor();
	void actionAutocolor();
	void actionUncolor();
	void actionInline();
	void actionInlineDeep();
	void actionInvert();
	void actionMakePrimitive();
	void actionSplitQuads();
	void actionEditRaw();
	void actionBorders();
	void actionRoundCoordinates();
	void actionVisibilityHide();
	void actionVisibilityReveal();
	void actionVisibilityToggle();
	void actionReplaceCoords();
	void actionFlip();
	void actionDemote();
	void actionYtruder();
	void actionRectifier();
	void actionIntersector();
	void actionIsecalc();
	void actionCoverer();
	void actionEdger2();
	void actionHelp();
	void actionAbout();
	void actionAboutQt();
	void actionGridCoarse();
	void actionGridMedium();
	void actionGridFine();
	void actionEdit();
	void actionMoveUp();
	void actionMoveDown();
	void actionMoveXNeg();
	void actionMoveXPos();
	void actionMoveYNeg();
	void actionMoveYPos();
	void actionMoveZNeg();
	void actionMoveZPos();
	void actionRotateXNeg();
	void actionRotateXPos();
	void actionRotateYNeg();
	void actionRotateYPos();
	void actionRotateZNeg();
	void actionRotateZPos();
	void actionRotationPoint();
	void actionAddHistoryLine();
	void actionJumpTo();
	void actionSubfileSelection();
	void actionDrawAngles();
	void actionRandomColors();
	void actionOpenSubfiles();
	void actionSplitLines();
	void actionDrawSurfaces();
	void actionDrawEdgeLines();
	void actionDrawConditionalLines();

protected:
	void closeEvent (QCloseEvent* ev);

private:
	GLRenderer*			m_renderer;
	LDObjectList		m_selection;
	QList<LDQuickColor>	m_quickColors;
	QList<QToolButton*>	m_colorButtons;
	QList<QAction*>		m_recentFiles;
	MessageManager*		m_messageLog;
	Ui_LDForgeUI*		ui;
	QTabBar*			m_tabBar;
	bool				m_isUpdatingTabs;

private slots:
	void slot_selectionChanged();
	void slot_recentFile();
	void slot_quickColor();
	void slot_lastSecondCleanup();
	void slot_editObject (QListWidgetItem* listitem);
};

//! Pointer to the instance of MainWindow.
extern MainWindow* g_win;

//! Get an icon by name from the resources directory.
QPixmap GetIcon (QString iconName);

//! \returns a list of quick colors based on the configuration entry.
QList<LDQuickColor> LoadQuickColorList();

//! Asks the user a yes/no question with the given \c message and the given
//! window \c title.
//! \returns true if the user answered yes, false if no.
bool Confirm (const QString& title, const QString& message); // Generic confirm prompt

//! An overload of \c confirm(), this asks the user a yes/no question with the
//! given \c message.
//! \returns true if the user answered yes, false if no.
bool Confirm (const QString& message);

//! Displays an error prompt with the given \c message
void Critical (const QString& message);

//! Makes an icon of \c size x \c size pixels to represent \c colinfo
QIcon MakeColorIcon (LDColor colinfo, const int size);

//! Fills the given combo-box with color information
void MakeColorComboBox (QComboBox* box);

//! \returns a QImage from the given raw GL \c data
QImage GetImageFromScreencap (uchar* data, int w, int h);

//!
//! Takes in pairs of radio buttons and respective values and finds the first
//! selected one.
//! \returns returns the value of the first found radio button that was checked
//! \returns by the user.
//!
template<class T>
T RadioSwitch (const T& defval, QList<Pair<QRadioButton*, T>> haystack)
{
	for (Pair<QRadioButton*, const T&> i : haystack)
	{
		if (i.first->isChecked())
			return i.second;
	}

	return defval;
}

//!
//! Takes in pairs of radio buttons and respective values and checks the first
//! found radio button whose respsective value matches \c expr have the given value.
//!
template<class T>
void RadioDefault (const T& expr, QList<Pair<QRadioButton*, T>> haystack)
{
	for (Pair<QRadioButton*, const T&> i : haystack)
	{
		if (i.second == expr)
		{
			i.first->setChecked (true);
			return;
		}
	}
}

// =============================================================================
//
class SubfileListItem : public QTreeWidgetItem
{
	PROPERTY (public, Primitive*,	primitive, setPrimitive, STOCK_WRITE)

public:
	SubfileListItem (QTreeWidgetItem* parent, Primitive* info) :
		QTreeWidgetItem (parent),
		m_primitive (info) {}

	SubfileListItem (QTreeWidget* parent, Primitive* info) :
		QTreeWidgetItem (parent),
		m_primitive (info) {}
};

void PopulatePrimitives (QTreeWidget* tw, const QString& selectByDefault = QString());
