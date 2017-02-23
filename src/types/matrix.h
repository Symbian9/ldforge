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

#pragma once
#include <QString>
#include "../glShared.h"

/*
 * A mathematical 3 x 3 matrix
 */
class Matrix
{
public:
	class RowView;
	class ConstRowView;

	Matrix();
	Matrix (const std::initializer_list<double>& values);
	Matrix (double fillval);

	double* begin();
	const double* begin() const;
	double determinant() const;
	double* end();
	const double* end() const;
	Matrix multiply(const Matrix& other) const;
	QString toString() const;
	double& value(int index);
	const double& value(int index) const;
	void zero();

	bool operator==(const Matrix& other) const;
	bool operator!=(const Matrix& other) const;
	Matrix operator*(const Matrix& other) const;
	RowView operator[](int row);
	ConstRowView operator[](int row) const;
	double& operator()(int row, int column);
	const double& operator()(int row, int column) const;

	static const Matrix identity;
	static Matrix fromRotationMatrix(const GLRotationMatrix& rotationMatrix);

private:
	double m_values[9];
};

/*
 * A structure that provides a view into a row in a matrix.
 * This is returned by operator[] so that the matrix can be accessed by A[i][j]
 */
class Matrix::RowView
{
public:
	RowView(Matrix &matrix, int row);
	double& operator[](int column);
	Matrix& matrix() const;
	int row();

private:
	Matrix& _matrix;
	const int _row;
};

/*
 * Const version of the above
 */
class Matrix::ConstRowView
{
public:
	ConstRowView(const Matrix &matrix, int row);
	const double& operator[](int column);
	const Matrix& matrix() const;
	int row();

private:
	const Matrix& _matrix;
	const int _row;
};
