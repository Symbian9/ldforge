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
#include <QObject>
#include <QStringList>
#include <QMetaType>
#include <QVector3D>
#include <QVector>
#include <functional>
#include <math.h>
#include "macros.h"
#include "transform.h"
#include "types/matrix.h"

class LDObject;
class QFile;
class QTextStream;
class Matrix;
class LDDocument;

using int8 = qint8;
using int16 = qint16;
using int32 = qint32;
using int64 = qint64;
using uint8 = quint8;
using uint16 = quint16;
using uint32 = quint32;
using uint64 = quint64;
using LDObjectList = QList<LDObject*>;

template<typename T, typename R>
using Pair = std::pair<T, R>;

enum Axis
{
	X,
	Y,
	Z
};

static const Axis axes[] = {X, Y, Z};

//
// Derivative of QVector3D: this class is used for the vertices.
//
class Vertex : public QVector3D
{
public:
	using ApplyFunction = std::function<void (Axis, double&)>;
	using ApplyConstFunction = std::function<void (Axis, double)>;

	Vertex();
	Vertex (const QVector3D& a);
	Vertex (qreal xpos, qreal ypos, qreal zpos);

	void	apply (ApplyFunction func);
	void	apply (ApplyConstFunction func) const;
	QString	toString (bool mangled = false) const;
	void	transform (const Matrix& matr, const Vertex& pos);
	void	setCoordinate (Axis ax, qreal value);

	Vertex&	operator+= (const Vertex& other);
	Vertex	operator+ (const Vertex& other) const;
	Vertex&	operator*= (qreal scalar);
	Vertex	operator* (qreal scalar) const;
	bool	operator< (const Vertex& other) const;
	double	operator[] (Axis ax) const;
};

inline Vertex operator* (qreal scalar, const Vertex& vertex)
{
	return vertex * scalar;
}

Q_DECLARE_METATYPE (Vertex)
uint qHash(const Vertex& key);

//
// Defines a bounding box that encompasses a given set of objects.
// vertex0 is the minimum vertex, vertex1 is the maximum vertex.
//
class BoundingBox
{
public:
	BoundingBox();

	void calcObject (LDObject* obj);
	void calcVertex (const Vertex& vertex);
	Vertex center() const;
	bool isEmpty() const;
	double longestMeasurement() const;
	void reset(); 
	const Vertex& vertex0() const;
	const Vertex& vertex1() const;

	BoundingBox& operator<< (LDObject* obj);
	BoundingBox& operator<< (const Vertex& v);

private:
	bool m_isEmpty;
	Vertex m_vertex0;
	Vertex m_vertex1;
};

extern const Vertex Origin;
extern const Matrix IdentityMatrix;

static const double pi = 3.14159265358979323846;


// =============================================================================
// Plural expression
template<typename T>
static inline const char* plural (T n)
{
	return (n != 1) ? "s" : "";
}

template<typename T>
bool isZero (T a)
{
	return qFuzzyCompare (a + 1.0, 1.0);
}

template<typename T>
bool isInteger (T a)
{
	return (qAbs (a - floor(a)) < 0.00001) or (qAbs (a - ceil(a)) < 0.00001);
}

//
// Returns true if first arg is equal to any of the other args
//
template<typename T, typename Arg, typename... Args>
bool isOneOf (T const& a, Arg const& arg, Args const&... args)
{
	if (a == arg)
		return true;

	return isOneOf (a, args...);
}

template<typename T>
bool isOneOf (T const&)
{
	return false;
}

inline void toggle (bool& a)
{
	a = not a;
}

//
// Iterates an enum
//
template<typename Enum>
struct EnumIterShell
{
	struct Iterator
	{
		Iterator(typename std::underlying_type<Enum>::type i) :
			i(i) {}

		Iterator& operator++() { ++i; return *this; }
		bool operator==(Iterator other) { return i == other.i; }
		bool operator!=(Iterator other) { return i != other.i; }
		Enum operator*() const { return Enum(i); }

		typename std::underlying_type<Enum>::type i;
	};

	Iterator begin()
	{
		return Iterator(EnumLimits<Enum>::First);
	};

	Iterator end()
	{
		return Iterator(EnumLimits<Enum>::End);
	}
};

template<typename Enum>
EnumIterShell<Enum> iterateEnum()
{
	return EnumIterShell<Enum>();
}

// Is a value inside an enum?
template<typename Enum>
bool valueInEnum(typename std::underlying_type<Enum>::type x)
{
	return x >= EnumLimits<Enum>::First and x <= EnumLimits<Enum>::Last;
}

double getRadialPoint(int segment, int divisions, double(*func)(double));
QVector<QLineF> makeCircle(int segments, int divisions, double radius);

//
// Get the amount of elements in something.
//
template<typename T, size_t N>
int length(T(&)[N])
{
	return N;
}

static inline int length(const QString& string)
{
	return string.size();
}

template<typename T>
int length(const QVector<T>& vector)
{
	return vector.size();
}
