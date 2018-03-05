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

#include "serializer.h"
#include "linetypes/modelobject.h"

Serializer::Serializer(Archive& archive, Action action) :
	archive {archive},
	action {action} {}

Serializer::Archive Serializer::store(LDObject* object)
{
	Archive result;
	result.append(static_cast<int>(object->type()));
	Serializer serializer {result, Store};
	object->serialize(serializer);
	return result;
}

LDObject* Serializer::restore(Archive& archive)
{
	if (archive.size() > 0)
	{
		Serializer serializer {archive, Restore};
		int type;
		serializer << type;
		LDObject* object = LDObject::newFromType(static_cast<LDObjectType>(type));

		if (object)
			object->serialize(serializer);

		return object;
	}
	else
	{
		return nullptr;
	}
}

LDObject* Serializer::clone(LDObject* object)
{
	Archive archive = store(object);
	return restore(archive);
}
