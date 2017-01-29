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

BfcStatement NewPartDialog::getWinding() const
{
	if (ui.windingCcw->isChecked())
		return BfcStatement::CertifyCCW;

	if (ui.windingCw->isChecked())
		return BfcStatement::CertifyCW;

	return BfcStatement::NoCertify;
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
	newdoc->emplace<LDComment>(title());
	newdoc->emplace<LDComment>("Name: <untitled>.dat");
	newdoc->emplace<LDComment>("Author: " + author());
	newdoc->emplace<LDComment>("!LDRAW_ORG Unofficial_Part");
	QString license = preferredLicenseText();

	if (not license.isEmpty())
		newdoc->emplace<LDComment>(license);

	newdoc->emplace<LDEmpty>();
	newdoc->emplace<LDBfc>(getWinding());
	newdoc->emplace<LDEmpty>();
}
