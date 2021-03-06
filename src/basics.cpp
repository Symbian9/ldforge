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

#include <QApplication>
#include <QSettings>
#include <QLineF>
#include "basics.h"
#include "types/vertex.h"
#include "format.h"
#include "version.h"

int gcd(int a, int b)
{
	while (b != 0)
	{
		int temp = a;
		a = b;
		b = temp % b;
	}

	return a;
}

void simplify(int& numerator, int& denominator)
{
	int factor = gcd(numerator, denominator);
	numerator /= factor;
	denominator /= factor;
}


QString joinStrings(const QVector<StringFormatArg>& values, QString delimeter)
{
	QStringList list;

	for (const StringFormatArg& arg : values)
		list << arg.text();

	return list.join(delimeter);
}


double roundToDecimals(double value, int decimals)
{
	decimals = max(0, decimals);

	if (decimals == 0)
	{
		return round(value);
	}
	else
	{
		qreal coefficient = pow(10, decimals);
		return round(value * coefficient) / coefficient;
	}
}

double log1000(double x)
{
	return log10(x) / 3.0;
}

/*
 * Returns a string representation of the provided file size.
 */
QString formatFileSize(qint64 size)
{
	static const QString suffixes[] = {
		QObject::tr("bytes"),
		QObject::tr("kB", "kilobytes"),
		QObject::tr("MB", "megabytes"),
		QObject::tr("GB", "gigabytes"),
		QObject::tr("TB", "terabytes"),
		QObject::tr("PB", "petabytes"),
		QObject::tr("EB", "exabytes"),
		QObject::tr("ZB", "zettabytes"),
		QObject::tr("YB", "yottabytes")
	};

	if (size == 1)
	{
		return QObject::tr("1 byte");
	}
	else if (size > 0)
	{
		// Find out which suffix to use by the use of 1000-base logarithm:
		int magnitude = static_cast<int>(floor(log1000(size)));
		// ensure that magnitude is within bounds (even if we somehow get 1000s of yottabytes)
		magnitude = qBound(0, magnitude, countof(suffixes) - 1);
		// prepare the representation
		return QString::number(size / pow(1000, magnitude), 'g', 3) + " " + suffixes[magnitude];
	}
	else
	{
		return QString::number(size) + " " + suffixes[0];
	}
}

/*
 * Returns a settings object that interfaces the ini file.
 */
QSettings& settingsObject()
{
	static QSettings settings {
		qApp->applicationDirPath() + "/" UNIXNAME ".ini",
		QSettings::IniFormat
	};
	return settings;
}

QString largeNumberRep(int number)
{
	if (number < 0)
	{
		return "-" + largeNumberRep(-number);
	}
	else
	{
		QString rep;

		while (number >= 1000)
		{
			rep = " " + QString::number(number % 1000);
			number /= 1000;
		}

		return QString::number(number) + rep;
	}
}

static const QString superscripts = "⁰¹²³⁴⁵⁶⁷⁸⁹";
static const QString subscripts = "₀₁₂₃₄₅₆₇₈₉";

static QString customNumberRep(int number, const QString& script, const QString& minus)
{
	if (number < 0)
	{
		return minus + customNumberRep(-number, script, minus);
	}
	else
	{
		QString rep = "";
		for (QChar character : QString::number(number))
		{
			if (character >= '0' and character <= '9')
				rep += script[character.unicode() - '0'];
			else
				rep += character;
		}
		return rep;
	}
}

QString superscript(int number)
{
	return customNumberRep(number, superscripts, "⁻");
}

QString subscript(int number)
{
	return customNumberRep(number, subscripts, "₋");
}

QString fractionRep(int numerator, int denominator)
{
	return superscript(numerator) + "⁄" + subscript(denominator);
}

/*
 * Temporarily blocks the signals of `object` and calls `function`. Returns whatever `function`
 * returns. Assumes that the function does not change the signal blocking status of `object`.
 */
void withSignalsBlocked(QObject* object, std::function<void()> function)
{
	bool wasBlockingSignals = object->signalsBlocked();
	object->blockSignals(true);
	function();
	object->blockSignals(wasBlockingSignals);
}

/*
 * Returns the angle between two vectors
 */
qreal vectorAngle(const QVector3D& vec_1, const QVector3D& vec_2)
{
	return acos(QVector3D::dotProduct(vec_1, vec_2) / vec_1.length() / vec_2.length());
}

/*
 * Computes the determinant of a 2×2 matrix with each variable passed in row-major order.
 */
qreal determinant(qreal a, qreal b, qreal c, qreal d)
{
	return a * d - b * c;
}

/*
 * Computes the determinant of a 3×3 matrix with each variable passed in row-major order.
 */
qreal determinant(qreal a, qreal b, qreal c, qreal d, qreal e, qreal f, qreal g, qreal h, qreal i)
{
	return a*e*i + b*f*g + c*d*h - a*f*h - b*d*i - c*e*g;
}

/*
 * Computes the determinant of a 2×2 matrix.
 */
qreal determinant(const QMatrix2x2& matrix)
{
	return matrix(0, 0) * matrix(1, 1) - matrix(0, 1) * matrix(1, 0);
}

/*
 * Computes the determinant of a 3×3 matrix.
 */
qreal determinant(const QMatrix3x3& matrix)
{
	return sum(
		+matrix(0, 0) * matrix(1, 1) * matrix(2, 2),
		-matrix(0, 0) * matrix(1, 2) * matrix(2, 1),
		-matrix(0, 1) * matrix(1, 0) * matrix(2, 2),
		+matrix(0, 1) * matrix(1, 2) * matrix(2, 0),
		+matrix(0, 2) * matrix(1, 0) * matrix(2, 1),
		-matrix(0, 2) * matrix(1, 1) * matrix(2, 0));
}

/*
 * Computes the determinant of a 4×4 matrix.
 */
qreal determinant(const QMatrix4x4& matrix)
{
	qreal sum = 0;

	for (int column : {0, 1, 2, 3})
	{
		int column_1 = (column >= 1) ? 0 : 1;
		int column_2 = (column >= 2) ? 1 : 2;
		int column_3 = (column >= 3) ? 2 : 3;
		sum += ((column % 1) ? -1 : 1) * determinant(
			matrix(1, column_1), matrix(1, column_2), matrix(1, column_3),
			matrix(2, column_1), matrix(2, column_2), matrix(2, column_3),
			matrix(3, column_1), matrix(3, column_2), matrix(3, column_3));
	}

	return sum;
}

bool operator==(const CircularSection& one, const CircularSection& other)
{
	return one.segments == other.segments and one.divisions == other.divisions;
}

bool operator!=(const CircularSection& one, const CircularSection& other)
{
	return not (one == other);
}
