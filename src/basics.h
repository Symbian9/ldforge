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
#include <QObject>
#include <QStringList>
#include <QMetaType>
#include <QSet>
#include <QVector3D>
#include <QVector>
#include <QFile>
#include <QMatrix4x4>
#include <functional>
#include <math.h>
#include "macros.h"
#include "transform.h"

class Matrix;
using GLRotationMatrix = QMatrix4x4;

template<typename T, typename R>
using Pair = std::pair<T, R>;

enum Axis
{
	X,
	Y,
	Z
};

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
	Vertex	transformed(const GLRotationMatrix& matrix) const;
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

static inline qreal abs(const QVector3D &vector)
{
	return vector.length();
}

//
// Defines a bounding box that encompasses a given set of objects.
// vertex0 is the minimum vertex, vertex1 is the maximum vertex.
//
class BoundingBox
{
public:
	BoundingBox();

	void calcVertex (const Vertex& vertex);
	Vertex center() const;
	bool isEmpty() const;
	double longestMeasurement() const;
	void reset(); 
	const Vertex& vertex0() const;
	const Vertex& vertex1() const;

	BoundingBox& operator<< (const Vertex& v);

private:
	bool m_isEmpty;
	Vertex m_vertex0;
	Vertex m_vertex1;
};

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
		return Iterator(EnumLimits<Enum>::Last + 1);
	}
};

template<typename Enum>
EnumIterShell<Enum> iterateEnum()
{
	return EnumIterShell<Enum>();
}

// Is a value inside an enum?
template<typename Enum>
bool valueInEnum(Enum enumerator)
{
	typename std::underlying_type<Enum>::type index = static_cast<typename std::underlying_type<Enum>::type>(enumerator);
	return index >= EnumLimits<Enum>::First and index <= EnumLimits<Enum>::Last;
}

double getRadialPoint(int segment, int divisions, double(*func)(double));
QVector<QLineF> makeCircle(int segments, int divisions, double radius);
qreal distanceFromPointToRectangle(const QPointF& point, const QRectF& rectangle);

/*
 * Implements a ring adapter over T. This class corrects indices given to the element-operator so that they're within bounds.
 * The maximum amount can be specified manually.
 *
 * Example:
 *
 *   int A[] = {10,20,30,40};
 *   ring(A)[0]  ==     A[0 % 4]    == A[0]
 *   ring(A)[5]  ==     A[5 % 4]    == A[1]
 *   ring(A)[-1] == ring(A)[-1 + 4] == A[3]
 */
template<typename T>
class RingAdapter
{
private:
	// The private section must come first because _collection is used in decltype() below.
	T& _collection;
	const int _count;

public:
	RingAdapter(T& collection, int count) :
	    _collection {collection},
	    _count {count} {}

	template<typename IndexType>
	decltype(_collection[IndexType()]) operator[](IndexType index)
	{
		if (_count == 0)
		{
			// Argh! ...let the collection deal with this case.
			return _collection[0];
		}
		else
		{
			index %= _count;

			// Fix negative modulus...
			if (index < 0)
				index += _count;

			return _collection[index];
		}
	}

	int size() const
	{
		return _count;
	}
};

/*
 * Convenience function for RingAdapter so that the template parameter does not have to be provided. The ring amount is assumed
 * to be the amount of elements in the collection.
 */
template<typename T>
RingAdapter<T> ring(T& collection)
{
	return RingAdapter<T> {collection, countof(collection)};
}

/*
 * Version of ring() that allows manual specification of the count.
 */
template<typename T>
RingAdapter<T> ring(T& collection, int count)
{
	return RingAdapter<T> {collection, count};
}

//
// Get the amount of elements in something.
//
template<typename T, size_t N>
int countof(T(&)[N])
{
	return N;
}

static inline int countof(const QString& string)
{
	return string.length();
}

template<typename T>
int countof(const QVector<T>& vector)
{
	return vector.size();
}

template<typename T>
int countof(const QList<T>& vector)
{
	return vector.size();
}

template<typename T>
int countof(const QSet<T>& set)
{
	return set.size();
}

template<typename T>
int countof(const std::initializer_list<T>& vector)
{
	return vector.size();
}

template<typename T>
int countof(const RingAdapter<T>& ring)
{
	return ring.size();
}

/*
 * Extracts the sign of x.
 */
template<typename T>
T sign(T x)
{
	if (isZero(x))
		return {};
	else
		return x / qAbs(x);
}

/*
 * Returns the maximum of a single parameter (the parameter itself).
 */
template <typename T>
T max(T a)
{
	return a;
}

/*
 * Returns the maximum of two parameters.
 */
template <typename T>
T max(T a, T b)
{
	return a > b ? a : b;
}

/*
 * Returns the maximum of n parameters.
 */
template <typename T, typename... Rest>
T max(T a, Rest&&... rest)
{
	return max(a, max(rest...));
}

/*
 * Returns the minimum of a single parameter (the parameter itself).
 */
template <typename T>
T min(T a)
{
	return a;
}

/*
 * Returns the minimum of two parameters.
 */
template <typename T>
T min(T a, T b)
{
	return a < b ? a : b;
}

/*
 * Returns the minimum of n parameters.
 */
template <typename T, typename... Rest>
T min(T a, Rest&&... rest)
{
	return min(a, min(rest...));
}
