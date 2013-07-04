/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 Santeri Piippo
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

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QLabel>
#include <QPushButton>
#include <QBoxLayout>
#include <QGridLayout>
#include <qprogressbar.h>
#include <QCheckBox>

#include "dialogs.h"
#include "widgets.h"
#include "gui.h"
#include "gldraw.h"
#include "docs.h"
#include "file.h"
#include "dialogs.h"

extern_cfg (str, io_ldpath);

// =============================================================================
OverlayDialog::OverlayDialog (QWidget* parent, Qt::WindowFlags f) : QDialog (parent, f) {
	rb_camera = new RadioBox ("Camera", {}, 0, Qt::Horizontal, this);
	
	for (int i = 0; i < 6; ++i) {
		if (i == 3)
			rb_camera->rowBreak ();
		
		rb_camera->addButton (g_CameraNames[i]);
	}
	
	GL::Camera cam = g_win->R ()->camera ();
	if (cam != GL::Free)
		rb_camera->setValue ((int) cam);
	
	QGroupBox* gb_image = new QGroupBox ("Image", this);
	
	QLabel* lb_fpath = new QLabel ("File:");
	le_fpath = new QLineEdit;
	le_fpath->setFocus ();
	
	QLabel* lb_ofs = new QLabel ("Origin:");
	btn_fpath = new QPushButton;
	btn_fpath->setIcon (getIcon ("folder"));
	connect (btn_fpath, SIGNAL (clicked ()), this, SLOT (slot_fpath ()));
	
	sb_ofsx = new QSpinBox;
	sb_ofsy = new QSpinBox;
	sb_ofsx->setRange (0, 10000);
	sb_ofsy->setRange (0, 10000);
	sb_ofsx->setSuffix (" px");
	sb_ofsy->setSuffix (" px");
	
	QLabel* lb_dimens = new QLabel ("Dimensions:");
	dsb_lwidth = new QDoubleSpinBox;
	dsb_lheight = new QDoubleSpinBox;
	dsb_lwidth->setRange (0.0f, 10000.0f);
	dsb_lheight->setRange (0.0f, 10000.0f);
	dsb_lwidth->setSuffix (" LDU");
	dsb_lheight->setSuffix (" LDU");
	dsb_lwidth->setSpecialValueText ("Automatic");
	dsb_lheight->setSpecialValueText ("Automatic");
	
	dbb_buttons = makeButtonBox (*this);
	dbb_buttons->addButton (QDialogButtonBox::Help);
	connect (dbb_buttons, SIGNAL (helpRequested ()), this, SLOT (slot_help()));
	
	QHBoxLayout* fpathlayout = new QHBoxLayout;
	fpathlayout->addWidget (lb_fpath);
	fpathlayout->addWidget (le_fpath);
	fpathlayout->addWidget (btn_fpath);
	
	QGridLayout* metalayout = new QGridLayout;
	metalayout->addWidget (lb_ofs,			0, 0);
	metalayout->addWidget (sb_ofsx,		0, 1);
	metalayout->addWidget (sb_ofsy,		0, 2);
	metalayout->addWidget (lb_dimens,		1, 0);
	metalayout->addWidget (dsb_lwidth,	1, 1);
	metalayout->addWidget (dsb_lheight,	1, 2);
	
	QVBoxLayout* imagelayout = new QVBoxLayout (gb_image);
	imagelayout->addLayout (fpathlayout);
	imagelayout->addLayout (metalayout);
	
	QVBoxLayout* layout = new QVBoxLayout (this);
	layout->addWidget (rb_camera);
	layout->addWidget (gb_image);
	layout->addWidget (dbb_buttons);
	
	connect (dsb_lwidth, SIGNAL (valueChanged (double)), this, SLOT (slot_dimensionsChanged ()));
	connect (dsb_lheight, SIGNAL (valueChanged (double)), this, SLOT (slot_dimensionsChanged ()));
	connect (rb_camera, SIGNAL (valueChanged (int)), this, SLOT (fillDefaults (int)));
	
	slot_dimensionsChanged ();
	fillDefaults (cam);
}

void OverlayDialog::fillDefaults (int newcam) {
	overlayMeta& info = g_win->R ()->getOverlay (newcam);
	
	if (info.img != null) {
		le_fpath->setText (info.fname);
		sb_ofsx->setValue (info.ox);
		sb_ofsy->setValue (info.oy);
		dsb_lwidth->setValue (info.lw);
		dsb_lheight->setValue (info.lh);
	} else {
		le_fpath->setText ("");
		sb_ofsx->setValue (0);
		sb_ofsy->setValue (0);
		dsb_lwidth->setValue (0.0f);
		dsb_lheight->setValue (0.0f);
	} 
}

