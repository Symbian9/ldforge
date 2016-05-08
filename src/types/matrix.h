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

#pragma once
#include <QString>

/**
 * @brief The Matrix class
 * A mathematical 3 x 3 matrix
 */
class Matrix
{
public:
	Matrix();
	Matrix (const std::initializer_list<double>& values);
	Matrix (double fillval);
	Matrix (double values[]);

	double determinant() const;
	Matrix multiply (const Matrix& other) const;
	void dump() const;
	QString toString() const;
	void zero();
	bool operator==(const Matrix& other) const;
	bool operator!=(const Matrix& other) const;

	/// @returns a mutable reference to a value by @c idx.
	inline double& value (int idx) { return m_values[idx]; }

	/// @returns a const reference to a value by @c idx.
	inline const double& value (int idx) const { return m_values[idx]; }

	/// @returns this matrix multiplied by @c other.
	inline Matrix operator* (const Matrix& other) const { return multiply (other); }

	/// @returns a mutable reference to a value by @c idx.
	inline double& operator[] (int idx) { return value(idx); }

	/// @returns a const reference to a value by @c idx.
	inline const double& operator[] (int idx) const { return value (idx); }

private:
	double m_values[9];
};
