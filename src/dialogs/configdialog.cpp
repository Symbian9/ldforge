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

#include <QGridLayout>
#include <QFileDialog>
#include <QColorDialog>
#include <QBoxLayout>
#include <QKeyEvent>
#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QSettings>
#include <QPushButton>
#include "../main.h"
#include "../lddocument.h"
#include "../librariesmodel.h"
#include "../canvas.h"
#include "../guiutilities.h"
#include "../documentmanager.h"
#include "colorselector.h"
#include "configdialog.h"
#include "ui_configdialog.h"

const char* const ConfigDialog::externalProgramPathFilter =
#ifdef _WIN32
	"Applications (*.exe)(*.exe);;"
#endif
	"All files (*.*)(*.*)";

ConfigDialog::ConfigDialog (QWidget* parent, ConfigDialog::Tab defaulttab, Qt::WindowFlags f) :
	QDialog (parent, f),
	HierarchyElement (parent),
	ui (*new Ui_ConfigDialog),
	librariesModel {new LibrariesModel {this->libraries, this}},
	libraries {config::libraries()},
	shortcuts {m_window},
	shortcutsDelegate {this}
{
	ui.setupUi (this);
	ui.librariesView->setModel(this->librariesModel);
	ui.shortcutsList->setModel(&shortcuts);
	ui.shortcutsList->setItemDelegateForColumn(ShortcutsModel::KeySequenceColumn, &shortcutsDelegate);

	// Set defaults
	applyToWidgetOptions([&](QWidget* widget, QString confname)
	{
		QVariant value = ::settingsObject().value(confname, config::defaults().value(confname));
		QLineEdit* lineedit;
		QSpinBox* spinbox;
		QDoubleSpinBox* doublespinbox;
		QSlider* slider;
		QCheckBox* checkbox;
		QPushButton* button;

		if ((lineedit = qobject_cast<QLineEdit*> (widget)))
		{
			lineedit->setText (value.toString());
		}
		else if ((spinbox = qobject_cast<QSpinBox*> (widget)))
		{
			spinbox->setValue (value.toInt());
		}
		else if ((doublespinbox = qobject_cast<QDoubleSpinBox*> (widget)))
		{
			doublespinbox->setValue (value.toDouble());
		}
		else if ((slider = qobject_cast<QSlider*> (widget)))
		{
			slider->setValue (value.toInt());
		}
		else if ((checkbox = qobject_cast<QCheckBox*> (widget)))
		{
			checkbox->setChecked (value.toBool());
		}
		else if ((button = qobject_cast<QPushButton*> (widget)))
		{
			setButtonBackground (button, value.toString());
			connect (button, SIGNAL (clicked()), this, SLOT (setButtonColor()));
		}
		else
		{
			print ("Unknown widget of type %1\n", widget->metaObject()->className());
		}
	});

	initExtProgs();
	selectPage (defaulttab);
	connect (ui.findDownloadPath, SIGNAL (clicked (bool)), this, SLOT (slot_findDownloadFolder()));
	connect (ui.buttonBox, SIGNAL (clicked (QAbstractButton*)),
		this, SLOT (buttonClicked (QAbstractButton*)));
	connect (ui.m_pages, SIGNAL (currentChanged (int)), this, SLOT (selectPage (int)));
	connect (ui.m_pagelist, SIGNAL (currentRowChanged (int)), this, SLOT (selectPage (int)));
	connect(
		this->ui.addLibrary,
		&QPushButton::clicked,
		[&]()
		{
			this->librariesModel->insertRow(this->librariesModel->rowCount());
		}
	);
	connect(
		this->ui.removeLibrary,
		&QPushButton::clicked,
		[&]()
		{
			QModelIndex index = this->ui.librariesView->currentIndex();

			if (index.isValid())
				this->librariesModel->removeRow(index.row());
		}
	);
	connect(
		this->ui.moveLibraryUp,
		&QPushButton::clicked,
		[&]()
		{
			QModelIndex index = this->ui.librariesView->currentIndex();

			if (index.isValid())
				this->librariesModel->moveRows({}, index.row(), 1, {}, index.row() - 1);
		}
	);
	connect(
		this->ui.moveLibraryDown,
		&QPushButton::clicked,
		[&]()
		{
			QModelIndex index = this->ui.librariesView->currentIndex();

			if (index.isValid())
				this->librariesModel->moveRows({}, index.row(), 1, {}, index.row() + 2);
		}
	);
}

ConfigDialog::~ConfigDialog()
{
	delete this->librariesModel;
	delete &ui;
}

void ConfigDialog::selectPage (int row)
{
	ui.m_pagelist->setCurrentRow (row);
	ui.m_pages->setCurrentIndex (row);
}

//
// Initializes the stuff in the ext programs tab
//
void ConfigDialog::initExtProgs()
{
	QGridLayout* pathsLayout = new QGridLayout;
	int row = 0;

	for (int i = 0; i < NumExternalPrograms; ++i)
	{
		ExtProgramType program = (ExtProgramType) i;
		ExternalProgramWidgets& widgets = m_externalProgramWidgets[i];
		QString name = m_window->externalPrograms()->externalProgramName (program);
		QLabel* icon = new QLabel;
		QLabel* progLabel = new QLabel (name);
		QLineEdit* input = new QLineEdit;
		QPushButton* setPathButton = new QPushButton;

		icon->setPixmap (MainWindow::getIcon (name.toLower()));
		input->setText (m_window->externalPrograms()->getPathSetting (program));
		setPathButton->setIcon (MainWindow::getIcon ("folder"));
		widgets.input = input;
		widgets.setPathButton = setPathButton;
		widgets.wineBox = nullptr;
		connect (setPathButton, SIGNAL (clicked()), this, SLOT (slot_setExtProgPath()));
		pathsLayout->addWidget (icon, row, 0);
		pathsLayout->addWidget (progLabel, row, 1);
		pathsLayout->addWidget (input, row, 2);
		pathsLayout->addWidget (setPathButton, row, 3);

#ifdef Q_OS_UNIX
		{
			QCheckBox* wineBox = new QCheckBox ("Wine");
			wineBox->setChecked (m_window->externalPrograms()->programUsesWine (program));
			widgets.wineBox = wineBox;
			pathsLayout->addWidget (wineBox, row, 4);
		}
#endif
		++row;
	}
	ui.extProgs->setLayout (pathsLayout);
}

