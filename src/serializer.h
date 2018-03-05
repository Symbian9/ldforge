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
#include "main.h"

using LDObjectState = QVector<QVariant>;

class Serializer
{
public:
	using Archive = LDObjectState;

	enum Action
	{
		Store,
		Restore,
	};

	Serializer(Archive& archive, Action action);

	template<typename T>
	Serializer& operator<<(T& value);

	static Archive store(LDObject* object);
	static LDObject* restore(Archive& archive) __attribute__((warn_unused_result));
	static LDObject* clone(LDObject* object) __attribute__((warn_unused_result));

private:
	Archive& archive;
	int cursor = 0;
	Action action;
};

template<typename T>
Serializer& Serializer::operator<<(T& value)
{
	switch (action)
	{
	case Store:
		this->archive.append(QVariant::fromValue(value));
		break;

	case Restore:
		if (this->cursor < this->archive.size())
		{
			value = this->archive[this->cursor].value<T>();
			this->cursor += 1;
		}
		else
		{
			fprintf(stderr, "warning: archive ran out while restoring\n");
			value = T {};
		}
		break;
	}

	return *this;
}
