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
 * Represents a single code-3 triangle in the LDraw code file.
 */
class LDTriangle : public LDObject
{
public:
	static constexpr LDObjectType SubclassType = LDObjectType::Triangle;

	LDTriangle() = default;
	LDTriangle(Vertex const& v1, Vertex const& v2, Vertex const& v3);

	virtual LDObjectType type() const override
	{
		return LDObjectType::Triangle;
	}

	virtual QString asText() const override;
	int triangleCount(DocumentManager*) const override;
	int numVertices() const override { return 3; }
	QString typeName() const override { return "triangle"; }
};
