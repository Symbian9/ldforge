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

#include "doublespinbox.h"

/*
 * Constructs a new double spin box. The locale is fixed to system default "C".
 */
DoubleSpinBox::DoubleSpinBox(QWidget* parent) :
	QDoubleSpinBox {parent}
{
	this->setLocale({"C"});
}

/*
 * Reimplementation of QDoubleSpinBox::textFromValue to remove trailing zeros.
 */
QString DoubleSpinBox::textFromValue(double value) const
{
	QString result = QDoubleSpinBox::textFromValue(value);

	if (result.contains("."))
	{
		// Remove trailing zeros
		while (result.endsWith("0"))
			result.chop(1);

		// Remove trailing decimal point if we just removed all the zeros.
		if (result.endsWith("."))
			result.chop(1);
	}

	return result;
}

/*
 * Reimplementation of QDoubleSpinBox::validate to fix the decimal point if the locale-specific
 * decimal point was used.
 */
QValidator::State DoubleSpinBox::validate(QString& input, int& pos) const
{
	input.replace(QLocale().decimalPoint(), this->locale().decimalPoint());
	return QDoubleSpinBox::validate(input, pos);
}
