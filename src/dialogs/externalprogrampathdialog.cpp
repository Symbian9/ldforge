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
#include "externalprogrampathdialog.h"
#include "configdialog.h"
#include "ui_externalprogrampathdialog.h"
#include "../mainwindow.h"
#include "../glrenderer.h"

/*
 * Constructs a new external program path dialog.
 */
ExternalProgramPathDialog::ExternalProgramPathDialog(QString programName, QWidget* parent, Qt::WindowFlags flags) :
    QDialog {parent, flags},
    ui {*new Ui_ExtProgPath}
{
	ui.setupUi (this);
	QString labelText = ui.programLabel->text();
	labelText.replace("<PROGRAM>", programName);
	ui.programLabel->setText(labelText);
	connect(ui.findPathButton, SIGNAL(clicked (bool)), this, SLOT(findPath()));
}

/*
 * Destructs the UI pointer when the dialog is deleted.
 */
ExternalProgramPathDialog::~ExternalProgramPathDialog()
{
	delete &ui;
}

/*
 * Handler for the button in the dialog, shows a modal dialog for the user to locate the program.
 */
void ExternalProgramPathDialog::findPath()
{
	QString path = QFileDialog::getOpenFileName(nullptr, "", "", ConfigDialog::externalProgramPathFilter);

	if (not path.isEmpty())
		ui.path->setText(path);
}

/*
 * Returns the path specified by the user in this dialog.
 */
QString ExternalProgramPathDialog::path() const
{
	return ui.path->text();
}
