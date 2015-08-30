/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Teemu Piippo
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
#include "main.h"
#include "configDialog.h"
#include "ldDocument.h"
#include "configuration.h"
#include "miscallenous.h"
#include "colors.h"
#include "dialogs/colorselector.h"
#include "glRenderer.h"
#include "ui_config.h"

EXTERN_CFGENTRY (String, YtruderPath)
EXTERN_CFGENTRY (String, RectifierPath)
EXTERN_CFGENTRY (String, IntersectorPath)
EXTERN_CFGENTRY (String, CovererPath)
EXTERN_CFGENTRY (String, IsecalcPath)
EXTERN_CFGENTRY (String, Edger2Path)
EXTERN_CFGENTRY (Bool, YtruderUsesWine)
EXTERN_CFGENTRY (Bool, RectifierUsesWine)
EXTERN_CFGENTRY (Bool, IntersectorUsesWine)
EXTERN_CFGENTRY (Bool, CovererUsesWine)
EXTERN_CFGENTRY (Bool, IsecalcUsesWine)
EXTERN_CFGENTRY (Bool, Edger2UsesWine)
EXTERN_CFGENTRY (String, QuickColorToolbar)

const char* g_extProgPathFilter =
#ifdef _WIN32
	"Applications (*.exe)(*.exe);;"
#endif
	"All files (*.*)(*.*)";

//
//
static struct LDExtProgInfo
{
	QString const	name;
	QString const	iconname;
	QString* const	path;
	QLineEdit*		input;
	QPushButton*	setPathButton;
	bool* const		wine;
	QCheckBox*		wineBox;
} g_LDExtProgInfo[] =
{
#ifndef _WIN32
# define EXTPROG(NAME, LOWNAME) { #NAME, #LOWNAME, &cfg::NAME##Path, null, null, \
	&cfg::NAME##UsesWine, null },
#else
# define EXTPROG(NAME, LOWNAME) { #NAME, #LOWNAME, &cfg::NAME##Path, null, null, null, null },
#endif
	EXTPROG (Ytruder, ytruder)
	EXTPROG (Rectifier, rectifier)
	EXTPROG (Intersector, intersector)
	EXTPROG (Isecalc, isecalc)
	EXTPROG (Coverer, coverer)
	EXTPROG (Edger2, edger2)
#undef EXTPROG
};

//
//
ConfigDialog::ConfigDialog (ConfigDialog::Tab deftab, QWidget* parent, Qt::WindowFlags f) :
	QDialog (parent, f)
{
	ui = new Ui_ConfigUI;
	ui->setupUi (this);

	// Set defaults
	m_applyToWidgetOptions ([&](QWidget* wdg, AbstractConfigEntry* conf)
	{
		QVariant value (conf->toVariant());
		QLineEdit* le;
		QSpinBox* spinbox;
		QDoubleSpinBox* doublespinbox;
		QSlider* slider;
		QCheckBox* checkbox;
		QPushButton* button;

		if ((le = qobject_cast<QLineEdit*> (wdg)) != null)
		{
			le->setText (value.toString());
		}
		else if ((spinbox = qobject_cast<QSpinBox*> (wdg)) != null)
		{
			spinbox->setValue (value.toInt());
		}
		else if ((doublespinbox = qobject_cast<QDoubleSpinBox*> (wdg)) != null)
		{
			doublespinbox->setValue (value.toDouble());
		}
		else if ((slider = qobject_cast<QSlider*> (wdg)) != null)
		{
			slider->setValue (value.toInt());
		}
		else if ((checkbox = qobject_cast<QCheckBox*> (wdg)) != null)
		{
			checkbox->setChecked (value.toBool());
		}
		else if ((button = qobject_cast<QPushButton*> (wdg)) != null)
		{
			setButtonBackground (button, value.toString());
			connect (button, SIGNAL (clicked()), this, SLOT (setButtonColor()));
		}
		else
		{
			print ("Unknown widget of type %1\n", wdg->metaObject()->className());
		}
	});

	if (g_win)
	{
		g_win->applyToActions ([&](QAction* act)
		{
			addShortcut (act);
		});
	}

	ui->shortcutsList->setSortingEnabled (true);
	ui->shortcutsList->sortItems();
	quickColors = LoadQuickColorList();
	updateQuickColorList();
	initExtProgs();
	selectPage (deftab);
	connect (ui->shortcut_set, SIGNAL (clicked()), this, SLOT (slot_setShortcut()));
	connect (ui->shortcut_reset, SIGNAL (clicked()), this, SLOT (slot_resetShortcut()));
	connect (ui->shortcut_clear, SIGNAL (clicked()), this, SLOT (slot_clearShortcut()));
	connect (ui->quickColor_add, SIGNAL (clicked()), this, SLOT (slot_setColor()));
	connect (ui->quickColor_remove, SIGNAL (clicked()), this, SLOT (slot_delColor()));
	connect (ui->quickColor_edit, SIGNAL (clicked()), this, SLOT (slot_setColor()));
	connect (ui->quickColor_addSep, SIGNAL (clicked()), this, SLOT (slot_addColorSeparator()));
	connect (ui->quickColor_moveUp, SIGNAL (clicked()), this, SLOT (slot_moveColor()));
	connect (ui->quickColor_moveDown, SIGNAL (clicked()), this, SLOT (slot_moveColor()));
	connect (ui->quickColor_clear, SIGNAL (clicked()), this, SLOT (slot_clearColors()));
	connect (ui->findDownloadPath, SIGNAL (clicked (bool)), this, SLOT (slot_findDownloadFolder()));
	connect (ui->buttonBox, SIGNAL (clicked (QAbstractButton*)),
		this, SLOT (buttonClicked (QAbstractButton*)));
	connect (ui->m_pages, SIGNAL (currentChanged (int)), this, SLOT (selectPage (int)));
	connect (ui->m_pagelist, SIGNAL (currentRowChanged (int)), this, SLOT (selectPage (int)));
}

