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

#include "boundingbox.h"

BoundingBox::BoundingBox() {}

BoundingBox& BoundingBox::operator<<(const Vertex& vertex)
{
	consider(vertex);
	return *this;
}

void BoundingBox::consider(const Vertex& vertex)
{
	this->minimum.x = min(vertex.x, this->minimum.x);
	this->minimum.y = min(vertex.y, this->minimum.y);
	this->minimum.z = min(vertex.z, this->minimum.z);
	this->maximum.x = max(vertex.x, this->maximum.x);
	this->maximum.y = max(vertex.y, this->maximum.y);
	this->maximum.z = max(vertex.z, this->maximum.z);
	this->storedIsEmpty = false;
}

/*
 * Clears the bounding box
 */
void BoundingBox::clear()
{
	(*this)	= {};
}

/*
 * Returns the length of the bounding box on the longest measure.
 */
double BoundingBox::longestMeasure() const
{
	double dx = this->minimum.x - this->maximum.x;
	double dy = this->minimum.y - this->maximum.y;
	double dz = this->minimum.z - this->maximum.z;
	double size = max(dx, dy, dz);
	return max(abs(size / 2.0), 1.0);
}


/*
 * Yields the center of the bounding box.
 */
Vertex BoundingBox::center() const
{
	return {
		(this->minimum.x + this->maximum.x) / 2,
		(this->minimum.y + this->maximum.y) / 2,
		(this->minimum.z + this->maximum.z) / 2
	};
}

bool BoundingBox::isEmpty() const
{
	return this->storedIsEmpty;
}

/*
 * Returns the minimum vertex, the -X, -Y, -Z corner.
 */
const Vertex& BoundingBox::minimumVertex() const
{
	return this->minimum;
}

/*
 * Returns the maximum vertex, the +X, +Y, +Z corner.
 */
const Vertex& BoundingBox::maximumVertex() const
{
	return this->maximum;
}

/*
 * Returns the length of the bounding box's space diagonal.
 */
double BoundingBox::spaceDiagonal() const
{
	return distance(this->minimumVertex(), this->maximumVertex());
}
