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
#include "doublemap.h"

class MessageManager;
class MainWindow;
class QToolButton;
class QDialogButtonBox;
class GLRenderer;
class QComboBox;
class QProgressBar;
class Toolset;
class Configuration;
class PrimitiveManager;
class Grid;
class MathFunctions;

class ColorToolbarItem
{
public:
	ColorToolbarItem (LDColor color, QToolButton* toolButton);
	LDColor color() const;
	bool isSeparator() const;
	void setColor (LDColor color);
	void setToolButton (QToolButton* value);
	QToolButton* toolButton() const;

	static ColorToolbarItem makeSeparator();

private:
	LDColor m_color;
	QToolButton* m_toolButton;
};

// Object list class for MainWindow
class ObjectList : public QListWidget
{
	Q_OBJECT

protected:
	void contextMenuEvent (QContextMenuEvent* ev);
};

// LDForge's main GUI class.
class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(Configuration& config, QWidget* parent = nullptr, Qt::WindowFlags flags = 0);
	~MainWindow();

	void addMessage (QString msg);
	void applyToActions (std::function<void(QAction*)> function);
	void buildObjectList();
	void changeDocument (LDDocument* f);
	void closeInitialDocument();
	Configuration* config();
	void createBlankDocument();
	LDDocument* currentDocument();
	void currentDocumentClosed();
	QKeySequence defaultShortcut (QAction* act);
	void deleteByColor (LDColor color);
	int deleteSelection();
	DocumentManager* documents() { return m_documents; }
	void doFullRefresh();
	void endAction();
	class ExtProgramToolset* externalPrograms();
	QVariant getConfigValue (QString name);
	QTreeWidget* getPrimitivesTree() const;
	class QSettings* getSettings() { return m_settings; }
	LDColor getUniformSelectedColor();
	Grid* grid();
	class GuiUtilities* guiUtilities();
	void loadShortcuts();
	MathFunctions* mathFunctions() const;
	LDDocument* newDocument (bool cache = false);
	PrimitiveManager* primitives();
	GLRenderer* renderer();
	void refresh();
	void refreshObjectList();
	bool ringToolHiRes() const;
	int ringToolSegments() const;
	bool save (LDDocument* doc, bool saveAs);
	void saveShortcuts();
	void scrollToSelection();
	const LDObjectList& selectedObjects();
	void setQuickColors (const QList<ColorToolbarItem>& colors);
	void spawnContextMenu (const QPoint pos);
	int suggestInsertPoint();
	void syncSettings();
	Q_SLOT void updateActions();
	void updateColorToolbar();
	void updateDocumentList();
	void updateDocumentListItem (LDDocument* doc);
	void updateEditModeActions();
	void updateGridToolBar();
	void updateRecentFilesMenu();
	void updateSelection();
	void updateTitle();

public slots:
	void actionTriggered();
	void circleToolSegmentsChanged();
	void closeTab (int tabindex);
	void historyTraversed();
	void ringToolHiResClicked (bool clicked);
	void tabSelected();
	void updatePrimitives();

protected:
	void closeEvent (QCloseEvent* ev);

private:
	struct ToolInfo { QMetaMethod method; Toolset* object; };

	Configuration& m_config;
	class GuiUtilities* m_guiUtilities;
	GLRenderer* m_renderer;
	PrimitiveManager* m_primitives;
	Grid* m_grid;
	MathFunctions* m_mathFunctions;
	LDObjectList m_sel;
	QList<ColorToolbarItem>	m_quickColors;
	QList<QToolButton*>	m_colorButtons;
	QList<QAction*> m_recentFiles;
	class Ui_MainWindow& ui;
	QTabBar* m_tabs;
	bool m_updatingTabs;
	QVector<Toolset*> m_toolsets;
	QMap<QAction*, ToolInfo> m_toolmap;
	class ExtProgramToolset* m_externalPrograms;
	class QSettings* m_settings;
	DocumentManager* m_documents;
	LDDocument* m_currentDocument;
	DoubleMap<LDObject*, QListWidgetItem*> m_objectsInList;
	bool m_isSelectionLocked;
	QMap<QAction*, QKeySequence> m_defaultShortcuts;

private slots:
	void finishInitialization();
	void selectionChanged();
	void recentFileClicked();
	void quickColorClicked();
	void doLastSecondCleanup();
	void objectListDoubleClicked (QListWidgetItem* listitem);
};

// Pointer to the instance of MainWindow.
// TODO: it's going out, slowly but surely.
extern MainWindow* g_win;

// Get an icon by name from the resources directory.
QPixmap GetIcon (QString iconName);

// Asks the user a yes/no question with the given message and the given window title.
// Returns true if the user answered yes, false if no.
bool Confirm (const QString& title, const QString& message); // Generic confirm prompt

// An overload of confirm(), this asks the user a yes/no question with the given message.
// Returns true if the user answered yes, false if no.
bool Confirm (const QString& message);

// Displays an error prompt with the given message
void Critical (const QString& message);
void errorPrompt (QWidget *parent, const QString& message);

// Takes in pairs of radio buttons and respective values and finds the first selected one.
// Returns returns the value of the first found radio button that was checked by the user.
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

// Takes in pairs of radio buttons and respective values and checks the first found radio button whose respsective value
// matches expr have the given value.
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

QSettings* makeSettings (QObject* parent = nullptr);
