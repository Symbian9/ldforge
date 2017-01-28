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

#include "openprogressdialog.h"
#include "ui_openprogressdialog.h"
#include "../main.h"

OpenProgressDialog::OpenProgressDialog (QWidget* parent, Qt::WindowFlags f) :
	QDialog (parent, f),
	ui (*new Ui_OpenProgressUI),
	m_progress (0),
	m_numLines (0)
{
	ui.setupUi (this);
	ui.progressText->setText ("Parsing...");
}

OpenProgressDialog::~OpenProgressDialog()
{
	delete &ui;
}

void OpenProgressDialog::setNumLines (int a)
{
	m_numLines = a;
	ui.progressBar->setRange (0, numLines());
	updateValues();
}

void OpenProgressDialog::updateValues()
{
	ui.progressText->setText (format ("Parsing... %1 / %2", progress(), numLines()));
	ui.progressBar->setValue (progress());
}

void OpenProgressDialog::setProgress (int progress)
{
	m_progress = progress;
	updateValues();
}