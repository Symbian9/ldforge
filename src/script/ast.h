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
