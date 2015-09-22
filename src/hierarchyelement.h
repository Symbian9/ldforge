#pragma once
#include <QObject>
#include "main.h"

class MainWindow;
class ConfigurationValueBag;
class GuiUtilities;
class LDDocument;
class DocumentManager;

//
// Objects that are to take part in the MainWindow's hierarchy multiple-inherit from this class to get a few useful
// pointer members.
//
class HierarchyElement
{
public:
	HierarchyElement (QObject* parent);

	const LDObjectList& selectedObjects();
	LDDocument* currentDocument();
	GuiUtilities* guiUtilities() const;

protected:
	MainWindow* m_window;
	ConfigurationValueBag* m_config;
	DocumentManager* m_documents;
};
