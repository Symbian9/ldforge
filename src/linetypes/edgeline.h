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
#include "../linetypes/modelobject.h"

/*
 * Represents a single code-2 line in the LDraw code file.
 */
class LDEdgeLine : public LDObject
{
public:
	static constexpr LDObjectType SubclassType = LDObjectType::EdgeLine;

	virtual LDObjectType type() const override
	{
		return SubclassType;
	}

	virtual QString asText() const override;
	virtual void invert() override;
	int numVertices() const override { return 2; }
	LDColor defaultColor() const override { return EdgeColor; }
	QString typeName() const override { return "line"; }

protected:
	friend class Model;
	LDEdgeLine (Model* model);
	LDEdgeLine (Vertex v1, Vertex v2, Model* model = nullptr);
};
