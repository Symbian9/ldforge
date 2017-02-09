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

#include "comment.h"

LDComment::LDComment(Model* model) :
    LDObject {model} {}

LDComment::LDComment(QString text, Model* model) :
    LDObject {model},
    m_text {text} {}

LDObjectType LDComment::type() const
{
	return OBJ_Comment;
}

QString LDComment::typeName() const
{
	return "comment";
}

bool LDComment::isScemantic() const
{
	return false;
}

bool LDComment::isColored() const
{
	return false;
}

QString LDComment::text() const
{
	return m_text;
}

void LDComment::setText (QString value)
{
	changeProperty(&m_text, value);
}

QString LDComment::objectListText() const
{
	return text().simplified();
}

QString LDComment::asText() const
{
	return format ("0 %1", text());
}
