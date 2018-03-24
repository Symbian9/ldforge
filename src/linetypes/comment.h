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
#include "../linetypes/modelobject.h"

/*
 * Represents a line type 0 comment in the LDraw model.
 */
class LDComment : public LDObject
{
public:
	static const LDObjectType SubclassType = LDObjectType::Comment;

	LDComment(QString text = "");

	QString asText() const override;
	bool isColored() const override;
	bool isScemantic() const override;
	QString objectListText() const override;
	QString text() const;
	LDObjectType type() const override;
	QString typeName() const override;
	void setText(QString value);
	void serialize(class Serializer& serializer) override;

private:
	QString m_text;
};
