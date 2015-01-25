#include "ast.h"

Script::AstNode::AstNode (QSharedPointer<AstNode> parent) :
	m_parent (parent)
{

}
