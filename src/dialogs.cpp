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

#include <QFileDialog>
#include "dialogs.h"
#include "mainwindow.h"
#include "glRenderer.h"
#include "documentation.h"
#include "dialogs.h"
#include "ui_overlay.h"
#include "ui_extprogpath.h"

extern const char* g_extProgPathFilter;

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
