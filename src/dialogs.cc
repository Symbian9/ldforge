/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Santeri Piippo
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
#include <QProgressBar>
#include <QCheckBox>
#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>
#include "dialogs.h"
#include "radioGroup.h"
#include "mainWindow.h"
#include "glRenderer.h"
#include "documentation.h"
#include "ldDocument.h"
#include "dialogs.h"
#include "ui_overlay.h"
#include "ui_ldrawpath.h"
#include "ui_openprogress.h"
#include "ui_extprogpath.h"
#include "ui_about.h"
#include "ui_bombbox.h"

extern const char* g_extProgPathFilter;
EXTERN_CFGENTRY (String, ldrawPath);

// =============================================================================
// =============================================================================
OverlayDialog::OverlayDialog (QWidget* parent, Qt::WindowFlags f) : QDialog (parent, f)
{
	ui = new Ui_OverlayUI;
	ui->setupUi (this);

	m_cameraArgs =
	{
		{ ui->top,    ETopCamera },
		{ ui->bottom, EBottomCamera },
		{ ui->front,  EFrontCamera },
		{ ui->back,   EBackCamera },
		{ ui->left,   ELeftCamera },
		{ ui->right,  ERightCamera }
	};

	ECamera cam = g_win->R()->camera();

	if (cam == EFreeCamera)
		cam = ETopCamera;

	connect (ui->width, SIGNAL (valueChanged (double)), this, SLOT (slot_dimensionsChanged()));
	connect (ui->height, SIGNAL (valueChanged (double)), this, SLOT (slot_dimensionsChanged()));
	connect (ui->buttonBox, SIGNAL (helpRequested()), this, SLOT (slot_help()));
	connect (ui->fileSearchButton, SIGNAL (clicked (bool)), this, SLOT (slot_fpath()));

	slot_dimensionsChanged();
	fillDefaults (cam);
}

// =============================================================================
// =============================================================================
OverlayDialog::~OverlayDialog()
{
	delete ui;
}

// =============================================================================
// =============================================================================
void OverlayDialog::fillDefaults (int newcam)
{
	LDGLOverlay& info = g_win->R()->getOverlay (newcam);
	radioDefault<int> (newcam, m_cameraArgs);

	if (info.img != null)
	{
		ui->filename->setText (info.fname);
		ui->originX->setValue (info.ox);
		ui->originY->setValue (info.oy);
		ui->width->setValue (info.lw);
		ui->height->setValue (info.lh);
	}
	else
	{
		ui->filename->setText ("");
		ui->originX->setValue (0);
		ui->originY->setValue (0);
		ui->width->setValue (0.0f);
		ui->height->setValue (0.0f);
	}
}

// =============================================================================
// =============================================================================
String OverlayDialog::fpath() const
{
	return ui->filename->text();
}

int OverlayDialog::ofsx() const
{
	return ui->originX->value();
}

int OverlayDialog::ofsy() const
{
	return ui->originY->value();
}

double OverlayDialog::lwidth() const
{
	return ui->width->value();
}

double OverlayDialog::lheight() const
{
	return ui->height->value();
}

int OverlayDialog::camera() const
{
	return radioSwitch<int> (ETopCamera, m_cameraArgs);
}

void OverlayDialog::slot_fpath()
{
	ui->filename->setText (QFileDialog::getOpenFileName (null, "Overlay image"));
}

void OverlayDialog::slot_help()
{
	showDocumentation (g_docs_overlays);
}

void OverlayDialog::slot_dimensionsChanged()
{
	bool enable = (ui->width->value() != 0) || (ui->height->value() != 0);
	ui->buttonBox->button (QDialogButtonBox::Ok)->setEnabled (enable);
}

// =============================================================================
// =============================================================================
LDrawPathDialog::LDrawPathDialog (const bool validDefault, QWidget* parent, Qt::WindowFlags f) :
	QDialog (parent, f),
	m_validDefault (validDefault)
{
	ui = new Ui_LDPathUI;
	ui->setupUi (this);
	ui->status->setText ("---");

	if (validDefault)
		ui->heading->hide();
	else
	{
		cancelButton()->setText ("Exit");
		cancelButton()->setIcon (getIcon ("exit"));
	}

	okButton()->setEnabled (false);

	connect (ui->path, SIGNAL (textEdited (QString)), this, SLOT (slot_tryConfigure()));
	connect (ui->searchButton, SIGNAL (clicked()), this, SLOT (slot_findPath()));
	connect (ui->buttonBox, SIGNAL (rejected()), this, validDefault ? SLOT (reject()) : SLOT (slot_exit()));
	connect (ui->buttonBox, SIGNAL (accepted()), this, SLOT (slot_accept()));

	setPath (cfg::ldrawPath);

	if (validDefault)
		slot_tryConfigure();
}

// =============================================================================
// =============================================================================
LDrawPathDialog::~LDrawPathDialog()
{
	delete ui;
}

