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
#include "../mainwindow.h"
#include "../toolsets/extprogramtoolset.h"
#include "shortcutsmodel.h"
#include <QDialog>

struct ExternalProgramWidgets
{
	class QLineEdit* input;
	class QPushButton* setPathButton;
	class QCheckBox* wineBox;
};

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

signals:
	void settingsChanged();

private:
	class Ui_ConfigDialog& ui;
	QVector<QListWidgetItem*> quickColorItems;
	QMap<QPushButton*, QColor> m_buttonColors;
	ExternalProgramWidgets m_externalProgramWidgets[NumExternalPrograms];
	class LibrariesModel* librariesModel;
	Libraries libraries;
	ShortcutsModel shortcuts;
	KeySequenceDelegate shortcutsDelegate;

	void applySettings();
	void setButtonBackground (QPushButton* button, QString value);
	void initExtProgs();
	void applyToWidgetOptions (std::function<void (QWidget*, QString)> func);

private slots:
	void setButtonColor();
	void slot_setExtProgPath();
	void slot_findDownloadFolder();
	void buttonClicked (QAbstractButton* button);
	void selectPage (int row);
};
