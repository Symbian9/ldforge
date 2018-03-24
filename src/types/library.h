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
#include <QDataStream>
#include "generics/enums.h"

struct Library
{
	QString path;
	enum
	{
		ReadOnlyStorage, // for official files, etc
		UnofficialFiles, // put downloaded files here
		WorkingDirectory, // for editable documents
	} role = ReadOnlyStorage;

	bool operator==(const Library& other) const
	{
		return (this->path == other.path) and (this->role == other.role);
	}
};

Q_DECLARE_METATYPE(Library)
using Libraries = QVector<Library>;

inline QDataStream& operator<<(QDataStream& out, const Library& library)
{
	return out << library.path << library.role;
}

inline QDataStream& operator>>(QDataStream &in, Library& library)
{
	return in >> library.path >> enum_cast(library.role);
}
