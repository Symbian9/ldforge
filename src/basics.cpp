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


QString joinStrings(const QList<StringFormatArg>& values, QString delimeter)
{
	QStringList list;

	for (const StringFormatArg& arg : values)
		list << arg.text();

	return list.join(delimeter);
}


double roundToDecimals(double value, int decimals)
{
	if (decimals == 0)
	{
		return round(value);
	}
	else if (decimals > 0)
	{
		qreal coefficient = pow(10, decimals);
		return round(value * coefficient) / coefficient;
	}
}

QString formatFileSize(qint64 size)
{
	static const QString suffixes[] = {" bytes", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
	int magnitude = (size > 0) ? floor(log10(size) / 3.0 + 1e-10) : 0;
	magnitude = qBound(0, magnitude, countof(suffixes) - 1);
	return QString::number(size / pow(1000, magnitude), 'f', 3) + suffixes[magnitude];
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
