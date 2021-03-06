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
#include "glShared.h"

class QMdiSubWindow;
class QToolButton;
class Canvas;
class Toolset;
class PrimitiveManager;
class Grid;
class DocumentManager;
class LDDocument;

// LDForge's main GUI class.
class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget* parent = nullptr, Qt::WindowFlags flags = 0);
	~MainWindow();

	void applyToActions(function<void(QAction*)> function);
	void changeDocument (LDDocument* f);
	void clearSelection();
	void createBlankDocument();
	Canvas* createCameraForDocument(LDDocument* document, gl::CameraType cameraType);
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
	void loadShortcuts();
	void openDocumentForEditing(LDDocument* document);
	PrimitiveManager* primitives();
	Canvas* renderer();
	void refresh();
	void replaceSelection(const QItemSelection& selection);
	CircularSection circleToolSection() const;
	bool save (LDDocument* doc, bool saveAs);
	void select(const QModelIndex& objectIndex);
	Canvas* selectCameraForDocument(LDDocument* document, gl::CameraType cameraType);
	QModelIndexList selectedIndexes() const;
	QSet<LDObject*> selectedObjects() const;
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

signals:
	void gridChanged();

public slots:
	void actionTriggered();
	void closeTab (int tabindex);
	void historyTraversed();
	void tabSelected();
	void documentClosed(LDDocument* document);
	void updateTitle();
	void newDocument (LDDocument* document, bool cache = false);
	void settingsChanged();
	void canvasClosed();

protected:
	void closeEvent (QCloseEvent* event);

private:
	struct ToolInfo;

	QMap<LDDocument*, QStack<Canvas*>> m_renderers;
	QMap<LDDocument*, QItemSelectionModel*> m_selectionModels;
	PrimitiveManager* m_primitives;
	Grid* m_grid;
	QVector<QToolButton*>	m_colorButtons;
	QVector<QAction*> m_recentFiles;
	class Ui_MainWindow& ui;
	QTabBar* m_tabs;
	bool m_updatingTabs;
	QVector<Toolset*> m_toolsets;
	QMap<QAction*, ToolInfo> m_toolmap;
	class ExtProgramToolset* m_externalPrograms;
	DocumentManager* m_documents;
	LDDocument* m_currentDocument;
	QMap<QAction*, QKeySequence> m_defaultShortcuts;
	QMap<Canvas*, QMdiSubWindow*> m_subWindows;
	int previousDivisions = MediumResolution;

private slots:
	void finishInitialization();
	void recentFileClicked();
	void canvasActivated(QMdiSubWindow* window);
	void mainModelLoaded(LDDocument* document);
};
