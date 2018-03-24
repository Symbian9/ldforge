/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2018 Teemu Piippo
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

#include "../linetypes/modelobject.h"
#include "generateprimitivedialog.h"
#include "ui_generateprimitivedialog.h"

GeneratePrimitiveDialog::GeneratePrimitiveDialog (QWidget* parent, Qt::WindowFlags f) :
	QDialog (parent, f),
	ui(*new Ui_GeneratePrimitiveDialog)
{
	ui.setupUi (this);
	this->previousDivisions = ui.divisions->currentText().toInt();
	connect(
		ui.divisions,
		&QComboBox::currentTextChanged,
		[&]()
		{
			int divisions = ui.divisions->currentText().toInt();

			// Scale the segments value to fit.
			int newSegments = static_cast<int>(round(
				ui.segments->value() * double(divisions) / this->previousDivisions
			));

			ui.segments->setMaximum(divisions);
			ui.segments->setValue(newSegments);
			this->previousDivisions = divisions;
		}
	);
}


GeneratePrimitiveDialog::~GeneratePrimitiveDialog()
{
	delete &ui;
}


PrimitiveModel GeneratePrimitiveDialog::primitiveModel() const
{
	PrimitiveModel result;
	result.type =
		ui.typeCircle->isChecked()       ? PrimitiveModel::Circle :
		ui.typeCylinder->isChecked()     ? PrimitiveModel::Cylinder :
		ui.typeDisc->isChecked()         ? PrimitiveModel::Disc :
		ui.typeDiscNegative->isChecked() ? PrimitiveModel::DiscNegative :
		ui.typeRing->isChecked()         ? PrimitiveModel::Ring :
		                                   PrimitiveModel::Cone;
	result.divisions = ui.divisions->currentText().toInt();
	result.segments = ui.segments->value();
	result.ringNumber = ui.ringNumber->value();
	return result;
}
