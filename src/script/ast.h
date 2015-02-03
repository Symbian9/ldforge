#pragma once
#include <QVector>
#include <QSharedPointer>
#include "../main.h"

namespace Script
{
	namespace Ast
	{
		using NodePointer = QSharedPointer<class BaseNode>;
		using RootPointer = QSharedPointer<class RootNode>;
		using MacroPointer = QSharedPointer<class MacroNode>;

		enum NodeType
		{
			ROOT,
			MACRO,
		};

		class BaseNode
		{
		public:
			BaseNode (NodePointer parent, NodeType type);
			virtual ~BaseNode() = 0;

			NodeType type() const { return m_type; }
			NodePointer parent() const { return m_parent; }
			QVector<NodePointer> children() const { return m_children; }
			void addChild (NodePointer child) { m_children.append (child); }
			void dump();
			virtual QString describe() const = 0;

		private:
			QVector<NodePointer> m_children;
			NodePointer m_parent;
			NodeType m_type;
		};

		class RootNode : public BaseNode
		{
		public:
			RootNode() : BaseNode (NodePointer(), ROOT) {}
			QString describe() const override;
		};

		class MacroNode : public BaseNode
		{
		public:
			MacroNode (NodePointer parent, QString macroName) :
				BaseNode (parent, MACRO),
				m_macroName (macroName) {}

			QString macroName() const { return m_macroName; }
			QString describe() const override;

		private:
			QString m_macroName;
		};

		template<typename T, typename... Args>
		QSharedPointer<T> spawn (NodePointer parent, Args&&... args)
		{
			static_assert (std::is_base_of<BaseNode, T>::value,
				"Script::Ast::spawn can only be used for AST types.");
			QSharedPointer<T> ast (new T (parent, args...));
			parent->addChild (ast);
			return ast;
		}

		inline RootPointer spawnRoot()
		{
			return RootPointer (new RootNode());
		}
	}
}