//
//
ConfigDialog::~ConfigDialog()
{
	delete ui;
}

//
//
void ConfigDialog::selectPage (int row)
{
	ui->m_pagelist->setCurrentRow (row);
	ui->m_pages->setCurrentIndex (row);
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
		item->setIcon (GetIcon ("empty"));

	ui->shortcutsList->insertItem (ui->shortcutsList->count(), item);
}

//
// Initializes the stuff in the ext programs tab
//
void ConfigDialog::initExtProgs()
{
	QGridLayout* pathsLayout = new QGridLayout;
	int row = 0;

	for (LDExtProgInfo& info : g_LDExtProgInfo)
	{
		QLabel* icon = new QLabel,
		*progLabel = new QLabel (info.name);
		QLineEdit* input = new QLineEdit;
		QPushButton* setPathButton = new QPushButton;

		icon->setPixmap (GetIcon (info.iconname));
		input->setText (*info.path);
		setPathButton->setIcon (GetIcon ("folder"));
		info.input = input;
		info.setPathButton = setPathButton;

		connect (setPathButton, SIGNAL (clicked()), this, SLOT (slot_setExtProgPath()));

		pathsLayout->addWidget (icon, row, 0);
		pathsLayout->addWidget (progLabel, row, 1);
		pathsLayout->addWidget (input, row, 2);
		pathsLayout->addWidget (setPathButton, row, 3);

		if (info.wine != null)
		{
			QCheckBox* wineBox = new QCheckBox ("Wine");
			wineBox->setChecked (*info.wine);
			info.wineBox = wineBox;
			pathsLayout->addWidget (wineBox, row, 4);
		}

		++row;
	}

	ui->extProgs->setLayout (pathsLayout);
}

void ConfigDialog::m_applyToWidgetOptions (std::function<void (QWidget*, AbstractConfigEntry*)> func)
{
	// Apply configuration
	for (QWidget* widget : findChildren<QWidget*>())
	{
		if (not widget->objectName().startsWith ("config"))
			continue;

		QString confname (widget->objectName().mid (strlen ("config")));
		AbstractConfigEntry* conf (Config::FindByName (confname));

		if (conf == null)
		{
			print ("Couldn't find configuration entry named %1", confname);
			continue;
		}

		func (widget, conf);
	}
}

//
// Set the settings based on widget data.
//
void ConfigDialog::applySettings()
{
	m_applyToWidgetOptions ([&](QWidget* widget, AbstractConfigEntry* conf)
	{
		QVariant value (conf->toVariant());
		QLineEdit* le;
		QSpinBox* spinbox;
		QDoubleSpinBox* doublespinbox;
		QSlider* slider;
		QCheckBox* checkbox;
		QPushButton* button;

		if ((le = qobject_cast<QLineEdit*> (widget)) != null)
			value = le->text();
		else if ((spinbox = qobject_cast<QSpinBox*> (widget)) != null)
			value = spinbox->value();
		else if ((doublespinbox = qobject_cast<QDoubleSpinBox*> (widget)) != null)
			value = doublespinbox->value();
		else if ((slider = qobject_cast<QSlider*> (widget)) != null)
			value = slider->value();
		else if ((checkbox = qobject_cast<QCheckBox*> (widget)) != null)
			value = checkbox->isChecked();
		else if ((button = qobject_cast<QPushButton*> (widget)) != null)
			value = m_buttonColors[button];
		else
			print ("Unknown widget of type %1\n", widget->metaObject()->className());

		conf->loadFromVariant (value);
	});

	// Rebuild the quick color toolbar
	if (g_win)
		g_win->setQuickColors (quickColors);

	cfg::QuickColorToolbar = quickColorString();

	// Ext program settings
	for (const LDExtProgInfo& info : g_LDExtProgInfo)
	{
		*info.path = info.input->text();

		if (info.wine != null)
			*info.wine = info.wineBox->isChecked();
	}

	// Apply shortcuts
	for (int i = 0; i < ui->shortcutsList->count(); ++i)
	{
		auto item = static_cast<ShortcutListItem*> (ui->shortcutsList->item (i));
		item->action()->setShortcut (item->sequence());
	}

	Config::Save();
	LDDocument::current()->reloadAllSubfiles();
	LoadLogoStuds();

	if (g_win)
	{
		g_win->R()->setBackground();
		g_win->doFullRefresh();
		g_win->updateDocumentList();
	}
}

