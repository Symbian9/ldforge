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
#include "modelobject.h"

/*
 * Represents a single code-4 quadrilateral.
 */
class LDQuadrilateral : public LDObject
{
public:
	static constexpr LDObjectType SubclassType = LDObjectType::Quadrilateral;

	LDQuadrilateral() = default;
	LDQuadrilateral(const Vertex& v1, const Vertex& v2, const Vertex& v3, const Vertex& v4);

	QString asText() const override;
	bool isCoPlanar() const;
	qreal planeAngle() const;
	int numVertices() const override;
	int triangleCount(DocumentManager*) const override;
	LDObjectType type() const override;
	QString iconName() const override;
};
