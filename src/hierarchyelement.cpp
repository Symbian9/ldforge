/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2016 Teemu Piippo
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

#include <stdio.h>
#include <QMetaObject>
#include "hierarchyelement.h"
#include "mainwindow.h"
#include "guiutilities.h"

HierarchyElement::HierarchyElement (QObject* parent) :
	m_window (nullptr)
{
	if (parent)
	{
		while (parent->parent())
			parent = parent->parent();

		m_window = qobject_cast<MainWindow*> (parent);
	}

	if (m_window == nullptr)
	{
		m_window = g_win;
		print ("WARNING: Hierarchy element instance %p should be in the hierarchy of a "
			"MainWindow but isn't.\n", this);
	}

	m_documents = m_window->documents();
}

GuiUtilities* HierarchyElement::guiUtilities() const
{
	return m_window->guiUtilities();
}

LDDocument* HierarchyElement::currentDocument()
{
	return m_window->currentDocument();
}

const LDObjectList& HierarchyElement::selectedObjects()
{
	return m_window->selectedObjects();
}
