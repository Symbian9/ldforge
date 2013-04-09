/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 Santeri `arezey` Piippo
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

#include "common.h"
#include "zz_configDialog.h"
#include "file.h"
#include "config.h"
#include "misc.h"
#include <qgridlayout.h>
#include <qfiledialog.h>
#include <qcolordialog.h>
#include <qboxlayout.h>
#include <qevent.h>

extern_cfg (str, io_ldpath);
extern_cfg (str, gl_bgcolor);
extern_cfg (str, gl_maincolor);
extern_cfg (bool, lv_colorize);
extern_cfg (bool, gl_colorbfc);
extern_cfg (float, gl_maincolor_alpha);
extern_cfg (int, gl_linethickness);

ConfigDialog* g_ConfigDialog = nullptr;

#define INIT_CHECKBOX(BOX, CFG) \
	BOX->setCheckState (CFG ? Qt::Checked : Qt::Unchecked);

#define APPLY_CHECKBOX(BTN, CFG) \
	CFG = BTN->checkState() == Qt::Checked;

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ConfigDialog::ConfigDialog (ForgeWindow* parent) : QDialog (parent) {
	g_ConfigDialog = this;
	qTabs = new QTabWidget;
	
	initMainTab ();
	initShortcutsTab ();
	
	IMPLEMENT_DIALOG_BUTTONS
	
	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget (qTabs);
	layout->addWidget (qButtons);
	setLayout (layout);
	
	setWindowTitle (APPNAME_DISPLAY " - editing settings");
	setWindowIcon (QIcon ("icons/settings.png"));
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::initMainTab () {
	qMainTab = new QWidget;
	
	// =========================================================================
	// LDraw path
	qLDrawPath = new QLineEdit;
	qLDrawPath->setText (io_ldpath.value.chars());
	
	qLDrawPathLabel = new QLabel ("LDraw path:");
	
	qLDrawPathFindButton = new QPushButton;
	qLDrawPathFindButton->setIcon (QIcon ("icons/folder.png"));
	connect (qLDrawPathFindButton, SIGNAL (clicked ()),
		this, SLOT (slot_findLDrawPath ()));
	
	// =========================================================================
	// Background and foreground colors
	qGLBackgroundLabel = new QLabel ("Background color:");
	qGLBackgroundButton = new QPushButton;
	setButtonBackground (qGLBackgroundButton, gl_bgcolor.value);
	connect (qGLBackgroundButton, SIGNAL (clicked ()),
		this, SLOT (slot_setGLBackground ()));
	
	qGLForegroundLabel = new QLabel ("Foreground color:");
	qGLForegroundButton = new QPushButton;
	setButtonBackground (qGLForegroundButton, gl_maincolor.value);
	connect (qGLForegroundButton, SIGNAL (clicked ()),
		this, SLOT (slot_setGLForeground ()));
	
	// =========================================================================
	// Alpha and line thickness sliders
	qGLForegroundAlphaLabel = new QLabel ("Alpha:");
	makeSlider (qGLForegroundAlpha, 1, 10, (gl_maincolor_alpha * 10.0f));
	
	qGLLineThicknessLabel = new QLabel ("Line thickness:");
	makeSlider (qGLLineThickness, 1, 8, gl_linethickness);
	
	// =========================================================================
	// List view colorizer and BFC red/green view checkboxes
	qLVColorize = new QCheckBox ("Colorize polygons in list view");
	INIT_CHECKBOX (qLVColorize, lv_colorize)
	
	qGLColorBFC = new QCheckBox ("Red/green BFC view");
	INIT_CHECKBOX (qGLColorBFC, gl_colorbfc)
	
	QGridLayout* layout = new QGridLayout;
	layout->addWidget (qLDrawPathLabel, 0, 0);
	layout->addWidget (qLDrawPath, 0, 1, 1, 2);
	layout->addWidget (qLDrawPathFindButton, 0, 3);
	
	layout->addWidget (qGLBackgroundLabel, 1, 0);
	layout->addWidget (qGLBackgroundButton, 1, 1);
	layout->addWidget (qGLForegroundLabel, 1, 2);
	layout->addWidget (qGLForegroundButton, 1, 3);
	
	layout->addWidget (qGLLineThicknessLabel, 2, 0);
	layout->addWidget (qGLLineThickness, 2, 1);
	layout->addWidget (qGLForegroundAlphaLabel, 2, 2);
	layout->addWidget (qGLForegroundAlpha, 2, 3);
	
	layout->addWidget (qLVColorize, 3, 0, 1, 2);
	layout->addWidget (qGLColorBFC, 3, 2, 1, 2);
	qMainTab->setLayout (layout);
	
	// Add the tab to the manager
	qTabs->addTab (qMainTab, "Main settings");
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::initShortcutsTab () {
	QGridLayout* qLayout;
	
	qShortcutsTab = new QWidget;
	qShortcutList = new QListWidget;
	qLayout = new QGridLayout;
	
	// Init table items
	ulong i = 0;
	for (actionmeta meta : g_ActionMeta) {
		QAction* const qAct = *meta.qAct;
		QListWidgetItem* qItem = new QListWidgetItem;
		setShortcutText (qItem, meta);
		qItem->setIcon (qAct->icon ());
		
		qaShortcutItems.push_back (qItem);
		qShortcutList->insertItem (i, qItem);
		++i;
	}
	
	qSetShortcut = new QPushButton ("Set");
	qResetShortcut = new QPushButton ("Reset");
	qClearShortcut = new QPushButton ("Clear");
	
	connect (qSetShortcut, SIGNAL (clicked ()), this, SLOT (slot_setShortcut ()));
	connect (qResetShortcut, SIGNAL (clicked ()), this, SLOT (slot_resetShortcut ()));
	connect (qClearShortcut, SIGNAL (clicked ()), this, SLOT (slot_clearShortcut ()));
	
	QVBoxLayout* qButtonLayout = new QVBoxLayout;
	qButtonLayout->addWidget (qSetShortcut);
	qButtonLayout->addWidget (qResetShortcut);
	qButtonLayout->addWidget (qClearShortcut);
	qButtonLayout->addStretch (10);
	
	qLayout->addWidget (qShortcutList, 0, 0);
	qLayout->addLayout (qButtonLayout, 0, 1);
	qShortcutsTab->setLayout (qLayout);
	qTabs->addTab (qShortcutsTab, "Shortcuts");
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::makeSlider (QSlider*& qSlider, short int dMin, short int dMax,
	short dDefault)
{
	qSlider = new QSlider (Qt::Horizontal);
	qSlider->setRange (dMin, dMax);
	qSlider->setSliderPosition (dDefault);
	qSlider->setTickPosition (QSlider::TicksAbove);
	qSlider->setTickInterval (1);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ConfigDialog::~ConfigDialog () {
	g_ConfigDialog = nullptr;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::slot_findLDrawPath () {
	str zDir = QFileDialog::getExistingDirectory (this, "Choose LDraw directory",
		qLDrawPath->text());
	
	if (~zDir)
		qLDrawPath->setText (zDir.chars());
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::pickColor (strconfig& cfg, QPushButton* qButton) {
	QColorDialog dlg (QColor (cfg.value.chars()));
	dlg.setWindowIcon (QIcon ("icons/colorselect.png"));
	
	if (dlg.exec ()) {
		uchar r = dlg.currentColor ().red (),
			g = dlg.currentColor ().green (),
			b = dlg.currentColor ().blue ();
		cfg.value.format ("#%.2X%.2X%.2X", r, g, b);
		setButtonBackground (qButton, cfg.value);
	}
}

void ConfigDialog::slot_setGLBackground () {
	pickColor (gl_bgcolor, qGLBackgroundButton);
}

void ConfigDialog::slot_setGLForeground () {
	pickColor (gl_maincolor, qGLForegroundButton);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::setButtonBackground (QPushButton* qButton, str zValue) {
	qButton->setIcon (QIcon ("icons/colorselect.png"));
	qButton->setAutoFillBackground (true);
	qButton->setStyleSheet (
		str::mkfmt ("background-color: %s", zValue.chars()).chars()
	);
}

// =============================================================================
long ConfigDialog::getItemRow (QListWidgetItem* qItem) {
	long i = 0;
	
	for (QListWidgetItem* it : qaShortcutItems) {
		if (it == qItem)
			return i;
		++i;
	}
	
	return -1;
}

// =============================================================================
void ConfigDialog::slot_setShortcut () {
	QList<QListWidgetItem*> qaSel = qShortcutList->selectedItems ();
	
	if (qaSel.size() < 1)
		return;
	
	QListWidgetItem* qItem = qaSel[0];
	
	// Find the row this object is on.
	long idx = getItemRow (qItem);
	
	if (KeySequenceDialog::staticDialog (g_ActionMeta[idx], this))
		setShortcutText (qItem, g_ActionMeta[idx]);
}

// =============================================================================
void ConfigDialog::slot_resetShortcut () {
	QList<QListWidgetItem*> qaSel = qShortcutList->selectedItems ();
	
	for (QListWidgetItem* qItem : qaSel) {
		long idx = getItemRow (qItem);
		
		actionmeta meta = g_ActionMeta[idx];
		keyseqconfig* conf = g_ActionMeta[idx].conf;
		
		conf->reset ();
		(*meta.qAct)->setShortcut (*conf);
		
		setShortcutText (qItem, meta);
	}
}

// =============================================================================
void ConfigDialog::slot_clearShortcut () {
	QList<QListWidgetItem*> qaSel = qShortcutList->selectedItems ();
	QKeySequence qDummySeq;
	
	for (QListWidgetItem* qItem : qaSel) {
		long idx = getItemRow (qItem);
		
		actionmeta meta = g_ActionMeta[idx];
		keyseqconfig* conf = g_ActionMeta[idx].conf;
		conf->value = qDummySeq;
		
		(*meta.qAct)->setShortcut (*conf);
		setShortcutText (qItem, meta);
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::setShortcutText (QListWidgetItem* qItem, actionmeta meta) {
	QAction* const qAct = *meta.qAct;
	str zLabel = qAct->iconText ();
	str zKeybind = qAct->shortcut ().toString ();
	
	qItem->setText (str::mkfmt ("%s (%s)", zLabel.chars () ,zKeybind.chars ()).chars());
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::staticDialog (ForgeWindow* window) {
	ConfigDialog dlg (window);
	
	if (dlg.exec ()) {
		io_ldpath = dlg.qLDrawPath->text();
		
		APPLY_CHECKBOX (dlg.qLVColorize, lv_colorize)
		APPLY_CHECKBOX (dlg.qGLColorBFC, gl_colorbfc)
		
		gl_maincolor_alpha = ((double)dlg.qGLForegroundAlpha->value ()) / 10.0f;
		gl_linethickness = dlg.qGLLineThickness->value ();
		
		// Save the config
		config::save ();
		
		// Reload all subfiles
		reloadAllSubfiles ();
		
		window->R->setBackground ();
		window->refresh ();
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
KeySequenceDialog::KeySequenceDialog (QKeySequence seq, QWidget* parent,
	Qt::WindowFlags f) : QDialog (parent, f), seq (seq)
{
	qOutput = new QLabel;
	IMPLEMENT_DIALOG_BUTTONS
	
	setWhatsThis ("Into this dialog you can input a key sequence for use as a "
		"shortcut in LDForge. Use OK to confirm the new shortcut and Cancel to "
		"dismiss.");
	
	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget (qOutput);
	layout->addWidget (qButtons);
	setLayout (layout);
	
	updateOutput ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
bool KeySequenceDialog::staticDialog (actionmeta& meta, QWidget* parent) {
	KeySequenceDialog dlg (*meta.conf, parent);
	
	if (dlg.exec () == false)
		return false;
	
	*meta.conf = dlg.seq;
	(*meta.qAct)->setShortcut (*meta.conf);
	return true;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void KeySequenceDialog::updateOutput () {
	str zShortcut = seq.toString ();
	
	str zText = str::mkfmt ("<center><b>%s</b></center>", zShortcut.chars ());
	
	qOutput->setText (zText);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void KeySequenceDialog::keyPressEvent (QKeyEvent* ev) {
	seq = ev->key ();
	
	switch (seq) {
	case Qt::Key_Shift:
	case Qt::Key_Control:
	case Qt::Key_Alt:
	case Qt::Key_Meta:
		seq = 0;
		break;
	
	default:
		break;
	}
	
	seq = (seq | ev->modifiers ());
	
	updateOutput ();
}