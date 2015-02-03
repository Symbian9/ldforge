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

#include "ast.h"

Script::Ast::BaseNode::BaseNode (NodePointer parent, NodeType type) :
	m_parent (parent),
	m_type (type)
{
}

Script::Ast::BaseNode::~BaseNode() {}

void Script::Ast::BaseNode::dump()
{
	static QString tabs;

	if (children().isEmpty())
	{
		print (tabs + describe() + "\n");
	}
	else
	{
		print (tabs + describe() + ":\n");

		for (NodePointer child : children())
		{
			tabs += '\t';
			child->dump();
			tabs.chop(1);
		}
	}
}

QString Script::Ast::RootNode::describe() const
{
	return "root";
}

QString Script::Ast::MacroNode::describe() const
{
	return format ("macro (%1)", m_macroName);
}