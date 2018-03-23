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

#include "boundingbox.h"

// =============================================================================
//
BoundingBox::BoundingBox()
{
	reset();
}

// =============================================================================
//
BoundingBox& BoundingBox::operator<< (const Vertex& v)
{
	calcVertex (v);
	return *this;
}

// =============================================================================
//
void BoundingBox::calcVertex (const Vertex& vertex)
{
	m_vertex0.x = qMin(vertex.x, m_vertex0.x);
	m_vertex0.y = qMin(vertex.y, m_vertex0.y);
	m_vertex0.z = qMin(vertex.z, m_vertex0.z);
	m_vertex1.x = qMax(vertex.x, m_vertex1.x);
	m_vertex1.y = qMax(vertex.y, m_vertex1.y);
	m_vertex1.z = qMax(vertex.z, m_vertex1.z);
	m_isEmpty = false;
}

// =============================================================================
//
// Clears the bounding box
//
void BoundingBox::reset()
{
	m_vertex0 = {10000.0, 10000.0, 10000.0};
	m_vertex1 = {-10000.0, -10000.0, -10000.0};
	m_isEmpty = true;
}

// =============================================================================
//
// Returns the length of the bounding box on the longest measure.
//
double BoundingBox::longestMeasurement() const
{
	double xscale = m_vertex0.x - m_vertex1.x;
	double yscale = m_vertex0.y - m_vertex1.y;
	double zscale = m_vertex0.z - m_vertex1.z;
	double size = qMax(xscale, qMax(yscale, zscale));
	return qMax(qAbs(size / 2.0), 1.0);
}

// =============================================================================
//
// Yields the center of the bounding box.
//
Vertex BoundingBox::center() const
{
	return {
		(m_vertex0.x + m_vertex1.x) / 2,
		(m_vertex0.y + m_vertex1.y) / 2,
		(m_vertex0.z + m_vertex1.z) / 2
	};
}

bool BoundingBox::isEmpty() const
{
	return m_isEmpty;
}

const Vertex& BoundingBox::vertex0() const
{
	return m_vertex0;
}

const Vertex& BoundingBox::vertex1() const
{
	return m_vertex1;
}
