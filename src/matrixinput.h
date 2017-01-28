/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2017 Teemu Piippo
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

#pragma once
#include "main.h"
#include <QDoubleSpinBox>

/*
 * Models a 3×3 array of spinboxes for matrix input.
 */
class MatrixInput : public QWidget
{
	Q_OBJECT

public:
	explicit MatrixInput(QWidget *parent = 0);
	int decimals() const;
	double maximum() const;
	double minimum() const;
	QString prefix() const;
	void setDecimals(int precision);
	void setMaximum(double maximum);
	void setMinimum(double minimum);
	void setPrefix(const QString& prefix);
	void setRange(double minimum, double maximum);
	void setSingleStep(double singleStep);
	void setSuffix(const QString& suffix);
	QString suffix();
	void setValue(const Matrix& value);
	Matrix value();

private:
	QDoubleSpinBox* _spinboxes[9];
};
