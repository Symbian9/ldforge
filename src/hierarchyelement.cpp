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
		// Drat! It doesn't seem to have the MainWindow as a parent! We'll need to force it to be in one.
		// This shouldn't have any side effects but also shouldn't happen regardless.
		m_window = g_win;
		print("Hierarchy element instance %1 should have a MainWindow parent, but it is %2 (%3).\n", this,
		      parent, parent ? parent->metaObject()->className() : "nullptr");
	}

	m_documents = m_window->documents();
	m_config = m_window->config();
}


GuiUtilities* HierarchyElement::guiUtilities() const
{
	return m_window->guiUtilities();
}


LDDocument* HierarchyElement::currentDocument()
{
	return m_window->currentDocument();
}


const QSet<LDObject*>& HierarchyElement::selectedObjects()
{
	return m_window->selectedObjects();
}


PrimitiveManager* HierarchyElement::primitives()
{
	return m_window->primitives();
}


Grid* HierarchyElement::grid() const
{
	return m_window->grid();
}


MathFunctions* HierarchyElement::math() const
{
	return m_window->mathFunctions();
}


QString HierarchyElement::preferredLicenseText() const
{
	QString caLicenseText = "!LICENSE Redistributable under CCAL version 2.0 : see CAreadme.txt";
	return m_config->useCaLicense() ? caLicenseText : "";
}