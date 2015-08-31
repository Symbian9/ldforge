/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2015 Teemu Piippo
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
#include "../ldDocument.h"
#include "../mainwindow.h"
#include "newpartdialog.h"
#include "ui_newpartdialog.h"

NewPartDialog::NewPartDialog (QWidget *parent) :
	QDialog (parent),
	HierarchyElement (parent),
	ui (*new Ui_NewPart)
{
	ui.setupUi (this);

	QString authortext = m_config->defaultName();

	if (not m_config->defaultUser().isEmpty())
		authortext.append (format (" [%1]", m_config->defaultUser()));

	ui.author->setText (authortext);
	ui.useCaLicense->setChecked (m_config->useCaLicense());
}

BFCStatement NewPartDialog::getWinding() const
{
	if (ui.windingCcw->isChecked())
		return BFCStatement::CertifyCCW;

	if (ui.windingCw->isChecked())
		return BFCStatement::CertifyCW;

	return BFCStatement::NoCertify;
}

bool NewPartDialog::useCaLicense() const
{
	return ui.useCaLicense->isChecked();
}

QString NewPartDialog::author() const
{
	return ui.author->text();
}

QString NewPartDialog::title() const
{
	return ui.title->text();
}

void NewPartDialog::fillHeader (LDDocument* newdoc) const
{
	LDObjectList objs;
	objs << new LDComment (title());
	objs << new LDComment ("Name: <untitled>.dat");
	objs << new LDComment ("Author: " + author());
	objs << new LDComment ("!LDRAW_ORG Unofficial_Part");

	if (useCaLicense())
		objs << new LDComment (CALicenseText);
	
	objs << new LDEmpty();
	objs << new LDBFC (getWinding());
	objs << new LDEmpty();
	newdoc->addObjects (objs);
}