//
// A dialog button was clicked
//
void ConfigDialog::buttonClicked (QAbstractButton* button)
{
	QDialogButtonBox* dbb = ui->buttonBox;

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
void ConfigDialog::updateQuickColorList (LDQuickColor* sel)
{
	for (QListWidgetItem * item : quickColorItems)
		delete item;

	quickColorItems.clear();

	// Init table items
	for (LDQuickColor& entry : quickColors)
	{
		QListWidgetItem* item = new QListWidgetItem;

		if (entry.isSeparator())
		{
			item->setText ("<hr />");
			item->setIcon (GetIcon ("empty"));
		}
		else
		{
			LDColor color = entry.color();

			if (color.isValid())
			{
				item->setText (color.name());
				item->setIcon (MakeColorIcon (color, 16));
			}
			else
			{
				item->setText ("[[unknown color]]");
				item->setIcon (GetIcon ("error"));
			}
		}

		ui->quickColorList->addItem (item);
		quickColorItems << item;

		if (sel and &entry == sel)
		{
			ui->quickColorList->setCurrentItem (item);
			ui->quickColorList->scrollToItem (item);
		}
	}
}

//
// Quick colors: add or edit button was clicked.
//
void ConfigDialog::slot_setColor()
{
	LDQuickColor* entry = null;
	QListWidgetItem* item = null;
	const bool isNew = static_cast<QPushButton*> (sender()) == ui->quickColor_add;

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

	LDColor defaultValue = entry ? entry->color() : LDColor::nullColor();
	LDColor value;

	if (not ColorSelector::selectColor (value, defaultValue, this))
		return;

	if (entry != null)
	{
		entry->setColor (value);
	}
	else
	{
		LDQuickColor newentry (value, null);
		item = getSelectedQuickColor();
		int idx = (item) ? getItemRow (item, quickColorItems) + 1 : quickColorItems.size();
		quickColors.insert (idx, newentry);
		entry = &quickColors[idx];
	}

	updateQuickColorList (entry);
}

//
// Remove a quick color
//
void ConfigDialog::slot_delColor()
{
	if (ui->quickColorList->selectedItems().isEmpty())
		return;

	QListWidgetItem* item = ui->quickColorList->selectedItems() [0];
	quickColors.removeAt (getItemRow (item, quickColorItems));
	updateQuickColorList();
}

//
// Move a quick color up/down
//
void ConfigDialog::slot_moveColor()
{
	const bool up = (static_cast<QPushButton*> (sender()) == ui->quickColor_moveUp);

	if (ui->quickColorList->selectedItems().isEmpty())
		return;

	QListWidgetItem* item = ui->quickColorList->selectedItems() [0];
	int idx = getItemRow (item, quickColorItems);
	int dest = up ? (idx - 1) : (idx + 1);

	if (dest < 0 or dest >= quickColorItems.size())
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
	quickColors << LDQuickColor::getSeparator();
	updateQuickColorList (&quickColors[quickColors.size() - 1]);
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

	if (button == null)
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
	button->setIcon (GetIcon ("colorselect"));
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
	if (ui->quickColorList->selectedItems().isEmpty())
		return null;

	return ui->quickColorList->selectedItems() [0];
}

//
// Get the list of shortcuts selected
//
QList<ShortcutListItem*> ConfigDialog::getShortcutSelection()
{
	QList<ShortcutListItem*> out;

	for (QListWidgetItem* entry : ui->shortcutsList->selectedItems())
		out << static_cast<ShortcutListItem*> (entry);

	return out;
}

//
// Edit the shortcut of a given action.
//
void ConfigDialog::slot_setShortcut()
{
	QList<ShortcutListItem*> sel = getShortcutSelection();

	if (sel.size() < 1)
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
		item->setSequence (MainWindow::defaultShortcut (item->action()));
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
	const LDExtProgInfo* info = null;

	for (const LDExtProgInfo& it : g_LDExtProgInfo)
	{
		if (it.setPathButton == sender())
		{
			info = &it;
			break;
		}
	}

	if (info != null)
	{
		QString filepath = QFileDialog::getOpenFileName (this,
			format ("Path to %1", info->name), *info->path, g_extProgPathFilter);
	
		if (filepath.isEmpty())
			return;
	
		info->input->setText (filepath);
	}
}

//
// '...' button pressed for the download path
//
void ConfigDialog::slot_findDownloadFolder()
{
	QString dpath = QFileDialog::getExistingDirectory();

	if (not dpath.isEmpty())
		ui->configDownloadFilePath->setText (dpath);
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

	for (const LDQuickColor& entry : quickColors)
	{
		if (val.length() > 0)
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
