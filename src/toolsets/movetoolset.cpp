/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2015 Teemu Piippo
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
#include "../ldObjectMath.h"
#include "../miscallenous.h"
#include "../mainwindow.h"
#include "movetoolset.h"

MoveToolset::MoveToolset (MainWindow* parent) :
	Toolset (parent) {}

void MoveToolset::moveSelection (bool up)
{
	LDObjectList objs = selectedObjects();
	LDObject::moveObjects (objs, up);
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
	Config->setGrid (Grid::Coarse);
	m_window->updateGridToolBar();
}

void MoveToolset::gridMedium()
{
	Config->setGrid (Grid::Medium);
	m_window->updateGridToolBar();
}

void MoveToolset::gridFine()
{
	Config->setGrid (Grid::Fine);
	m_window->updateGridToolBar();
}

void MoveToolset::moveObjects (Vertex vect)
{
	// Apply the grid values
	vect *= gridCoordinateSnap();

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
	return (Pi * gridAngleSnap()) / 180;
}

void MoveToolset::rotateXPos()
{
	RotateObjects (1, 0, 0, getRotateActionAngle(), selectedObjects());
}

void MoveToolset::rotateYPos()
{
	RotateObjects (0, 1, 0, getRotateActionAngle(), selectedObjects());
}

void MoveToolset::rotateZPos()
{
	RotateObjects (0, 0, 1, getRotateActionAngle(), selectedObjects());
}

void MoveToolset::rotateXNeg()
{
	RotateObjects (-1, 0, 0, getRotateActionAngle(), selectedObjects());
}

void MoveToolset::rotateYNeg()
{
	RotateObjects (0, -1, 0, getRotateActionAngle(), selectedObjects());
}

void MoveToolset::rotateZNeg()
{
	RotateObjects (0, 0, -1, getRotateActionAngle(), selectedObjects());
}

void MoveToolset::configureRotationPoint()
{
	configureRotationPoint();
}