str		OverlayDialog::fpath		() const { return le_fpath->text (); }
ushort	OverlayDialog::ofsx		() const { return sb_ofsx->value (); }
ushort	OverlayDialog::ofsy		() const { return sb_ofsy->value (); }
double	OverlayDialog::lwidth		() const { return dsb_lwidth->value (); }
double	OverlayDialog::lheight		() const { return dsb_lheight->value (); }
int		OverlayDialog::camera		() const { return rb_camera->value (); }

void OverlayDialog::slot_fpath () {
	le_fpath->setText (QFileDialog::getOpenFileName (null, "Overlay image"));
}

void OverlayDialog::slot_help () {
	showDocumentation (g_docs_overlays);
}

void OverlayDialog::slot_dimensionsChanged () {
	bool enable = (dsb_lwidth->value () != 0) || (dsb_lheight->value () != 0);
	dbb_buttons->button (QDialogButtonBox::Ok)->setEnabled (enable);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
SetContentsDialog::SetContentsDialog (QWidget* parent, Qt::WindowFlags f) : QDialog (parent, f) {
	lb_error = new QLabel;
	lb_errorIcon = new QLabel;
	lb_contents = new QLabel ("LDraw code:", parent);
	
	le_contents = new QLineEdit (parent);
	le_contents->setWhatsThis ("The LDraw code of this object. The code written "
		"here is expected to be valid LDraw code, invalid code here results "
		"the object being turned into an error object. Please do refer to the "
		"<a href=\"http://www.ldraw.org/article/218.html\">official file format "
		"standard</a> for further information.");
	le_contents->setMinimumWidth (384);
	
	QHBoxLayout* bottomRow = new QHBoxLayout;
	bottomRow->addWidget (lb_errorIcon);
	bottomRow->addWidget (lb_error);
	bottomRow->addWidget (makeButtonBox (*this));
	
	QVBoxLayout* layout = new QVBoxLayout (this);
	layout->addWidget (lb_contents);
	layout->addWidget (le_contents);
	layout->addLayout (bottomRow);
	
	setWindowIcon (getIcon ("set-contents"));
}

void SetContentsDialog::setObject (LDObject* obj) {
	le_contents->setText (obj->raw ());
	
	if (obj->getType() == LDObject::Gibberish) {
		lb_error->setText (fmt ("<span style=\"color: #900\">%1</span>",
			static_cast<LDGibberish*> (obj)->reason));
		
		QPixmap errorPixmap = getIcon ("error").scaledToHeight (16);
		lb_errorIcon->setPixmap (errorPixmap);
	}
}

str SetContentsDialog::text () const {
	return le_contents->text ();
}

// =================================================================================================
LDrawPathDialog::LDrawPathDialog (const bool validDefault, QWidget* parent, Qt::WindowFlags f)
	: QDialog (parent, f), m_validDefault (validDefault)
{
	QLabel* lb_description = null;
	lb_resolution = new QLabel ("---");
	
	if (validDefault == false)
		lb_description = new QLabel ("Please input your LDraw directory");
	
	QLabel* lb_path = new QLabel ("LDraw path:");
	le_path = new QLineEdit;
	btn_findPath = new QPushButton;
	btn_findPath->setIcon (getIcon ("folder"));
	
	btn_cancel = new QPushButton;
	
	if (validDefault == false) {
		btn_cancel->setText ("Exit");
		btn_cancel->setIcon (getIcon ("exit"));
	} else {
		btn_cancel->setText ("Cancel");
		btn_cancel->setIcon (getIcon ("cancel"));
	}
	
	dbb_buttons = new QDialogButtonBox (QDialogButtonBox::Ok);
	dbb_buttons->addButton (btn_cancel, QDialogButtonBox::RejectRole);
	okButton ()->setEnabled (false);
	
	QHBoxLayout* inputLayout = new QHBoxLayout;
	inputLayout->addWidget (lb_path);
	inputLayout->addWidget (le_path);
	inputLayout->addWidget (btn_findPath);
	
	QVBoxLayout* mainLayout = new QVBoxLayout;
	
	if (validDefault == false)
		mainLayout->addWidget (lb_description);
	
	mainLayout->addLayout (inputLayout);
	mainLayout->addWidget (lb_resolution);
	mainLayout->addWidget (dbb_buttons);
	setLayout (mainLayout);
	
	connect (le_path, SIGNAL (textEdited (QString)), this, SLOT (slot_tryConfigure ()));
	connect (btn_findPath, SIGNAL (clicked ()), this, SLOT (slot_findPath ()));
	connect (dbb_buttons, SIGNAL (accepted ()), this, SLOT (accept ()));
	connect (dbb_buttons, SIGNAL (rejected ()), this, (validDefault) ? SLOT (reject ()) : SLOT (slot_exit ()));
	
	setPath (io_ldpath);
	if (validDefault)
		slot_tryConfigure ();
}

QPushButton* LDrawPathDialog::okButton () {
	return dbb_buttons->button (QDialogButtonBox::Ok);
}

void LDrawPathDialog::setPath (str path) {
	le_path->setText (path);
}

str LDrawPathDialog::filename () const {
	return le_path->text ();
}

void LDrawPathDialog::slot_findPath () {
	str newpath = QFileDialog::getExistingDirectory (this, "Find LDraw Path");
	
	if (newpath.length () > 0 && newpath != filename ()) {
		setPath (newpath);
		slot_tryConfigure ();
	}
}

void LDrawPathDialog::slot_exit () {
	exit (1);
}

void LDrawPathDialog::slot_tryConfigure () {
	if (LDPaths::tryConfigure (filename ()) == false) {
		lb_resolution->setText (fmt ("<span style=\"color:red; font-weight: bold;\">%1</span>",
			LDPaths::getError()));
		okButton ()->setEnabled (false);
		return;
	}
	
	lb_resolution->setText ("<span style=\"color: #7A0; font-weight: bold;\">OK!</span>");
	okButton ()->setEnabled (true);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
RotationPointDialog::RotationPointDialog (QWidget* parent, Qt::WindowFlags f) : QDialog (parent, f) {
	rb_rotpoint = new RadioBox ("Rotation Point", { "Object center", "Custom" }, 0, Qt::Vertical, this);
	connect (rb_rotpoint, SIGNAL (valueChanged (int)), this, SLOT (radioBoxChanged ()));
	
	gb_customPos = new QGroupBox ("Custom point", this);
	dsb_customX = new QDoubleSpinBox;
	dsb_customY = new QDoubleSpinBox;
	dsb_customZ = new QDoubleSpinBox;
	
	for (auto x : initlist<QDoubleSpinBox*> ({dsb_customX, dsb_customY, dsb_customZ}))
		x->setRange (-10000.0f, 10000.0f);
	
	QGridLayout* customLayout = new QGridLayout (gb_customPos);
	customLayout->setColumnStretch (1, 1);
	customLayout->addWidget (new QLabel ("X"),	0, 0);
	customLayout->addWidget (dsb_customX,			0, 1);
	customLayout->addWidget (new QLabel ("Y"),	1, 0);
	customLayout->addWidget (dsb_customY,			1, 1);
	customLayout->addWidget (new QLabel ("Z"),	2, 0);
	customLayout->addWidget (dsb_customZ,			2, 1);
	
	QVBoxLayout* layout = new QVBoxLayout (this);
	layout->addWidget (rb_rotpoint);
	layout->addWidget (gb_customPos);
	layout->addWidget (makeButtonBox (*this));
}

bool RotationPointDialog::custom () const {
	return rb_rotpoint->value () == 1;
}

vertex RotationPointDialog::customPos () const {
	return vertex (dsb_customX->value (), dsb_customY->value (), dsb_customZ->value ());
}

void RotationPointDialog::setCustom (bool custom) {
	rb_rotpoint->setValue (custom == true ? 1 : 0);
	gb_customPos->setEnabled (custom);
}

void RotationPointDialog::setCustomPos (const vertex& pos) {
	dsb_customX->setValue (pos[X]);
	dsb_customY->setValue (pos[Y]);
	dsb_customZ->setValue (pos[Z]);
}

void RotationPointDialog::radioBoxChanged () {
	setCustom (rb_rotpoint->value ());
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
OpenProgressDialog::OpenProgressDialog (QWidget* parent, Qt::WindowFlags f) : QDialog (parent, f) {
	progressBar = new QProgressBar;
	progressText = new QLabel( "Parsing..." );
	setNumLines (0);
	m_progress = 0;
	
	QDialogButtonBox* dbb_buttons = new QDialogButtonBox;
	dbb_buttons->addButton (QDialogButtonBox::Cancel);
	connect (dbb_buttons, SIGNAL (rejected ()), this, SLOT (reject ()));
	
	QVBoxLayout* layout = new QVBoxLayout (this);
	layout->addWidget (progressText);
	layout->addWidget (progressBar);
	layout->addWidget (dbb_buttons);
}

READ_ACCESSOR (ulong, OpenProgressDialog::numLines) {
	return m_numLines;
}

SET_ACCESSOR (ulong, OpenProgressDialog::setNumLines) {
	m_numLines = val;
	progressBar->setRange (0, numLines ());
	updateValues ();
}

void OpenProgressDialog::updateValues () {
	progressText->setText( fmt( "Parsing... %1 / %2", progress(), numLines() ));
	progressBar->setValue( progress() );
}

void OpenProgressDialog::updateProgress (int progress) {
	m_progress = progress;
	updateValues ();
}