void ConfigDialog::applyToWidgetOptions (std::function<void (QWidget*, QString)> func)
{
	// Apply configuration
	for (QWidget* widget : findChildren<QWidget*>())
	{
		if (not widget->objectName().startsWith ("config"))
			continue;

		QString optionname (widget->objectName().mid (strlen ("config")));

		if (config::exists(optionname))
			func (widget, optionname);
		else
			print ("Couldn't find configuration entry named %1", optionname);
	}
}

//
// Set the settings based on widget data.
//
void ConfigDialog::applySettings()
{
	applyToWidgetOptions ([&](QWidget* widget, QString confname)
	{
		QVariant value;
		QLineEdit* le;
		QSpinBox* spinbox;
		QDoubleSpinBox* doublespinbox;
		QSlider* slider;
		QCheckBox* checkbox;
		QPushButton* button;

		if ((le = qobject_cast<QLineEdit*> (widget)))
			value = le->text();
		else if ((spinbox = qobject_cast<QSpinBox*> (widget)))
			value = spinbox->value();
		else if ((doublespinbox = qobject_cast<QDoubleSpinBox*> (widget)))
			value = doublespinbox->value();
		else if ((slider = qobject_cast<QSlider*> (widget)))
			value = slider->value();
		else if ((checkbox = qobject_cast<QCheckBox*> (widget)))
			value = checkbox->isChecked();
		else if ((button = qobject_cast<QPushButton*> (widget)))
			value = m_buttonColors[button];
		else
		{
			print ("Unknown widget of type %1\n", widget->metaObject()->className());
			return;
		}

		settingsObject().setValue(confname, value);
	});

	ui.colorToolbarEditor->saveChanges();
	shortcuts.saveChanges();
	config::setLibraries(this->libraries);

	// Ext program settings
	for (int i = 0; i < NumExternalPrograms; ++i)
	{
		ExtProgramType program = (ExtProgramType) i;
		ExtProgramToolset* toolset = m_window->externalPrograms();
		ExternalProgramWidgets& widgets = m_externalProgramWidgets[i];
		toolset->getPathSetting (program) = widgets.input->text();

		if (widgets.wineBox)
			toolset->setWineSetting (program, widgets.wineBox->isChecked());
	}

	settingsObject().sync();
	emit settingsChanged();
}

//
// A dialog button was clicked
//
void ConfigDialog::buttonClicked (QAbstractButton* button)
{
	QDialogButtonBox* dbb = ui.buttonBox;

	if (button == dbb->button (QDialogButtonBox::Ok))
	{
		applySettings();
		accept();
	}
	else if (button == dbb->button (QDialogButtonBox::Apply))
	{
		applySettings();
	}
	else if (button == dbb->button (QDialogButtonBox::Cancel))
	{
		reject();
	}
}

//
//
void ConfigDialog::setButtonColor()
{
	QPushButton* button = qobject_cast<QPushButton*> (sender());

	if (button == nullptr)
	{
		print ("setButtonColor: null sender!\n");
		return;
	}

	QColor color = QColorDialog::getColor (m_buttonColors[button]);

	if (color.isValid())
	{
		QString colorname;
		colorname.sprintf ("#%.2X%.2X%.2X", color.red(), color.green(), color.blue());
		setButtonBackground (button, colorname);
	}
}

//
// Sets background color of a given button.
//
void ConfigDialog::setButtonBackground (QPushButton* button, QString value)
{
	button->setIcon (MainWindow::getIcon ("colorselect"));
	button->setAutoFillBackground (true);
	button->setStyleSheet (format ("background-color: %1", value));
	m_buttonColors[button] = QColor (value);
}

//
// Set the path of an external program
//
void ConfigDialog::slot_setExtProgPath()
{
	ExtProgramType program = NumExternalPrograms;

	for (int i = 0; i < NumExternalPrograms; ++i)
	{
		if (m_externalProgramWidgets[i].setPathButton == sender())
		{
			program = (ExtProgramType) i;
			break;
		}
	}

	if (program != NumExternalPrograms)
	{
		ExtProgramToolset* toolset = m_window->externalPrograms();
		ExternalProgramWidgets& widgets = m_externalProgramWidgets[program];
		QString filepath = QFileDialog::getOpenFileName (this,
			format ("Path to %1", toolset->externalProgramName (program)),
		    widgets.input->text(), externalProgramPathFilter);
	
		if (filepath.isEmpty())
			return;
	
		widgets.input->setText (filepath);
	}
}

//
// '...' button pressed for the download path
//
void ConfigDialog::slot_findDownloadFolder()
{
	QString dpath = QFileDialog::getExistingDirectory();

	if (not dpath.isEmpty())
		ui.configDownloadFilePath->setText (dpath);
}
