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
#include "colorSelector.h"
#include "glRenderer.h"
#include "ui_config.h"

EXTERN_CFGENTRY (String, BackgroundColor);
EXTERN_CFGENTRY (String, MainColor);
EXTERN_CFGENTRY (Bool, ColorizeObjectsList);
EXTERN_CFGENTRY (Bool, BfcRedGreenView);
EXTERN_CFGENTRY (Float, MainColorAlpha);
EXTERN_CFGENTRY (Int, LineThickness);
EXTERN_CFGENTRY (String, QuickColorToolbar);
EXTERN_CFGENTRY (Bool, BlackEdges);
EXTERN_CFGENTRY (Bool, AntiAliasedLines);
EXTERN_CFGENTRY (Bool, ListImplicitFiles);
EXTERN_CFGENTRY (String, DownloadFilePath);
EXTERN_CFGENTRY (Bool, GuessDownloadPaths);
EXTERN_CFGENTRY (Bool, AutoCloseDownloadDialog);
EXTERN_CFGENTRY (Bool, UseLogoStuds);
EXTERN_CFGENTRY (Bool, DrawLineLengths);
EXTERN_CFGENTRY (String, DefaultName);
EXTERN_CFGENTRY (String, DefaultUser);
EXTERN_CFGENTRY (Bool, UseCALicense);
EXTERN_CFGENTRY (String, SelectColorBlend);
EXTERN_CFGENTRY (String, YtruderPath);
EXTERN_CFGENTRY (String, RectifierPath);
EXTERN_CFGENTRY (String, IntersectorPath);
EXTERN_CFGENTRY (String, CovererPath);
EXTERN_CFGENTRY (String, IsecalcPath);
EXTERN_CFGENTRY (String, Edger2Path);
EXTERN_CFGENTRY (Bool, YtruderUsesWine);
EXTERN_CFGENTRY (Bool, RectifierUsesWine);
EXTERN_CFGENTRY (Bool, IntersectorUsesWine);
EXTERN_CFGENTRY (Bool, CovererUsesWine);
EXTERN_CFGENTRY (Bool, IsecalcUsesWine);
EXTERN_CFGENTRY (Bool, Edger2UsesWine);
EXTERN_CFGENTRY (Float, GridCoarseCoordinateSnap);
EXTERN_CFGENTRY (Float, GridCoarseAngleSnap);
EXTERN_CFGENTRY (Float, GridMediumCoordinateSnap);
EXTERN_CFGENTRY (Float, GridMediumAngleSnap);
EXTERN_CFGENTRY (Float, GridFineCoordinateSnap);
EXTERN_CFGENTRY (Float, GridFineAngleSnap);
EXTERN_CFGENTRY (Bool, HighlightObjectBelowCursor)
EXTERN_CFGENTRY (Int, RoundPosition)
EXTERN_CFGENTRY (Int, RoundMatrix)

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
	assert (g_win != null);
	ui = new Ui_ConfigUI;
	ui->setupUi (this);

	// Interface tab
	setButtonBackground (ui->backgroundColorButton, cfg::BackgroundColor);
	connect (ui->backgroundColorButton, SIGNAL (clicked()),
			 this, SLOT (slot_setGLBackground()));

	setButtonBackground (ui->mainColorButton, cfg::MainColor);
	connect (ui->mainColorButton, SIGNAL (clicked()),
			 this, SLOT (slot_setGLForeground()));

	setButtonBackground (ui->selColorButton, cfg::SelectColorBlend);
	connect (ui->selColorButton, SIGNAL (clicked()),
			 this, SLOT (slot_setGLSelectColor()));

	ui->mainColorAlpha->setValue (cfg::MainColorAlpha * 10.0f);
	ui->lineThickness->setValue (cfg::LineThickness);
	ui->colorizeObjects->setChecked (cfg::ColorizeObjectsList);
	ui->colorBFC->setChecked (cfg::BfcRedGreenView);
	ui->blackEdges->setChecked (cfg::BlackEdges);
	ui->m_aa->setChecked (cfg::AntiAliasedLines);
	ui->implicitFiles->setChecked (cfg::ListImplicitFiles);
	ui->m_logostuds->setChecked (cfg::UseLogoStuds);
	ui->linelengths->setChecked (cfg::DrawLineLengths);

	ui->roundPosition->setValue (cfg::RoundPosition);
	ui->roundMatrix->setValue (cfg::RoundMatrix);

	g_win->applyToActions ([&](QAction* act)
	{
		addShortcut (act);
	});

	ui->shortcutsList->setSortingEnabled (true);
	ui->shortcutsList->sortItems();

	connect (ui->shortcut_set, SIGNAL (clicked()), this, SLOT (slot_setShortcut()));
	connect (ui->shortcut_reset, SIGNAL (clicked()), this, SLOT (slot_resetShortcut()));
	connect (ui->shortcut_clear, SIGNAL (clicked()), this, SLOT (slot_clearShortcut()));

	quickColors = quickColorsFromConfig();
	updateQuickColorList();

	connect (ui->quickColor_add, SIGNAL (clicked()), this, SLOT (slot_setColor()));
	connect (ui->quickColor_remove, SIGNAL (clicked()), this, SLOT (slot_delColor()));
	connect (ui->quickColor_edit, SIGNAL (clicked()), this, SLOT (slot_setColor()));
	connect (ui->quickColor_addSep, SIGNAL (clicked()), this, SLOT (slot_addColorSeparator()));
	connect (ui->quickColor_moveUp, SIGNAL (clicked()), this, SLOT (slot_moveColor()));
	connect (ui->quickColor_moveDown, SIGNAL (clicked()), this, SLOT (slot_moveColor()));
	connect (ui->quickColor_clear, SIGNAL (clicked()), this, SLOT (slot_clearColors()));

	ui->downloadPath->setText (cfg::DownloadFilePath);
	ui->guessNetPaths->setChecked (cfg::GuessDownloadPaths);
	ui->autoCloseNetPrompt->setChecked (cfg::AutoCloseDownloadDialog);
	connect (ui->findDownloadPath, SIGNAL (clicked (bool)), this, SLOT (slot_findDownloadFolder()));

	ui->m_profileName->setText (cfg::DefaultName);
	ui->m_profileUsername->setText (cfg::DefaultUser);
	ui->UseCALicense->setChecked (cfg::UseCALicense);
	ui->gridCoarseCoordinateSnap->setValue (cfg::GridCoarseCoordinateSnap);
	ui->gridCoarseAngleSnap->setValue (cfg::GridCoarseAngleSnap);
	ui->gridMediumCoordinateSnap->setValue (cfg::GridMediumCoordinateSnap);
	ui->gridMediumAngleSnap->setValue (cfg::GridMediumAngleSnap);
	ui->gridFineCoordinateSnap->setValue (cfg::GridFineCoordinateSnap);
	ui->gridFineAngleSnap->setValue (cfg::GridFineAngleSnap);
	ui->highlightObjectBelowCursor->setChecked (cfg::HighlightObjectBelowCursor);

	initExtProgs();
	selectPage (deftab);

	connect (ui->buttonBox, SIGNAL (clicked (QAbstractButton*)),
		this, SLOT (buttonClicked (QAbstractButton*)));

	connect (ui->m_pages, SIGNAL (currentChanged (int)),
		this, SLOT (selectPage (int)));

	connect (ui->m_pagelist, SIGNAL (currentRowChanged (int)),
		this, SLOT (selectPage (int)));
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
		item->setIcon (getIcon ("empty"));

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

		icon->setPixmap (getIcon (info.iconname));
		input->setText (*info.path);
		setPathButton->setIcon (getIcon ("folder"));
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

