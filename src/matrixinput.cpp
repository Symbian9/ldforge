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

#include <QGridLayout>
#include "matrixinput.h"

MatrixInput::MatrixInput(QWidget *parent) : QWidget(parent)
{
	QGridLayout* layout = new QGridLayout {this};
	setLayout(layout);

	for (int i = 0; i < 3; ++i)
	for (int j = 0; j < 3; ++j)
	{
		_spinboxes[i * 3 + j] = new QDoubleSpinBox {this};
		layout->addWidget(_spinboxes[i * 3 + j], i, j);
	}
}

int MatrixInput::decimals() const
{
	return _spinboxes[0]->decimals();
}

double MatrixInput::maximum() const
{
	return _spinboxes[0]->maximum();
}

double MatrixInput::minimum() const
{
	return _spinboxes[0]->minimum();
}

QString MatrixInput::prefix() const
{
	return _spinboxes[0]->prefix();
}

void MatrixInput::setDecimals(int precision)
{
	for (QDoubleSpinBox* spinbox : _spinboxes)
		spinbox->setDecimals(precision);
}

void MatrixInput::setMaximum(double maximum)
{
	for (QDoubleSpinBox* spinbox : _spinboxes)
		spinbox->setMaximum(maximum);
}

void MatrixInput::setMinimum(double minimum)
{
	for (QDoubleSpinBox* spinbox : _spinboxes)
		spinbox->setMinimum(minimum);
}

void MatrixInput::setPrefix(const QString& prefix)
{
	for (QDoubleSpinBox* spinbox : _spinboxes)
		spinbox->setPrefix(prefix);
}

void MatrixInput::setRange(double minimum, double maximum)
{
	for (QDoubleSpinBox* spinbox : _spinboxes)
		spinbox->setRange(minimum, maximum);
}

void MatrixInput::setSingleStep(double singleStep)
{
	for (QDoubleSpinBox* spinbox : _spinboxes)
		spinbox->setSingleStep(singleStep);
}

void MatrixInput::setSuffix(const QString& suffix)
{
	for (QDoubleSpinBox* spinbox : _spinboxes)
		spinbox->setSuffix(suffix);
}

QString MatrixInput::suffix()
{
	return _spinboxes[0]->suffix();
}

void MatrixInput::setValue(const Matrix& value)
{
	for (int i = 0; i < 3; ++i)
	for (int j = 0; j < 3; ++j)
		_spinboxes[i * 3 + j]->setValue(value(i, j));
}

Matrix MatrixInput::value()
{
	Matrix result;

	for (int i = 0; i < 3; ++i)
	for (int j = 0; j < 3; ++j)
		result(i, j) = _spinboxes[i * 3 + j]->value();

	return result;
}