QPushButton* LDrawPathDialog::okButton()
{
	return ui->buttonBox->button (QDialogButtonBox::Ok);
}

QPushButton* LDrawPathDialog::cancelButton()
{
	return ui->buttonBox->button (QDialogButtonBox::Cancel);
}

void LDrawPathDialog::setPath (String path)
{
	ui->path->setText (path);
}

String LDrawPathDialog::filename() const
{
	return ui->path->text();
}

// =============================================================================
// =============================================================================
void LDrawPathDialog::slot_findPath()
{
	String newpath = QFileDialog::getExistingDirectory (this, "Find LDraw Path");

	if (newpath.length() > 0 && newpath != filename())
	{
		setPath (newpath);
		slot_tryConfigure();
	}
}

// =============================================================================
// =============================================================================
void LDrawPathDialog::slot_exit()
{
	exit (0);
}

// =============================================================================
// =============================================================================
void LDrawPathDialog::slot_tryConfigure()
{
	if (not LDPaths::tryConfigure (filename()))
	{
		ui->status->setText (format ("<span style=\"color:#700; \">%1</span>", LDPaths::getError()));
		okButton()->setEnabled (false);
		return;
	}

	ui->status->setText ("<span style=\"color: #270; \">OK!</span>");
	okButton()->setEnabled (true);
}

// =============================================================================
// =============================================================================
void LDrawPathDialog::slot_accept()
{
	Config::save();
	accept();
}

// =============================================================================
// =============================================================================
OpenProgressDialog::OpenProgressDialog (QWidget* parent, Qt::WindowFlags f) : QDialog (parent, f)
{
	ui = new Ui_OpenProgressUI;
	ui->setupUi (this);
	ui->progressText->setText ("Parsing...");
	setNumLines (0);
	m_progress = 0;
}

// =============================================================================
// =============================================================================
OpenProgressDialog::~OpenProgressDialog()
{
	delete ui;
}

// =============================================================================
// =============================================================================
void OpenProgressDialog::setNumLines (int const& a)
{
	m_numLines = a;
	ui->progressBar->setRange (0, numLines());
	updateValues();
}

// =============================================================================
// =============================================================================
void OpenProgressDialog::updateValues()
{
	ui->progressText->setText (format ("Parsing... %1 / %2", progress(), numLines()));
	ui->progressBar->setValue (progress());
}

// =============================================================================
// =============================================================================
void OpenProgressDialog::updateProgress (int progress)
{
	setProgress (progress);
	updateValues();
}

// =============================================================================
// =============================================================================
ExtProgPathPrompt::ExtProgPathPrompt (String progName, QWidget* parent, Qt::WindowFlags f) :
	QDialog (parent, f),
	ui (new Ui_ExtProgPath)
{
	ui->setupUi (this);
	String labelText = ui->m_label->text();
	labelText.replace ("<PROGRAM>", progName);
	ui->m_label->setText (labelText);
	connect (ui->m_findPath, SIGNAL (clicked (bool)), this, SLOT (findPath()));
}

// =============================================================================
// =============================================================================
ExtProgPathPrompt::~ExtProgPathPrompt()
{
	delete ui;
}

// =============================================================================
// =============================================================================
void ExtProgPathPrompt::findPath()
{
	String path = QFileDialog::getOpenFileName (null, "", "", g_extProgPathFilter);

	if (not path.isEmpty())
		ui->m_path->setText (path);
}

// =============================================================================
// =============================================================================
String ExtProgPathPrompt::getPath() const
{
	return ui->m_path->text();
}

// =============================================================================
// =============================================================================
AboutDialog::AboutDialog (QWidget* parent, Qt::WindowFlags f) :
	QDialog (parent, f)
{
	Ui::AboutUI ui;
	ui.setupUi (this);
	ui.versionInfo->setText (APPNAME " " + String (fullVersionString()));

	QPushButton* mailButton = new QPushButton;
	mailButton->setText (tr ("Contact"));
	mailButton->setIcon (getIcon ("mail"));
	ui.buttonBox->addButton (static_cast<QAbstractButton*> (mailButton), QDialogButtonBox::HelpRole);
	connect (ui.buttonBox, SIGNAL (helpRequested()), this, SLOT (slot_mail()));

	setWindowTitle (format (tr ("About %1"), APPNAME));
}

// =============================================================================
// =============================================================================
void AboutDialog::slot_mail()
{
	QDesktopServices::openUrl (QUrl ("mailto:Santeri Piippo <arezey@gmail.com>?subject=LDForge"));
}

// =============================================================================
// =============================================================================
void bombBox (const String& message)
{
	QDialog dlg (g_win);
	Ui_BombBox ui;

	ui.setupUi (&dlg);
	ui.m_text->setText (message);
	ui.buttonBox->button (QDialogButtonBox::Close)->setText (QObject::tr ("Damn it"));
	dlg.exec();
}
