/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Santeri Piippo
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
#include "basics.h"
#include <QMap>
#include <QVector>

class MagicWand
{
	QMap<Vertex, QVector<LDObjectPtr>> _vertices;
	QVector<LDObjectPtr> _selection;

public:
	using BoundaryType = std::tuple<Vertex, Vertex>;
	enum MagicType
	{
		Set,
		Additive,
		Subtractive,
		InternalRecursion
	};

	MagicWand();
	void doMagic (LDObjectPtr obj, MagicType type);

private:
	void fillBoundaries (LDObjectPtr obj, QVector<BoundaryType>& boundaries, QVector<LDObjectPtr>& candidates);
};
