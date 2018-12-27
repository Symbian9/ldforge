/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2018 Teemu Piippo
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
#include "configuration.h"
#include "mainwindow.h"

class LDDocument;
class DocumentManager;
class PrimitiveManager;
class Grid;
class MainWindow;

//
// Objects that are to take part in the MainWindow's hierarchy multiple-inherit from this class to get a pointer back
// to the MainWindow class along with a few useful pointers and methods.
//
class HierarchyElement
{
public:
	HierarchyElement (QObject* parent);

	QSet<LDObject *> selectedObjects();
	LDDocument* currentDocument() const;
	PrimitiveManager* primitives();
	Grid* grid() const;

	// Utility functions
	QString preferredLicenseText() const;

protected:
	MainWindow* m_window;
	DocumentManager* m_documents;
};