//
// Set the settings based on widget data.
//
void ConfigDialog::applySettings()
{
	// Apply configuration
	cfg::ColorizeObjectsList = ui->colorizeObjects->isChecked();
	cfg::BfcRedGreenView = ui->colorBFC->isChecked();
	cfg::BlackEdges = ui->blackEdges->isChecked();
	cfg::MainColorAlpha = ((double) ui->mainColorAlpha->value()) / 10.0;
	cfg::LineThickness = ui->lineThickness->value();
	cfg::ListImplicitFiles = ui->implicitFiles->isChecked();
	cfg::DownloadFilePath = ui->downloadPath->text();
	cfg::GuessDownloadPaths = ui->guessNetPaths->isChecked();
	cfg::AutoCloseDownloadDialog = ui->autoCloseNetPrompt->isChecked();
	cfg::UseLogoStuds = ui->m_logostuds->isChecked();
	cfg::DrawLineLengths = ui->linelengths->isChecked();
	cfg::DefaultUser = ui->m_profileUsername->text();
	cfg::DefaultName = ui->m_profileName->text();
	cfg::UseCALicense = ui->UseCALicense->isChecked();
	cfg::AntiAliasedLines = ui->m_aa->isChecked();
	cfg::HighlightObjectBelowCursor = ui->highlightObjectBelowCursor->isChecked();
	cfg::RoundPosition = ui->roundPosition->value();
	cfg::RoundMatrix = ui->roundMatrix->value();

	// Rebuild the quick color toolbar
	g_win->setQuickColors (quickColors);
	cfg::QuickColorToolbar = quickColorString();

	// Set the grid settings
	cfg::GridCoarseCoordinateSnap = ui->gridCoarseCoordinateSnap->value();
	cfg::GridCoarseAngleSnap = ui->gridCoarseAngleSnap->value();
	cfg::GridMediumCoordinateSnap = ui->gridMediumCoordinateSnap->value();
	cfg::GridMediumAngleSnap = ui->gridMediumAngleSnap->value();
	cfg::GridFineCoordinateSnap = ui->gridFineCoordinateSnap->value();
	cfg::GridFineAngleSnap = ui->gridFineAngleSnap->value();

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
	reloadAllSubfiles();
	loadLogoedStuds();
	g_win->R()->setBackground();
	g_win->doFullRefresh();
	g_win->updateDocumentList();
}

