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

#include <math.h>
#include <locale.h>
#include <QColor>
#include "main.h"
#include "miscallenous.h"
#include "mainwindow.h"
#include "dialogs.h"
#include "ldDocument.h"


int gcd (int a, int b)
{
	while (b != 0)
	{
		int temp = a;
		a = b;
		b = temp % b;
	}

	return a;
}


void simplify (int& numerator, int& denominator)
{
	int factor = gcd(numerator, denominator);
	numerator /= factor;
	denominator /= factor;
}


QString joinStrings (QList<StringFormatArg> vals, QString delim)
{
	QStringList list;

	for (const StringFormatArg& arg : vals)
		list << arg.text();

	return list.join (delim);
}


void roundToDecimals (double& a, int decimals)
{
	static const double factors[] = { 1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9 };
	if (decimals >= 0 and decimals < countof(factors))
		a = round (a * factors[decimals]) / factors[decimals];
}


void applyToMatrix (Matrix& a, ApplyToMatrixFunction func)
{
	for (int i = 0; i < 9; ++i)
		func (i, a[i]);
}

void applyToMatrix (const Matrix& a, ApplyToMatrixConstFunction func)
{
	for (int i = 0; i < 9; ++i)
		func (i, a[i]);
}


QString formatFileSize (qint64 size)
{
	if (size < 1024LL)
		return QString::number (size) + " bytes";
	else if (size < (1024LL * 1024LL))
		return QString::number (double (size) / 1024LL, 'f', 1) + " Kb";
	else if (size < (1024LL * 1024LL * 1024LL))
		return QString::number (double (size) / (1024LL * 1024LL), 'f', 1) + " Mb";
	else
		return QString::number (double (size) / (1024LL * 1024LL * 1024LL), 'f', 1) + " Gb";
}
