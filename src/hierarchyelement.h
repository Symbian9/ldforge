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
	DocumentManager* m_documents;
};
