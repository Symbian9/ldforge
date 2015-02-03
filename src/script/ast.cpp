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