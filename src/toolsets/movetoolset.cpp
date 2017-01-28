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

#include "../ldDocument.h"
#include "../mathfunctions.h"
#include "../miscallenous.h"
#include "../mainwindow.h"
#include "movetoolset.h"
#include "ui_rotpoint.h"
#include "../grid.h"
#include "../glRenderer.h"

MoveToolset::MoveToolset (MainWindow* parent) :
	Toolset (parent) {}

void MoveToolset::moveSelection (bool up)
{
	LDObjectList objs = selectedObjects();
	if (objs.isEmpty())
		return;

	// If we move down, we need to iterate the array in reverse order.
	int start = up ? 0 : (countof(objs) - 1);
	int end = up ? countof(objs) : -1;
	int increment = up ? 1 : -1;
	QSet<LDObject*> objsToCompile;
	LDDocument* file = objs[0]->document();

	for (int i = start; i != end; i += increment)
	{
		LDObject* obj = objs[i];

		int idx = obj->lineNumber();
		int target = idx + (up ? -1 : 1);

		if ((up and idx == 0) or (not up and idx == countof(file->objects()) - 1))
		{
			// One of the objects hit the extrema. If this happens, this should be the first
			// object to be iterated on. Thus, nothing has changed yet and it's safe to just
			// abort the entire operation.
			return;
		}

		objsToCompile << obj;
		objsToCompile << file->getObject(target);
		obj->swap(file->getObject(target));
	}

	// The objects need to be recompiled, otherwise their pick lists are left with
	// the wrong index colors which messes up selection.
	for (LDObject* obj : objsToCompile)
		m_window->renderer()->compileObject(obj);

	m_window->buildObjectList();
}

void MoveToolset::moveUp()
{
	moveSelection (true);
}

void MoveToolset::moveDown()
{
	moveSelection (false);
}

void MoveToolset::gridCoarse()
{
	m_config->setGrid (Grid::Coarse);
	m_window->updateGridToolBar();
}

void MoveToolset::gridMedium()
{
	m_config->setGrid (Grid::Medium);
	m_window->updateGridToolBar();
}

void MoveToolset::gridFine()
{
	m_config->setGrid (Grid::Fine);
	m_window->updateGridToolBar();
}

void MoveToolset::moveObjects (Vertex vect)
{
	// Apply the grid values
	vect *= grid()->coordinateSnap();

	for (LDObject* obj : selectedObjects())
		obj->move (vect);
}

void MoveToolset::moveXNeg()
{
	moveObjects ({-1, 0, 0});
}

void MoveToolset::moveYNeg()
{
	moveObjects ({0, -1, 0});
}

void MoveToolset::moveZNeg()
{
	moveObjects ({0, 0, -1});
}

void MoveToolset::moveXPos()
{
	moveObjects ({1, 0, 0});
}

void MoveToolset::moveYPos()
{
	moveObjects ({0, 1, 0});
}

void MoveToolset::moveZPos()
{
	moveObjects ({0, 0, 1});
}

double MoveToolset::getRotateActionAngle()
{
	return (pi * grid()->angleSnap()) / 180;
}

void MoveToolset::rotateXPos()
{
	math()->rotateObjects (1, 0, 0, getRotateActionAngle(), selectedObjects());
}

void MoveToolset::rotateYPos()
{
	math()->rotateObjects (0, 1, 0, getRotateActionAngle(), selectedObjects());
}

void MoveToolset::rotateZPos()
{
	math()->rotateObjects (0, 0, 1, getRotateActionAngle(), selectedObjects());
}

void MoveToolset::rotateXNeg()
{
	math()->rotateObjects (-1, 0, 0, getRotateActionAngle(), selectedObjects());
}

void MoveToolset::rotateYNeg()
{
	math()->rotateObjects (0, -1, 0, getRotateActionAngle(), selectedObjects());
}

void MoveToolset::rotateZNeg()
{
	math()->rotateObjects (0, 0, -1, getRotateActionAngle(), selectedObjects());
}

void MoveToolset::configureRotationPoint()
{
	QDialog* dialog = new QDialog;
	Ui_RotPointUI ui;
	ui.setupUi(dialog);

	switch (RotationPoint(m_config->rotationPointType()))
	{
	case ObjectOrigin:
		ui.objectPoint->setChecked (true);
		break;

	case WorldOrigin:
		ui.worldPoint->setChecked (true);
		break;

	case CustomPoint:
		ui.customPoint->setChecked (true);
		break;
	}

	Vertex custompoint = m_config->customRotationPoint();
	ui.customX->setValue(custompoint.x());
	ui.customY->setValue(custompoint.y());
	ui.customZ->setValue(custompoint.z());

	if (dialog->exec() == QDialog::Accepted)
	{
		RotationPoint pointType;

		if (ui.objectPoint->isChecked())
			pointType = ObjectOrigin;
		else if (ui.objectPoint->isChecked())
			pointType = WorldOrigin;
		else
			pointType = CustomPoint;

		custompoint.setX (ui.customX->value());
		custompoint.setY (ui.customY->value());
		custompoint.setZ (ui.customZ->value());
		m_config->setRotationPointType((int) pointType);
		m_config->setCustomRotationPoint (custompoint);
	}
}