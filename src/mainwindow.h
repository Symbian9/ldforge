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

#pragma once
#include <functional>
#include <QMainWindow>
#include <QAction>
#include <QListWidget>
#include <QRadioButton>
#include <QTreeWidget>
#include <QMetaMethod>
#include "linetypes/modelobject.h"
#include "colors.h"

class MessageManager;
class QToolButton;
class Canvas;
class Toolset;
class PrimitiveManager;
class Grid;
class MathFunctions;
class DocumentManager;
class LDDocument;

class ColorToolbarItem
{
public:
	ColorToolbarItem (LDColor color = LDColor{}, QToolButton* toolButton = nullptr);
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

// LDForge's main GUI class.
class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget* parent = nullptr, Qt::WindowFlags flags = 0);
	~MainWindow();

	void addMessage (QString msg);
	void applyToActions (std::function<void(QAction*)> function);
	void changeDocument (LDDocument* f);
	void clearSelection();
	void closeInitialDocument();
	void createBlankDocument();
	LDDocument* currentDocument();
	void currentDocumentClosed();
	QItemSelectionModel* currentSelectionModel();
	QKeySequence defaultShortcut (QAction* act);
	void deleteByColor (LDColor color);
	int deleteSelection();
	DocumentManager* documents() { return m_documents; }
	void doFullRefresh();
	void endAction();
	class ExtProgramToolset* externalPrograms();
	LDColor getUniformSelectedColor();
	Canvas* getRendererForDocument(LDDocument* document);
	Grid* grid();
	class GuiUtilities* guiUtilities();
	void loadShortcuts();
	MathFunctions* mathFunctions() const;
	MessageManager* messageLog() const;
	LDDocument* newDocument (bool cache = false);
	void openDocumentForEditing(LDDocument* document);
	PrimitiveManager* primitives();
	Canvas* renderer();
	void refresh();
	void replaceSelection(const QItemSelection& selection);
	bool ringToolHiRes() const;
	int ringToolSegments() const;
	bool save (LDDocument* doc, bool saveAs);
	void saveShortcuts();
	void select(const QModelIndex& objectIndex);
	QModelIndexList selectedIndexes() const;
	QSet<LDObject*> selectedObjects() const;
	void setQuickColors (const QVector<ColorToolbarItem> &colors);
	void spawnContextMenu (const QPoint& position);
	int suggestInsertPoint();
	Q_SLOT void updateActions();
	void updateColorToolbar();
	void updateDocumentList();
	void updateDocumentListItem (LDDocument* doc);
	void updateEditModeActions();
	void updateGridToolBar();
	void updateRecentFilesMenu();

	static QPixmap getIcon(QString iconName);

	template<typename... Args>
	void print(QString formatString, Args... args)
	{
		formatHelper(formatString, args...);
		addMessage(formatString);
	}

signals:
	void gridChanged();

public slots:
	void actionTriggered();
	void circleToolSegmentsChanged();
	void closeTab (int tabindex);
	void historyTraversed();
	void ringToolHiResClicked (bool clicked);
	void tabSelected();
	void documentClosed(LDDocument* document);
	void updateTitle();

protected:
	void closeEvent (QCloseEvent* ev);

private:
	struct ToolInfo;

	class GuiUtilities* m_guiUtilities;
	MessageManager* m_messageLog = nullptr;
	QMap<LDDocument*, Canvas*> m_renderers;
	QMap<LDDocument*, QItemSelectionModel*> m_selections;
	PrimitiveManager* m_primitives;
	Grid* m_grid;
	MathFunctions* m_mathFunctions;
	QVector<ColorToolbarItem>	m_quickColors;
	QList<QToolButton*>	m_colorButtons;
	QList<QAction*> m_recentFiles;
	class Ui_MainWindow& ui;
	QTabBar* m_tabs;
	bool m_updatingTabs;
	QVector<Toolset*> m_toolsets;
	QMap<QAction*, ToolInfo> m_toolmap;
	class ExtProgramToolset* m_externalPrograms;
	DocumentManager* m_documents;
	LDDocument* m_currentDocument;
	QMap<QAction*, QKeySequence> m_defaultShortcuts;

private slots:
	void finishInitialization();
	void recentFileClicked();
	void quickColorClicked();
};
