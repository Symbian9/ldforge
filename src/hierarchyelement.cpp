#include <stdio.h>
#include <QMetaObject>
#include "hierarchyelement.h"
#include "mainwindow.h"

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
		fprintf (stderr, "FATAL ERROR: Hierarchy element instance %p should be in the hierarchy of a "
			"MainWindow but isn't.", this);
		abort();
	}
	else
	{
		m_config = m_window->configBag();
	}
}