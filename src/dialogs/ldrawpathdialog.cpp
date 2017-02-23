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
#include <QPushButton>
#include <QLabel>
#include "ldrawpathdialog.h"
#include "ui_ldrawpathdialog.h"
#include "../mainwindow.h"

LDrawPathDialog::LDrawPathDialog (const QString& defaultPath, bool validDefault, QWidget* parent, Qt::WindowFlags f) :
	QDialog (parent, f),
	m_hasValidDefault (validDefault),
	ui (*new Ui_LDrawPathDialog)
{
	ui.setupUi (this);
	ui.status->setText ("---");

	if (validDefault)
		ui.heading->hide();
	else
	{
		cancelButton()->setText ("Exit");
		cancelButton()->setIcon (MainWindow::getIcon ("exit"));
	}

	okButton()->setEnabled (false);

	connect (ui.path, SIGNAL (textChanged (QString)), this, SIGNAL (pathChanged (QString)));
	connect (ui.searchButton, SIGNAL (clicked()), this, SLOT (searchButtonClicked()));
	connect (ui.buttonBox, SIGNAL (rejected()), this, SLOT (reject()));
	connect (ui.buttonBox, SIGNAL (accepted()), this, SLOT (accept()));
	setPath (defaultPath);
}

LDrawPathDialog::~LDrawPathDialog()
{
	delete &ui;
}

QPushButton* LDrawPathDialog::okButton()
{
	return ui.buttonBox->button (QDialogButtonBox::Ok);
}

QPushButton* LDrawPathDialog::cancelButton()
{
	return ui.buttonBox->button (QDialogButtonBox::Cancel);
}

void LDrawPathDialog::setPath (QString path)
{
	ui.path->setText (path);
}

QString LDrawPathDialog::path() const
{
	return ui.path->text();
}

void LDrawPathDialog::searchButtonClicked()
{
	QString newpath = QFileDialog::getExistingDirectory (this, "Find LDraw Path");

	if (not newpath.isEmpty())
		setPath (newpath);
}

void LDrawPathDialog::setStatusText (const QString& statusText, bool ok)
{
	okButton()->setEnabled (ok);

	if (statusText.isEmpty() and not ok)
	{
		ui.status->setText ("---");
	}
	else
	{
		ui.status->setText (QString ("<span style=\"color: %1\">%2</span>")
			.arg (ok ? "#270" : "#700")
			.arg (statusText));
	}
}