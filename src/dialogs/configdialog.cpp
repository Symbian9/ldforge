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
 *  =====================================================================
 *
 *  configDialog.cxx: Settings dialog and everything related to it.
 *  Actual configuration core is in config.cxx.
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

ShortcutListItem::ShortcutListItem (QListWidget* view, int type) :
	QListWidgetItem (view, type) {}

QAction* ShortcutListItem::action() const
{
	return m_action;
}

void ShortcutListItem::setAction (QAction* action)
{
	m_action = action;
}

QKeySequence ShortcutListItem::sequence() const
{
	return m_sequence;
}

void ShortcutListItem::setSequence (const QKeySequence& sequence)
{
	m_sequence = sequence;
}

ConfigDialog::ConfigDialog (QWidget* parent, ConfigDialog::Tab defaulttab, Qt::WindowFlags f) :
	QDialog (parent, f),
	HierarchyElement (parent),
	ui (*new Ui_ConfigDialog),
	libraries {config::libraries()},
	librariesModel {new LibrariesModel {this->libraries, this}}
{
	ui.setupUi (this);
	ui.librariesView->setModel(this->librariesModel);

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

	m_window->applyToActions ([&](QAction* act)
	{
		addShortcut (act);
	});

	ui.shortcutsList->setSortingEnabled (true);
	ui.shortcutsList->sortItems();
	quickColors = guiUtilities()->loadQuickColorList();
	updateQuickColorList();
	initExtProgs();
	selectPage (defaulttab);
	connect (ui.shortcut_set, SIGNAL (clicked()), this, SLOT (slot_setShortcut()));
	connect (ui.shortcut_reset, SIGNAL (clicked()), this, SLOT (slot_resetShortcut()));
	connect (ui.shortcut_clear, SIGNAL (clicked()), this, SLOT (slot_clearShortcut()));
	connect (ui.quickColor_add, SIGNAL (clicked()), this, SLOT (slot_setColor()));
	connect (ui.quickColor_remove, SIGNAL (clicked()), this, SLOT (slot_delColor()));
	connect (ui.quickColor_edit, SIGNAL (clicked()), this, SLOT (slot_setColor()));
	connect (ui.quickColor_addSep, SIGNAL (clicked()), this, SLOT (slot_addColorSeparator()));
	connect (ui.quickColor_moveUp, SIGNAL (clicked()), this, SLOT (slot_moveColor()));
	connect (ui.quickColor_moveDown, SIGNAL (clicked()), this, SLOT (slot_moveColor()));
	connect (ui.quickColor_clear, SIGNAL (clicked()), this, SLOT (slot_clearColors()));
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
// Adds a shortcut entry to the list of shortcuts.
//
void ConfigDialog::addShortcut (QAction* act)
{
	ShortcutListItem* item = new ShortcutListItem;
	item->setIcon (act->icon());
	item->setAction (act);
	item->setSequence (act->shortcut());
	setShortcutText (item);

	// If the action doesn't have a valid icon, use an empty one
	// so that the list is kept aligned.
	if (act->icon().isNull())
		item->setIcon (MainWindow::getIcon ("empty"));

	ui.shortcutsList->insertItem (ui.shortcutsList->count(), item);
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

	// Rebuild the quick color toolbar
	m_window->setQuickColors (quickColors);
	config::setQuickColorToolbar (quickColorString());
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

	// Apply shortcuts
	for (int i = 0; i < ui.shortcutsList->count(); ++i)
	{
		auto item = static_cast<ShortcutListItem*> (ui.shortcutsList->item (i));
		item->action()->setShortcut (item->sequence());
	}

	settingsObject().sync();
	m_documents->loadLogoedStuds();
	m_window->renderer()->setBackground();
	m_window->doFullRefresh();
	m_window->updateDocumentList();
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
// Update the list of color toolbar items in the quick color tab.
//
void ConfigDialog::updateQuickColorList (ColorToolbarItem* sel)
{
	for (QListWidgetItem * item : quickColorItems)
		delete item;

	quickColorItems.clear();

	// Init table items
	for (ColorToolbarItem& entry : quickColors)
	{
		QListWidgetItem* item = new QListWidgetItem;

		if (entry.isSeparator())
		{
			item->setText ("<hr />");
			item->setIcon (MainWindow::getIcon ("empty"));
		}
		else
		{
			LDColor color = entry.color();

			if (color.isValid())
			{
				item->setText (color.name());
				item->setIcon (makeColorIcon (color, 16));
			}
			else
			{
				item->setText ("[[unknown color]]");
				item->setIcon (MainWindow::getIcon ("error"));
			}
		}

		ui.quickColorList->addItem (item);
		quickColorItems << item;

		if (sel and &entry == sel)
		{
			ui.quickColorList->setCurrentItem (item);
			ui.quickColorList->scrollToItem (item);
		}
	}
}

//
// Quick colors: add or edit button was clicked.
//
void ConfigDialog::slot_setColor()
{
	ColorToolbarItem* entry = nullptr;
	QListWidgetItem* item = nullptr;
	const bool isNew = static_cast<QPushButton*> (sender()) == ui.quickColor_add;

	if (not isNew)
	{
		item = getSelectedQuickColor();

		if (not item)
			return;

		int i = getItemRow (item, quickColorItems);
		entry = &quickColors[i];

		if (entry->isSeparator() == true)
			return; // don't color separators
	}

	LDColor defaultValue = entry ? entry->color() : LDColor::nullColor;
	LDColor value;

	if (not ColorSelector::selectColor (this, value, defaultValue))
		return;

	if (entry)
	{
		entry->setColor (value);
	}
	else
	{
		ColorToolbarItem newentry {value};
		item = getSelectedQuickColor();
		int idx = (item) ? getItemRow (item, quickColorItems) + 1 : countof(quickColorItems);
		quickColors.insert(idx, newentry);
		entry = &quickColors[idx];
	}

	updateQuickColorList (entry);
}

//
// Remove a quick color
//
void ConfigDialog::slot_delColor()
{
	if (ui.quickColorList->selectedItems().isEmpty())
		return;

	QListWidgetItem* item = ui.quickColorList->selectedItems() [0];
	quickColors.removeAt (getItemRow (item, quickColorItems));
	updateQuickColorList();
}

//
// Move a quick color up/down
//
void ConfigDialog::slot_moveColor()
{
	const bool up = (static_cast<QPushButton*> (sender()) == ui.quickColor_moveUp);

	if (ui.quickColorList->selectedItems().isEmpty())
		return;

	QListWidgetItem* item = ui.quickColorList->selectedItems() [0];
	int idx = getItemRow (item, quickColorItems);
	int dest = up ? (idx - 1) : (idx + 1);

	if (dest < 0 or dest >= countof(quickColorItems))
		return; // destination out of bounds

	qSwap (quickColors[dest], quickColors[idx]);
	updateQuickColorList (&quickColors[dest]);
}

//
//
// Add a separator to quick colors
//
void ConfigDialog::slot_addColorSeparator()
{
	quickColors << ColorToolbarItem::makeSeparator();
	updateQuickColorList (&quickColors[countof(quickColors) - 1]);
}

//
//
// Clear all quick colors
//
void ConfigDialog::slot_clearColors()
{
	quickColors.clear();
	updateQuickColorList();
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
// Finds the given list widget item in the list of widget items given.
//
int ConfigDialog::getItemRow (QListWidgetItem* item, QList<QListWidgetItem*>& haystack)
{
	int i = 0;

	for (QListWidgetItem* it : haystack)
	{
		if (it == item)
			return i;

		++i;
	}

	return -1;
}

//
// Which quick color is currently selected?
//
QListWidgetItem* ConfigDialog::getSelectedQuickColor()
{
	if (ui.quickColorList->selectedItems().isEmpty())
		return nullptr;

	return ui.quickColorList->selectedItems() [0];
}

//
// Get the list of shortcuts selected
//
QList<ShortcutListItem*> ConfigDialog::getShortcutSelection()
{
	QList<ShortcutListItem*> out;

	for (QListWidgetItem* entry : ui.shortcutsList->selectedItems())
		out << static_cast<ShortcutListItem*> (entry);

	return out;
}

//
// Edit the shortcut of a given action.
//
void ConfigDialog::slot_setShortcut()
{
	QList<ShortcutListItem*> sel = getShortcutSelection();

	if (countof(sel) < 1)
		return;

	ShortcutListItem* item = sel[0];

	if (KeySequenceDialog::staticDialog (item, this))
		setShortcutText (item);
}

//
// Reset a shortcut to defaults
//
void ConfigDialog::slot_resetShortcut()
{
	QList<ShortcutListItem*> sel = getShortcutSelection();

	for (ShortcutListItem* item : sel)
	{
		item->setSequence (m_window->defaultShortcut (item->action()));
		setShortcutText (item);
	}
}

//
// Remove the shortcut of an action.
//
void ConfigDialog::slot_clearShortcut()
{
	QList<ShortcutListItem*> sel = getShortcutSelection();

	for (ShortcutListItem* item : sel)
	{
		item->setSequence (QKeySequence());
		setShortcutText (item);
	}
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

//
//
// Updates the text string for a given shortcut list item
//
void ConfigDialog::setShortcutText (ShortcutListItem* item)
{
	QAction* act = item->action();
	QString label = act->iconText();
	QString keybind = item->sequence().toString();
	item->setText (format ("%1 (%2)", label, keybind));
}

//
// Gets the configuration string of the quick color toolbar
//
QString ConfigDialog::quickColorString()
{
	QString val;

	for (const ColorToolbarItem& entry : quickColors)
	{
		if (not val.isEmpty())
			val += ':';

		if (entry.isSeparator())
			val += '|';
		else
			val += format ("%1", entry.color().index());
	}

	return val;
}

//
//
KeySequenceDialog::KeySequenceDialog (QKeySequence seq, QWidget* parent, Qt::WindowFlags f) :
	QDialog (parent, f), seq (seq)
{
	lb_output = new QLabel;

	bbx_buttons = new QDialogButtonBox (QDialogButtonBox::Ok | QDialogButtonBox::Cancel); \
	connect (bbx_buttons, SIGNAL (accepted()), this, SLOT (accept())); \
	connect (bbx_buttons, SIGNAL (rejected()), this, SLOT (reject())); \

	setWhatsThis (tr ("Into this dialog you can input a key sequence for use as a "
		"shortcut in LDForge. Use OK to confirm the new shortcut and Cancel to "
		"dismiss."));

	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget (lb_output);
	layout->addWidget (bbx_buttons);
	setLayout (layout);

	updateOutput();
}

//
//
bool KeySequenceDialog::staticDialog (ShortcutListItem* item, QWidget* parent)
{
	KeySequenceDialog dlg (item->sequence(), parent);

	if (dlg.exec() == QDialog::Rejected)
		return false;

	item->setSequence (dlg.seq);
	return true;
}

//
//
void KeySequenceDialog::updateOutput()
{
	QString shortcut = seq.toString();

	if (seq == QKeySequence())
		shortcut = "&lt;empty&gt;";

	QString text = format ("<center><b>%1</b></center>", shortcut);
	lb_output->setText (text);
}

//
//
void KeySequenceDialog::keyPressEvent (QKeyEvent* ev)
{
	seq = ev->key() + ev->modifiers();
	updateOutput();
}
