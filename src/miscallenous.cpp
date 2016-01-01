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
#include "ui_rotpoint.h"

ConfigOption (int Grid = 1)
ConfigOption (float GridCoarseCoordinateSnap = 5.0f)
ConfigOption (float GridCoarseAngleSnap = 45.0f)
ConfigOption (float GridCoarseBezierCurveSegments = 8)
ConfigOption (float GridMediumCoordinateSnap = 1.0f)
ConfigOption (float GridMediumAngleSnap = 22.5f)
ConfigOption (float GridMediumBezierCurveSegments = 16)
ConfigOption (float GridFineCoordinateSnap = 0.1f)
ConfigOption (float GridFineAngleSnap = 7.5f)
ConfigOption (float GridFineBezierCurveSegments = 32)
ConfigOption (int RotationPointType = 0)
ConfigOption (Vertex CustomRotationPoint = Origin)


float gridCoordinateSnap()
{
	switch (Config->grid())
	{
	default:
	case Grid::Coarse: return Config->gridCoarseCoordinateSnap();
	case Grid::Medium: return Config->gridMediumCoordinateSnap();
	case Grid::Fine: return Config->gridFineCoordinateSnap();
	}
}


float gridAngleSnap()
{
	switch (Config->grid())
	{
	default:
	case Grid::Coarse: return Config->gridCoarseAngleSnap();
	case Grid::Medium: return Config->gridMediumAngleSnap();
	case Grid::Fine: return Config->gridFineAngleSnap();
	}
}


float gridBezierCurveSegments()
{
	switch (Config->grid())
	{
	default:
	case Grid::Coarse: return Config->gridCoarseBezierCurveSegments();
	case Grid::Medium: return Config->gridMediumBezierCurveSegments();
	case Grid::Fine: return Config->gridFineBezierCurveSegments();
	}
}

// Snaps the given coordinate value on the current grid's given axis.
double snapToGrid (double value, const Grid::Config type)
{
	double snapvalue = (type == Grid::Coordinate) ? gridCoordinateSnap() : gridAngleSnap();
	double mult = floor (qAbs<double> (value / snapvalue));
	double out = mult * snapvalue;

	if (qAbs (value) - (mult * snapvalue) > snapvalue / 2)
		out += snapvalue;

	if (value < 0)
		out = -out;

	return out;
}


int gcd (int a, int b)
{
	if (a > 0 and b > 0)
	{
		while (a != b)
		{
			if (a < b)
				b -= a;
			else
				a -= b;
		}
	}

	return a;
}


void simplify (int& numer, int& denom)
{
	int factor = gcd (numer, denom);
	numer /= factor;
	denom /= factor;
}


Vertex getRotationPoint (const LDObjectList& objs)
{
	switch (RotationPoint (Config->rotationPointType()))
	{
	case RotationPoint::ObjectOrigin:
		{
			BoundingBox box;

			// Calculate center vertex
			for (LDObject* obj : objs)
			{
				if (obj->hasMatrix())
					box << static_cast<LDMatrixObject*> (obj)->position();
				else
					box << obj;
			}

			return box.center();
		}

	case RotationPoint::WorldOrigin:
		return Origin;

	case RotationPoint::CustomPoint:
		return Config->customRotationPoint();

	case RotationPoint::NumValues:
		break;
	}

	return Vertex();
}


void configureRotationPoint()
{
	QDialog* dlg = new QDialog;
	Ui::RotPointUI ui;
	ui.setupUi (dlg);

	switch (RotationPoint (Config->rotationPointType()))
	{
	case RotationPoint::ObjectOrigin:
		ui.objectPoint->setChecked (true);
		break;

	case RotationPoint::WorldOrigin:
		ui.worldPoint->setChecked (true);
		break;

	case RotationPoint::CustomPoint:
		ui.customPoint->setChecked (true);
		break;

	case RotationPoint::NumValues:
		break;
	}

	Vertex custompoint = Config->customRotationPoint();
	ui.customX->setValue (custompoint.x());
	ui.customY->setValue (custompoint.y());
	ui.customZ->setValue (custompoint.z());

	if (not dlg->exec())
		return;

	Config->setRotationPointType (int (
		(ui.objectPoint->isChecked()) ? RotationPoint::ObjectOrigin :
		(ui.worldPoint->isChecked())  ? RotationPoint::WorldOrigin :
		RotationPoint::CustomPoint));

	custompoint.setX (ui.customX->value());
	custompoint.setY (ui.customY->value());
	custompoint.setZ (ui.customZ->value());
	Config->setCustomRotationPoint (custompoint);
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
	if (decimals >= 0 and decimals < countof (factors))
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
