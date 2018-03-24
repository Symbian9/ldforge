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

#include <QLineEdit>
#include <QRadioButton>
#include <QCheckBox>
#include "../lddocument.h"
#include "newpartdialog.h"
#include "ui_newpartdialog.h"

NewPartDialog::NewPartDialog (QWidget *parent) :
	QDialog (parent),
	ui (*new Ui_NewPart)
{
	this->ui.setupUi (this);

	QString authortext = config::defaultName();

	if (not config::defaultUser().isEmpty())
		authortext.append(format(" [%1]", config::defaultUser()));

	this->ui.author->setText (authortext);
	this->ui.useCaLicense->setChecked (config::useCaLicense());
}

NewPartDialog::~NewPartDialog()
{
	delete &this->ui;
}

Winding NewPartDialog::winding() const
{
	if (this->ui.windingCcw->isChecked())
		return CounterClockwise;
	else if (this->ui.windingCw->isChecked())
		return Clockwise;
	else
		return NoWinding;
}

bool NewPartDialog::useCaLicense() const
{
	return this->ui.useCaLicense->isChecked();
}

QString NewPartDialog::author() const
{
	return this->ui.author->text();
}

QString NewPartDialog::description() const
{
	return this->ui.title->text();
}

void NewPartDialog::fillHeader (LDDocument* document) const
{
	document->header.description = this->description();
	document->header.type = LDHeader::Part;
	document->header.author = this->author();

	if (this->useCaLicense())
		document->header.license = LDHeader::CaLicense;
	else
		document->header.license = LDHeader::UnspecifiedLicense;

	document->setWinding(this->winding());
}
