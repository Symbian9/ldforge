/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2015 Teemu Piippo
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

// Prime number table.
static const int PrimeNumbers[] =
{
	2,    3,    5,    7,    11,   13,   17,   19,   23,   29,
	31,   37,   41,   43,   47,   53,   59,   61,   67,   71,
	73,   79,   83,   89,   97,   101,  103,  107,  109,  113,
	127,  131,  137,  139,  149,  151,  157,  163,  167,  173,
	179,  181,  191,  193,  197,  199,  211,  223,  227,  229,
	233,  239,  241,  251,  257,  263,  269,  271,  277,  281,
	283,  293,  307,  311,  313,  317,  331,  337,  347,  349,
	353,  359,  367,  373,  379,  383,  389,  397,  401,  409,
	419,  421,  431,  433,  439,  443,  449,  457,  461,  463,
	467,  479,  487,  491,  499,  503,  509,  521,  523,  541,
	547,  557,  563,  569,  571,  577,  587,  593,  599,  601,
	607,  613,  617,  619,  631,  641,  643,  647,  653,  659,
	661,  673,  677,  683,  691,  701,  709,  719,  727,  733,
	739,  743,  751,  757,  761,  769,  773,  787,  797,  809,
	811,  821,  823,  827,  829,  839,  853,  857,  859,  863,
	877,  881,  883,  887,  907,  911,  919,  929,  937,  941,
	947,  953,  967,  971,  977,  983,  991,  997, 1009, 1013,
	1019, 1021, 1031, 1033, 1039, 1049, 1051, 1061, 1063, 1069,
	1087, 1091, 1093, 1097, 1103, 1109, 1117, 1123, 1129, 1151,
	1153, 1163, 1171, 1181, 1187, 1193, 1201, 1213, 1217, 1223,
	1229, 1231, 1237, 1249, 1259, 1277, 1279, 1283, 1289, 1291,
	1297, 1301, 1303, 1307, 1319, 1321, 1327, 1361, 1367, 1373,
	1381, 1399, 1409, 1423, 1427, 1429, 1433, 1439, 1447, 1451,
	1453, 1459, 1471, 1481, 1483, 1487, 1489, 1493, 1499, 1511,
	1523, 1531, 1543, 1549, 1553, 1559, 1567, 1571, 1579, 1583,
	1597, 1601, 1607, 1609, 1613, 1619, 1621, 1627, 1637, 1657,
	1663, 1667, 1669, 1693, 1697, 1699, 1709, 1721, 1723, 1733,
	1741, 1747, 1753, 1759, 1777, 1783, 1787, 1789, 1801, 1811,
	1823, 1831, 1847, 1861, 1867, 1871, 1873, 1877, 1879, 1889,
	1901, 1907, 1913, 1931, 1933, 1949, 1951, 1973, 1979, 1987,
	1993, 1997, 1999, 2003, 2011, 2017, 2027, 2029, 2039, 2053,
	2063, 2069, 2081, 2083, 2087, 2089, 2099, 2111, 2113, 2129,
	2131, 2137, 2141, 2143, 2153, 2161, 2179, 2203, 2207, 2213,
	2221, 2237, 2239, 2243, 2251, 2267, 2269, 2273, 2281, 2287,
	2293, 2297, 2309, 2311, 2333, 2339, 2341, 2347, 2351, 2357,
	2371, 2377, 2381, 2383, 2389, 2393, 2399, 2411, 2417, 2423,
	2437, 2441, 2447, 2459, 2467, 2473, 2477, 2503, 2521, 2531,
	2539, 2543, 2549, 2551, 2557, 2579, 2591, 2593, 2609, 2617,
	2621, 2633, 2647, 2657, 2659, 2663, 2671, 2677, 2683, 2687,
	2689, 2693, 2699, 2707, 2711, 2713, 2719, 2729, 2731, 2741,
	2749, 2753, 2767, 2777, 2789, 2791, 2797, 2801, 2803, 2819,
	2833, 2837, 2843, 2851, 2857, 2861, 2879, 2887, 2897, 2903,
	2909, 2917, 2927, 2939, 2953, 2957, 2963, 2969, 2971, 2999,
	3001, 3011, 3019, 3023, 3037, 3041, 3049, 3061, 3067, 3079,
	3083, 3089, 3109, 3119, 3121, 3137, 3163, 3167, 3169, 3181,
	3187, 3191, 3203, 3209, 3217, 3221, 3229, 3251, 3253, 3257,
	3259, 3271, 3299, 3301, 3307, 3313, 3319, 3323, 3329, 3331,
	3343, 3347, 3359, 3361, 3371, 3373, 3389, 3391, 3407, 3413,
	3433, 3449, 3457, 3461, 3463, 3467, 3469, 3491, 3499, 3511,
	3517, 3527, 3529, 3533, 3539, 3541, 3547, 3557, 3559, 3571,
};

