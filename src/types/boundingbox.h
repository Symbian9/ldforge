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
#include "vertex.h"

//
// Defines a bounding box that encompasses a given set of objects.
// vertex0 is the minimum vertex, vertex1 is the maximum vertex.
//
class BoundingBox
{
public:
	BoundingBox();

	void calcVertex(const Vertex& vertex);
	Vertex center() const;
	bool isEmpty() const;
	double longestMeasurement() const;
	void reset();
	const Vertex& vertex0() const;
	const Vertex& vertex1() const;

	BoundingBox& operator<<(const Vertex& v);

private:
	bool m_isEmpty;
	Vertex m_vertex0;
	Vertex m_vertex1;
};
