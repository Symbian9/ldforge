/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 Santeri Piippo
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

#ifndef TYPES_H
#define TYPES_H

#include "common.h"

typedef unsigned int uint;
typedef short unsigned int ushort;
typedef long unsigned int ulong;

// Typedef out the _t suffices :)
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

template<class T> using initlist = std::initializer_list<T>;
using std::vector;

class matrix;

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// vertex
// 
// Vertex class. Not to be confused with LDVertex, which is a vertex used in an
// LDraw code file.
// =============================================================================
class vertex {
public:
	double x, y, z;
	
	vertex () {}
	vertex (double x, double y, double z) : x (x), y (y), z (z) {}
	
	// =========================================================================
	void move (vertex other) {
		x += other.x;
		y += other.y;
		z += other.z;
	}
	
	// =========================================================================
	vertex& operator+= (vertex other) {
		move (other);
		return *this;
	}
	
	// =========================================================================
	vertex operator/ (const double d) {
		vertex other (*this);
		return other /= d;
	}
	
	// =========================================================================
	vertex& operator/= (const double d) {
		x /= d;
		y /= d;
		z /= d;
		return *this;
	}
	
	// =========================================================================
	vertex operator- () const {
		return vertex (-x, -y, -z);
	}
	
	// =========================================================================
	double coord (const ushort n) const {
		return *(&x + n);
	}
	
	// =========================================================================
	// Midpoint between this vertex and another vertex.
	vertex midpoint (vertex& other);
	str stringRep (const bool mangled);
	void transform (matrix mMatrix, vertex pos);
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// matrix
// 
// A mathematical 3x3 matrix
// =============================================================================
class matrix {
public:
	double faValues[9];
	
	// Constructors
	matrix () {}
	matrix (std::vector<double> vals);
	matrix (double fVal);
	matrix (double a, double b, double c,
		double d, double e, double f,
		double g, double h, double i);
	
	matrix mult (matrix mOther);
	
	matrix& operator= (matrix mOther) {
		memcpy (&faValues[0], &mOther.faValues[0], sizeof faValues);
		return *this;
	}
	
	matrix operator* (matrix mOther) {
		return mult (mOther);
	}
	
	inline double& operator[] (const uint uIndex) {
		return faValues[uIndex];
	}
	
	void zero ();
	void testOutput ();
	str stringRep ();
};

#endif // TYPES_H