//
// A dialog button was clicked
//
void ConfigDialog::buttonClicked (QAbstractButton* button)
{
	typedef QDialogButtonBox QDDB;
	QDialogButtonBox* dbb = ui->buttonBox;

	if (button == dbb->button (QDDB::Ok))
	{
		applySettings();
		accept();
	} elif (button == dbb->button (QDDB::Apply))
	{
		applySettings();
	} elif (button == dbb->button (QDDB::Cancel))
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
			item->setText ("--------");
			item->setIcon (getIcon ("empty"));
		}
		else
		{
			LDColor col (entry.color());

			if (col == null)
			{
				item->setText ("[[unknown color]]");
				item->setIcon (getIcon ("error"));
			}
			else
			{
				item->setText (col.name());
				item->setIcon (makeColorIcon (col, 16));
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

	LDColor defval = entry ? entry->color() : null;
	LDColor val;

	if (not ColorSelector::selectColor (val, defval, this))
		return;

	if (entry)
		entry->setColor (val);
	else
	{
		LDQuickColor entry (val, null);

		item = getSelectedQuickColor();
		int idx = (item) ? getItemRow (item, quickColorItems) + 1 : quickColorItems.size();

		quickColors.insert (idx, entry);
		entry = quickColors[idx];
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

	LDQuickColor tmp = quickColors[dest];
	quickColors[dest] = quickColors[idx];
	quickColors[idx] = tmp;

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
// Pick a color and set the appropriate configuration option.
//
void ConfigDialog::pickColor (QString& conf, QPushButton* button)
{
	QColor col = QColorDialog::getColor (QColor (conf));

	if (col.isValid())
	{
		int r = col.red(),
			g = col.green(),
			b = col.blue();

		QString colname;
		colname.sprintf ("#%.2X%.2X%.2X", r, g, b);
		conf = colname;
		setButtonBackground (button, colname);
	}
}

//
//
void ConfigDialog::slot_setGLBackground()
{
	pickColor (cfg::BackgroundColor, ui->backgroundColorButton);
}

//
//
void ConfigDialog::slot_setGLForeground()
{
	pickColor (cfg::MainColor, ui->mainColorButton);
}

//
//
void ConfigDialog::slot_setGLSelectColor()
{
	pickColor (cfg::SelectColorBlend, ui->selColorButton);
}

//
// Sets background color of a given button.
//
void ConfigDialog::setButtonBackground (QPushButton* button, QString value)
{
	button->setIcon (getIcon ("colorselect"));
	button->setAutoFillBackground (true);
	button->setStyleSheet (format ("background-color: %1", value));
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

	assert (info != null);
	QString fpath = QFileDialog::getOpenFileName (this, format ("Path to %1", info->name), *info->path, g_extProgPathFilter);

	if (fpath.isEmpty())
		return;

	info->input->setText (fpath);
}

//
// '...' button pressed for the download path
//
void ConfigDialog::slot_findDownloadFolder()
{
	QString dpath = QFileDialog::getExistingDirectory();
	ui->downloadPath->setText (dpath);
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
	IMPLEMENT_DIALOG_BUTTONS

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
