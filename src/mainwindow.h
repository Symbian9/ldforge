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
#include <QTreeWidget>
#include <QMetaMethod>
#include "ldObject.h"
#include "colors.h"
#include "configurationvaluebag.h"

class MessageManager;
class MainWindow;
class QToolButton;
class QDialogButtonBox;
class GLRenderer;
class QComboBox;
class QProgressBar;
struct Primitive;
class Toolset;

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
	void updateDocumentListItem (LDDocument* doc);

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
	bool save (LDDocument* doc, bool saveAs);

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
	QTreeWidget* getPrimitivesTree() const;
	static QKeySequence defaultShortcut (QAction* act);
	void loadShortcuts();
	void saveShortcuts();
	void applyToActions (std::function<void(QAction*)> function);

	bool ringToolHiRes() const;
	int ringToolSegments() const;
	ConfigurationValueBag* configBag() { return &m_configOptions; }
	class QSettings* makeSettings (QObject* parent = nullptr);
	void syncSettings();
	QVariant getConfigValue (QString name);
	class QSettings* getSettings() { return m_settings; }

	class ExtProgramToolset* externalPrograms()
	{
		return m_externalPrograms;
	}

	class GuiUtilities* guiUtilities()
	{
		return m_guiUtilities;
	}

public slots:
	void updatePrimitives();
	void changeCurrentFile();
	void closeTab (int tabindex);
	void ringToolHiResClicked (bool clicked);
	void circleToolSegmentsChanged();
	void slot_action();

protected:
	void closeEvent (QCloseEvent* ev);

private:
	struct ToolInfo { QMetaMethod method; Toolset* object; };

	ConfigurationValueBag m_configOptions;
	class GuiUtilities* m_guiUtilities;
	GLRenderer*			m_renderer;
	LDObjectList		m_sel;
	QList<LDQuickColor>	m_quickColors;
	QList<QToolButton*>	m_colorButtons;
	QList<QAction*>		m_recentFiles;
	MessageManager*		m_msglog;
	class Ui_MainWindow& ui;
	QTabBar*			m_tabs;
	bool				m_updatingTabs;
	QVector<Toolset*>	m_toolsets;
	QMap<QAction*, ToolInfo> m_toolmap;
	class ExtProgramToolset* m_externalPrograms;
	class QSettings* m_settings;

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
