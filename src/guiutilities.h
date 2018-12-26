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
#include "hierarchyelement.h"
#include "main.h"
#include "mainwindow.h"

/*
 * GuiUtilities
 *
 * Contains generic state-aware GUI functions.
 */
class GuiUtilities : public QObject, public HierarchyElement
{
	Q_OBJECT
public:
	explicit GuiUtilities (QObject* parent);

	void fillUsedColorsToComboBox (class QComboBox* box);
};

QColor mainColorRepresentation();
QIcon makeColorIcon(LDColor color, int size);
void setColorButton(class QPushButton* button, LDColor color);
void editObject(MainWindow* parent, LDObject* object);
