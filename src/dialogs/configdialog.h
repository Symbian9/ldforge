/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2017 Teemu Piippo
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
#include "../mainwindow.h"
#include "../toolsets/extprogramtoolset.h"
#include <QDialog>

// =============================================================================
class ShortcutListItem : public QListWidgetItem
{
public:
	explicit ShortcutListItem (QListWidget* view = nullptr, int type = Type);

	QAction* action() const;
	QKeySequence sequence() const;
	void setAction (QAction* action);
	void setSequence (const QKeySequence& sequence);

private:
	QAction* m_action;
	QKeySequence m_sequence;
};

struct ExternalProgramWidgets
{
	class QLineEdit* input;
	class QPushButton* setPathButton;
	class QCheckBox* wineBox;
};

// =============================================================================
class ConfigDialog : public QDialog, public HierarchyElement
{
	Q_OBJECT

public:
	enum Tab
	{
		InterfaceTab,
		EditingToolsTab,
		ProfileTab,
		ShortcutsTab,
		QuickColorsTab,
		GridsTab,
		ExtProgsTab,
		DownloadTab
	};

	explicit ConfigDialog (QWidget* parent = nullptr, Tab defaulttab = (Tab) 0, Qt::WindowFlags f = 0);
	virtual ~ConfigDialog();

	static const char* const externalProgramPathFilter;

private:
	class Ui_ConfigDialog& ui;
	QList<QListWidgetItem*> quickColorItems;
	QMap<QPushButton*, QColor> m_buttonColors;
	ExternalProgramWidgets m_externalProgramWidgets[NumExternalPrograms];
	class QSettings* m_settings;
	QVector<ColorToolbarItem> quickColors;

	void applySettings();
	void addShortcut (QAction* act);
	void setButtonBackground (QPushButton* button, QString value);
	void updateQuickColorList (ColorToolbarItem* sel = nullptr);
	void setShortcutText (ShortcutListItem* item);
	int getItemRow (QListWidgetItem* item, QList<QListWidgetItem*>& haystack);
	QString quickColorString();
	QListWidgetItem* getSelectedQuickColor();
	QList<ShortcutListItem*> getShortcutSelection();
	void initExtProgs();
	void applyToWidgetOptions (std::function<void (QWidget*, QString)> func);

private slots:
	void setButtonColor();
	void slot_setShortcut();
	void slot_resetShortcut();
	void slot_clearShortcut();
	void slot_setColor();
	void slot_delColor();
	void slot_addColorSeparator();
	void slot_moveColor();
	void slot_clearColors();
	void slot_setExtProgPath();
	void slot_findDownloadFolder();
	void buttonClicked (QAbstractButton* button);
	void selectPage (int row);
};

// =============================================================================
//
class KeySequenceDialog : public QDialog
{
	Q_OBJECT

public:
	explicit KeySequenceDialog (QKeySequence seq, QWidget* parent = nullptr, Qt::WindowFlags f = 0);
	static bool staticDialog (ShortcutListItem* item, QWidget* parent = nullptr);

	class QLabel* lb_output;
	class QDialogButtonBox* bbx_buttons;
	QKeySequence seq;

private:
	void updateOutput();

private slots:
	virtual void keyPressEvent (QKeyEvent* ev) override;
};
