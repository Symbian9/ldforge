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

#include "../lddocument.h"
#include "../mathfunctions.h"
#include "../mainwindow.h"
#include "movetoolset.h"
#include "ui_rotationpointdialog.h"
#include "../grid.h"
#include "../canvas.h"

MoveToolset::MoveToolset (MainWindow* parent) :
	Toolset (parent) {}

void MoveToolset::moveSelection (bool up)
{
	for (const QItemSelectionRange& selectionRange : m_window->currentSelectionModel()->selection())
	{
		currentDocument()->moveRows(
			{},
			selectionRange.top(),
			selectionRange.height(),
			{},
			up ? (selectionRange.top() - 1) : (selectionRange.bottom() + 2)
		);
	}
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
	config::setGrid (Grid::Coarse);
	m_window->updateGridToolBar();
}

void MoveToolset::gridMedium()
{
	config::setGrid (Grid::Medium);
	m_window->updateGridToolBar();
}

void MoveToolset::gridFine()
{
	config::setGrid (Grid::Fine);
	m_window->updateGridToolBar();
}

void MoveToolset::polarGrid()
{
	config::togglePolarGrid();
	m_window->updateGridToolBar();
}

void MoveToolset::moveObjects(QVector3D vector)
{
	// Apply the grid values
	vector *= grid()->coordinateSnap();

	for (LDObject* obj : selectedObjects())
		obj->move (vector);
}

void MoveToolset::moveXNeg()
{
	moveObjects({-1, 0, 0});
}

void MoveToolset::moveYNeg()
{
	moveObjects({0, -1, 0});
}

void MoveToolset::moveZNeg()
{
	moveObjects({0, 0, -1});
}

void MoveToolset::moveXPos()
{
	moveObjects({1, 0, 0});
}

void MoveToolset::moveYPos()
{
	moveObjects({0, 1, 0});
}

void MoveToolset::moveZPos()
{
	moveObjects({0, 0, 1});
}

double MoveToolset::getRotateActionAngle()
{
	return (pi * grid()->angleSnap()) / 180;
}

void MoveToolset::rotateXPos()
{
	math()->rotateObjects (1, 0, 0, getRotateActionAngle(), selectedObjects().toList().toVector());
}

void MoveToolset::rotateYPos()
{
	math()->rotateObjects (0, 1, 0, getRotateActionAngle(), selectedObjects().toList().toVector());
}

void MoveToolset::rotateZPos()
{
	math()->rotateObjects (0, 0, 1, getRotateActionAngle(), selectedObjects().toList().toVector());
}

void MoveToolset::rotateXNeg()
{
	math()->rotateObjects (-1, 0, 0, getRotateActionAngle(), selectedObjects().toList().toVector());
}

void MoveToolset::rotateYNeg()
{
	math()->rotateObjects (0, -1, 0, getRotateActionAngle(), selectedObjects().toList().toVector());
}

void MoveToolset::rotateZNeg()
{
	math()->rotateObjects (0, 0, -1, getRotateActionAngle(), selectedObjects().toList().toVector());
}

void MoveToolset::configureRotationPoint()
{
	QDialog* dialog = new QDialog;
	Ui_RotPointUI ui;
	ui.setupUi(dialog);

	switch (RotationPoint(config::rotationPointType()))
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

	Vertex custompoint = config::customRotationPoint();
	ui.customX->setValue(custompoint.x);
	ui.customY->setValue(custompoint.y);
	ui.customZ->setValue(custompoint.z);

	if (dialog->exec() == QDialog::Accepted)
	{
		RotationPoint pointType;

		if (ui.objectPoint->isChecked())
			pointType = ObjectOrigin;
		else if (ui.objectPoint->isChecked())
			pointType = WorldOrigin;
		else
			pointType = CustomPoint;

		custompoint.x = ui.customX->value();
		custompoint.y = ui.customY->value();
		custompoint.z = ui.customZ->value();
		config::setRotationPointType(pointType);
		config::setCustomRotationPoint(custompoint);
	}
}
