/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2015 Teemu Piippo
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

#include "objtype.h"


Script::Type::Type() {}

QString Script::ContainerType::asString() const
{
	switch (m_kind)
	{
	case ARRAY:
		return m_elementType->asString() + "[]";

	case TUPLE:
		return m_elementType->asString()
			+ "(" + QString::number(m_n1) + ")";

	case MATRIX:
		return m_elementType->asString()
			+ "(" + QString::number(m_n1)
			+ "," + QString::number(m_n2) + ")";
	}

	return "???";
}

QString Script::BasicType::asString() const
{
	static const char* names[] =
	{
		"var",
		"int",
		"real",
		"string",
		"type",
		"object",
	};

	return names[int (m_kind)];
}