// =============================================================================
//
// Grid stuff
//
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
	case Grid::Coarse: return Config->gridCoarseCoordinateSnap();
	case Grid::Medium: return Config->gridMediumCoordinateSnap();
	case Grid::Fine: return Config->gridFineCoordinateSnap();
	}

	return 1.0f;
}

float gridAngleSnap()
{
	switch (Config->grid())
	{
	case Grid::Coarse: return Config->gridCoarseAngleSnap();
	case Grid::Medium: return Config->gridMediumAngleSnap();
	case Grid::Fine: return Config->gridFineAngleSnap();
	}

	return 45.0f;
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

// =============================================================================
//
// Snap the given coordinate value on the current grid's given axis.
//
double Grid::Snap (double value, const Grid::Config type)
{
	double snapvalue = (type == Coordinate) ? gridCoordinateSnap() : gridAngleSnap();
	double mult = floor (qAbs<double> (value / snapvalue));
	double out = mult * snapvalue;

	if (qAbs (value) - (mult * snapvalue) > snapvalue / 2)
		out += snapvalue;

	if (value < 0)
		out = -out;

	return out;
}

// =============================================================================
//
void Simplify (int& numer, int& denom)
{
	bool repeat;

	do
	{
		repeat = false;

		for (int x = 0; x < countof (PrimeNumbers); x++)
		{
			int const prime = PrimeNumbers[x];

			if (numer < prime and denom < prime)
				break;

			if ((numer % prime == 0) and (denom % prime == 0))
			{
				numer /= prime;
				denom /= prime;
				repeat = true;
				break;
			}
		}
	} while (repeat);
}

// =============================================================================
//
Vertex GetRotationPoint (const LDObjectList& objs)
{
	switch (RotationPoint (Config->rotationPointType()))
	{
	case RotationPoint::ObjectOrigin:
		{
			LDBoundingBox box;

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

// =============================================================================
//
void ConfigureRotationPoint()
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

// =============================================================================
//
QString Join (QList<StringFormatArg> vals, QString delim)
{
	QStringList list;

	for (const StringFormatArg& arg : vals)
		list << arg.text();

	return list.join (delim);
}

// =============================================================================
//
void RoundToDecimals (double& a, int decimals)
{
	static const long e10[] =
	{
		1l,
		10l,
		100l,
		1000l,
		10000l,
		100000l,
		1000000l,
		10000000l,
		100000000l,
		1000000000l,
	};

	if (decimals >= 0 and decimals < countof (e10))
		a = round (a * e10[decimals]) / e10[decimals];
}

// =============================================================================
//
void ApplyToMatrix (Matrix& a, ApplyToMatrixFunction func)
{
	for (int i = 0; i < 9; ++i)
		func (i, a[i]);
}

// =============================================================================
//
void ApplyToMatrix (const Matrix& a, ApplyToMatrixConstFunction func)
{
	for (int i = 0; i < 9; ++i)
		func (i, a[i]);
}

// =============================================================================
//
double GetCoordinateOf (const Vertex& a, Axis ax)
{
	switch (ax)
	{
		case X: return a.x();
		case Y: return a.y();
		case Z: return a.z();
	}

	return 0.0;
}


// =============================================================================
//
QString MakePrettyFileSize (qint64 size)
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
