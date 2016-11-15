/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2016 Teemu Piippo
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
#include "mainwindow.h"
#include "glRenderer.h"
#include "documentation.h"
#include "ldDocument.h"
#include "dialogs.h"
#include "ui_overlay.h"
#include "ui_extprogpath.h"

extern const char* g_extProgPathFilter;

// =============================================================================
// =============================================================================
OverlayDialog::OverlayDialog (QWidget* parent, Qt::WindowFlags f) : QDialog (parent, f)
{
	ui = new Ui_OverlayUI;
	ui->setupUi (this);

	m_cameraArgs =
	{
		{ ui->top,    TopCamera },
		{ ui->bottom, BottomCamera },
		{ ui->front,  FrontCamera },
		{ ui->back,   BackCamera },
		{ ui->left,   LeftCamera },
		{ ui->right,  RightCamera }
	};

	Camera cam = g_win->renderer()->camera();

	if (cam == FreeCamera)
		cam = TopCamera;

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
	LDGLOverlay& info = g_win->renderer()->getOverlay (newcam);
	RadioDefault<int> (newcam, m_cameraArgs);

	if (info.image)
	{
		ui->filename->setText (info.fileName);
		ui->originX->setValue (info.offsetX);
		ui->originY->setValue (info.offsetY);
		ui->width->setValue (info.width);
		ui->height->setValue (info.height);
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
QString OverlayDialog::fpath() const
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
	return RadioSwitch<int> (TopCamera, m_cameraArgs);
}

void OverlayDialog::slot_fpath()
{
	ui->filename->setText (QFileDialog::getOpenFileName (nullptr, "Overlay image"));
}

void OverlayDialog::slot_help()
{
	showDocumentation (g_docs_overlays);
}

void OverlayDialog::slot_dimensionsChanged()
{
	bool enable = (ui->width->value() != 0) or (ui->height->value() != 0);
	ui->buttonBox->button (QDialogButtonBox::Ok)->setEnabled (enable);
}

// =============================================================================
// =============================================================================
ExtProgPathPrompt::ExtProgPathPrompt (QString progName, QWidget* parent, Qt::WindowFlags f) :
	QDialog (parent, f),
	ui (new Ui_ExtProgPath)
{
	ui->setupUi (this);
	QString labelText = ui->m_label->text();
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
	QString path = QFileDialog::getOpenFileName (nullptr, "", "", g_extProgPathFilter);

	if (not path.isEmpty())
		ui->m_path->setText (path);
}

// =============================================================================
// =============================================================================
QString ExtProgPathPrompt::getPath() const
{
	return ui->m_path->text();
}
