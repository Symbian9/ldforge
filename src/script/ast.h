#pragma once
#include <QVector>
#include <QSharedPointer>

namespace Script
{
	enum AstNodeType
	{

	};

	class AstNode
	{
	public:
		AstNode (QSharedPointer<AstNode> parent);

	private:
		QVector<QSharedPointer<AstNode>> m_children;
		QSharedPointer<AstNode> m_parent;
	};
}
