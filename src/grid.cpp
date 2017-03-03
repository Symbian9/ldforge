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

#include "grid.h"
#include "configuration.h"

Grid::Grid(QObject* parent) :
	HierarchyElement(parent) {}


qreal Grid::coordinateSnap() const
{
	switch (m_config->grid())
	{
	default:
	case Grid::Coarse: return m_config->gridCoarseCoordinateSnap();
	case Grid::Medium: return m_config->gridMediumCoordinateSnap();
	case Grid::Fine: return m_config->gridFineCoordinateSnap();
	}
}


qreal Grid::angleSnap() const
{
	switch (m_config->grid())
	{
	default:
	case Grid::Coarse: return m_config->gridCoarseAngleSnap();
	case Grid::Medium: return m_config->gridMediumAngleSnap();
	case Grid::Fine: return m_config->gridFineAngleSnap();
	}
}


qreal Grid::angleAsRadians() const
{
	return (pi * angleSnap()) / 180;
}


int Grid::bezierCurveSegments() const
{
	switch (m_config->grid())
	{
	default:
	case Grid::Coarse: return m_config->gridCoarseBezierCurveSegments();
	case Grid::Medium: return m_config->gridMediumBezierCurveSegments();
	case Grid::Fine: return m_config->gridFineBezierCurveSegments();
	}
}


qreal Grid::snap(qreal value) const
{
	// First, extract the amount of grid steps the value is away from zero, round that to remove the remainder,
	// and multiply back by the the grid size.
	double size = coordinateSnap();
	return round(value / size) * size;
}
