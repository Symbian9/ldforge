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

#pragma once
#include <cstdio>
#include <cstdlib>
#include <QDate>
#include <QFile>
#include <QLineF>
#include <QMatrix4x4>
#include <QMetaType>
#include <QObject>
#include <QPointF>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVector>
#include <QVector3D>
#include "generics/functions.h"

#ifndef __GNUC__
# define __attribute__(X)
#endif

#define DEFINE_CLASS(SELF, SUPER) \
public: \
	using Self = SELF; \
	using Super = SUPER;

class LDObject;

enum Axis
{
	X,
	Y,
	Z
};

enum Winding
{
	NoWinding,
	CounterClockwise,
	Clockwise,
};

struct CircularSection
{
	int segments = 16;
	int divisions = 16;
};

Q_DECLARE_METATYPE(CircularSection)
bool operator==(const CircularSection& one, const CircularSection& other);
bool operator!=(const CircularSection& one, const CircularSection& other);

/*
 * Special operator definition that implements the XOR operator for windings.
 * However, if either winding is NoWinding, then this function returns NoWinding.
 */
inline Winding operator^(Winding one, Winding other)
{
	if (one == NoWinding or other == NoWinding)
		return NoWinding;
	else
		return static_cast<Winding>(static_cast<int>(one) ^ static_cast<int>(other));
}

inline Winding& operator^=(Winding& one, Winding other)
{
	one = one ^ other;
	return one;
}

static const double pi = 3.14159265358979323846;
static const double inf = std::numeric_limits<double>::infinity();

/*
 * Returns the norm of a vector.
 */
static inline qreal abs(const QVector3D &vector)
{
	return vector.length();
}

qreal determinant(qreal a, qreal b, qreal c, qreal d);
qreal determinant(qreal a, qreal b, qreal c, qreal d, qreal e, qreal f, qreal g, qreal h, qreal i);
qreal determinant(const QMatrix2x2& matrix);
qreal determinant(const QMatrix3x3& matrix);
qreal determinant(const QMatrix4x4& matrix);
QString formatFileSize(qint64 size);
int gcd(int a, int b);
QString joinStrings(const QList<class StringFormatArg>& values, QString delimeter = " ");
QString largeNumberRep(int number);
double log1000(double x);
double roundToDecimals(double value, int decimals);
class QSettings& settingsObject();
void simplify(int& numerator, int& denominator);
QString superscript(int number);
QString subscript(int number);
QString fractionRep(int numerator, int denominator);
qreal vectorAngle(const QVector3D& vec_1, const QVector3D& vec_2);
void withSignalsBlocked(QObject* object, std::function<void()> function);
