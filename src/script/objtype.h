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

#pragma once
#include "../main.h"

namespace Script
{
	class ObjectType
	{
	public:
		ObjectType();
		virtual QString asString() const = 0;
	};

	class BasicType : public ObjectType
	{
	public:
		enum Kind
		{
			VAR, // mixed
			INT,
			REAL,
			STRING,
			TYPE, // heh
			OBJECT,
		};

		QString asString() const override;
		Kind kind() const { return m_kind; }

	private:
		Kind m_kind;
	};

	class ContainerType : public ObjectType
	{
	public:
		enum Kind
		{
			ARRAY,
			TUPLE,
			MATRIX
		};

		ContainerType (Kind kind, int n1, int n2) :
			m_kind (kind), m_n1 (n1), m_n2 (n2) {}

		QString asString() const override;
		Kind kind() const { return m_kind; }
		int n1() const { return m_n1; }
		int n2() const { return m_n1; }
		QSharedPointer<ObjectType> elementType() const { return m_elementType; }

	private:
		Kind m_kind;
		QSharedPointer<ObjectType> m_elementType;
		int m_n1;
		int m_n2;
	};
}
