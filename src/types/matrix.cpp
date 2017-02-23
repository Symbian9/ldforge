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

#include "../basics.h"
#include "../format.h"
#include "matrix.h"

const Matrix Matrix::identity {1, 0, 0, 0, 1, 0, 0, 0, 1};

/*
 * Default-constructor for a matrix
 */
Matrix::Matrix() :
    m_values{0} {}

/*
 * Constructs a matrix from a single fill value.
 */
Matrix::Matrix (double fillvalue) :
    m_values {fillvalue} {}

/*
 * Constructs a matrix from an initializer list.
 * Note: the initializer list must have exactly 9 values.
 */
Matrix::Matrix (const std::initializer_list<double>& values)
{
	int i = 0;

	for (double value : values)
	{
		if (i < 9)
			m_values[i++] = value;
		else
			break;
	}
}

/*
 * Returns a string representation of the matrix
 */
QString Matrix::toString() const
{
	QString val;

	for (int i = 0; i < 9; ++i)
	{
		if (i > 0)
			val += ' ';

		val += QString::number (m_values[i]);
	}

	return val;
}

/*
 * Fills the matrix with zeros
 */
void Matrix::zero()
{
	for (double& value : m_values)
		value = 0;
}

/*
 * Performs matrix multiplication.
 * Note: A*B is not equivalent to B*A!
 */
Matrix Matrix::multiply (const Matrix& other) const
{
	Matrix result;

	for (int i = 0; i < 3; ++i)
	for (int j = 0; j < 3; ++j)
	for (int k = 0; k < 3; ++k)
		result(i, j) += (*this)(i, k) * other(k, j);

	return result;
}

/*
 * Returns the matrix's determinant
 */
double Matrix::determinant() const
{
	return (value (0) * value (4) * value (8)) +
		   (value (1) * value (5) * value (6)) +
		   (value (2) * value (3) * value (7)) -
		   (value (2) * value (4) * value (6)) -
		   (value (1) * value (3) * value (8)) -
		   (value (0) * value (5) * value (7));
}

/*
 * Returns a value in matrix value (index math must be done manually)
 */
double& Matrix::value(int index)
{
	return m_values[index];
}

/*
 * Returns a value in matrix value (index math must be done manually)
 */
const double& Matrix::value(int index) const
{
	return m_values[index];
}

/*
 * Performs matrix multiplication
 */
Matrix Matrix::operator*(const Matrix &other) const
{
	return multiply(other);
}

/*
 * Returns a row of the matrix
 */
Matrix::RowView Matrix::operator[](int row)
{
	return RowView {*this, row};
}

/*
 * Returns a row of the matrix
 */
Matrix::ConstRowView Matrix::operator[](int row) const
{
	return ConstRowView {*this, row};
}

/*
 * Checks whether the two matrices are equal
 */
bool Matrix::operator==(const Matrix& other) const
{
	for (int i = 0; i < countof(m_values); ++i)
	{
		if (not qFuzzyCompare(m_values[i], other.m_values[i]))
			return false;
	}
	return true;
}

/**
 * @brief Matrix::operator !=
 * @param other
 * @return whether the two matrices are not equal
 */
bool Matrix::operator!=(const Matrix& other) const
{
	return not operator==(other);
}

double& Matrix::operator()(int row, int column)
{
	return m_values[row * 3 + column];
}

const double& Matrix::operator()(int row, int column) const
{
	return m_values[row * 3 + column];
}

double* Matrix::begin()
{
	return m_values;
}

const double* Matrix::begin() const
{
	return m_values;
}

double* Matrix::end()
{
	return m_values + countof(m_values);
}

const double* Matrix::end() const
{
	return m_values + countof(m_values);
}

Matrix::RowView::RowView(Matrix &matrix, int row) :
    _matrix {matrix},
    _row {row} {}

double& Matrix::RowView::operator[](int column)
{
	return _matrix.value(_row * 3 + column);
}

Matrix& Matrix::RowView::matrix() const
{
	return _matrix;
}

int Matrix::RowView::row()
{
	return _row;
}

Matrix::ConstRowView::ConstRowView(const Matrix &matrix, int row) :
    _matrix {matrix},
    _row {row} {}

const double& Matrix::ConstRowView::operator[](int column)
{
	return _matrix.value(_row * 3 + column);
}

const Matrix& Matrix::ConstRowView::matrix() const
{
	return _matrix;
}

int Matrix::ConstRowView::row()
{
	return _row;
}

Matrix Matrix::fromRotationMatrix(const GLRotationMatrix& rotationMatrix)
{
	Matrix result;

	for (int i = 0; i < 3; ++i)
	for (int j = 0; j < 3; ++j)
		result(i, j) = rotationMatrix(i, j);

	return result;
}
