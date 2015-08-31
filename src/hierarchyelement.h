#pragma once
#include <QObject>

class MainWindow;
class ConfigurationValueBag;

//
// Objects that are to take part in the MainWindow's hierarchy multiple-inherit from this class to get a few useful
// pointer members.
//
class HierarchyElement
{
public:
	HierarchyElement (QObject* parent);

protected:
	MainWindow* m_window;
	ConfigurationValueBag* m_config;
};
