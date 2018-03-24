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
#include <QFile>
#include <QMatrix4x4>
#include <QMetaType>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTextFormat>
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

// TODO: get rid of this
#ifdef WIN32
# define DIRSLASH "\\"
# define DIRSLASH_CHAR '\\'
#else // WIN32
# define DIRSLASH "/"
# define DIRSLASH_CHAR '/'
#endif // WIN32

class LDObject;
using GLRotationMatrix = QMatrix4x4;

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

/*
 * Returns the norm of a vector.
 */
static inline qreal abs(const QVector3D &vector)
{
	return vector.length();
}

QString formatFileSize(qint64 size);
int gcd(int a, int b);
QString joinStrings(const QList<class StringFormatArg>& values, QString delimeter = " ");
double roundToDecimals(double value, int decimals);
class QSettings& settingsObject();
void simplify(int& numerator, int& denominator);
