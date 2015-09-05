#include <stdio.h>
#include <QMetaObject>
#include "hierarchyelement.h"
#include "mainwindow.h"
#include "guiutilities.h"

HierarchyElement::HierarchyElement (QObject* parent) :
	m_window (nullptr),
	m_config (nullptr)
{
	if (parent)
	{
		while (parent->parent() != nullptr)
			parent = parent->parent();

		m_window = qobject_cast<MainWindow*> (parent);
	}

	if (m_window == nullptr)
	{
		m_window = g_win;
		print ("WARNING: Hierarchy element instance %p should be in the hierarchy of a "
			"MainWindow but isn't.\n", this);
	}

	m_config = m_window->configBag();
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
