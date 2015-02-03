#pragma once
#include <QVector>
#include <QSharedPointer>

namespace Script
{
	using AstPointer = QSharedPointer<class AstNode>;

	enum AstNodeType
	{

	};

	class AstNode
	{
	public:
		AstNode (AstPointer parent);

	private:
		QVector<AstPointer> m_children;
		AstPointer m_parent;
	};
}
