/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2016 Teemu Piippo
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

/**
 * @brief Matrix::Matrix
 * Default-constructor for a matrix -- does not actually initialize the member values.
 */
Matrix::Matrix() {}

/**
 * @brief Matrix::Matrix
 * @param values
 * Initializes the matrix from a C array
 * Note: the array must have (at least) 9 values!
 */
Matrix::Matrix (double values[])
{
	for (int i = 0; i < 9; ++i)
		m_values[i] = values[i];
}

/**
 * @brief Matrix::Matrix
 * @param fillvalue
 * Constructs a matrix from a single fill value.
 */
Matrix::Matrix (double fillvalue)
{
	for (int i = 0; i < 9; ++i)
		m_values[i] = fillvalue;
}

/**
 * @brief Matrix::Matrix
 * @param values
 * Constructs a matrix from an initializer list.
 * Note: the initializer list must have exactly 9 values.
 */
Matrix::Matrix (const std::initializer_list<double>& values)
{
	if (length(values) == 9)
		memcpy (&m_values[0], values.begin(), sizeof m_values);
}

/**
 * @brief Matrix::dump
 * Prints the matrix out.
 */
void Matrix::dump() const
{
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
			print ("%1\t", m_values[i * 3 + j]);

		print ("\n");
	}
}

/**
 * @brief Matrix::toString
 * @return a string representation of the matrix
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

/**
 * @brief Matrix::zero
 * Fills the matrix with zeros
 */
void Matrix::zero()
{
	memset (&m_values[0], 0, sizeof m_values);
}

/**
 * @brief Matrix::multiply
 * @param other
 * @return this matrix multiplied with @c other.
 * @note @c a.mult(b) is not equivalent to @c b.mult(a)
 */
Matrix Matrix::multiply (const Matrix& other) const
{
	Matrix result;
	result.zero();

	for (int i = 0; i < 3; ++i)
	for (int j = 0; j < 3; ++j)
	for (int k = 0; k < 3; ++k)
		result[(i * 3) + j] += m_values[(i * 3) + k] * other[(k * 3) + j];

	return result;
}

/**
 * @brief Matrix::determinant
 * @return the matrix's determinant
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

/**
 * @brief Matrix::operator ==
 * @param other
 * @return whether the two matrices are equal
 */
bool Matrix::operator==(const Matrix& other) const
{
	for (int i = 0; i < length(m_values); ++i)
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

