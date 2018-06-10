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

#include <QDoubleSpinBox>
#include "../linetypes/modelobject.h"
#include "vertexobjecteditor.h"
#include "ui_vertexobjecteditor.h"
#include "../dialogs/colorselector.h"
#include "../guiutilities.h"

VertexObjectEditor::VertexObjectEditor(LDObject* object, QWidget *parent) :
	QDialog {parent},
	ui {*new Ui_VertexObjectEditor},
	vertexGrid {new QGridLayout},
	object {object}
{
	this->ui.setupUi(this);
	this->ui.verticesContainer->setLayout(this->vertexGrid);
	this->ui.colorButton->setColor(object->color());

	for (int i : range(0, 1, object->numVertices() - 1))
	{
		for (Axis axis : {X, Y, Z})
		{
			QDoubleSpinBox* spinbox = new QDoubleSpinBox;
			spinbox->setMinimum(-1e6);
			spinbox->setMaximum(1e6);
			spinbox->setDecimals(5);
			this->vertexGrid->addWidget(spinbox, i, axis);
		}
	}

	for (int i : range(0, 1, object->numVertices() - 1))
	{
		for (Axis axis : {X, Y, Z})
		{
			QDoubleSpinBox* spinbox = this->spinboxAt(i, axis);

			if (spinbox)
				spinbox->setValue(this->object->vertex(i)[axis]);
		}
	}
}

VertexObjectEditor::~VertexObjectEditor()
{
	delete &this->ui;
}

QDoubleSpinBox* VertexObjectEditor::spinboxAt(int i, Axis axis)
{
	QWidget* widget = this->vertexGrid->itemAtPosition(i, axis)->widget();
	return qobject_cast<QDoubleSpinBox*>(widget);
}

void VertexObjectEditor::accept()
{
	for (int i : range(0, 1, object->numVertices() - 1))
	{
		Vertex vertex;

		for (Axis axis : {X, Y, Z})
		{
			QDoubleSpinBox* spinbox = this->spinboxAt(i, axis);

			if (spinbox)
				vertex.setCoordinate(axis, spinbox->value());
		}

		this->object->setVertex(i, vertex);
	}

	this->object->setColor(ui.colorButton->color());
	QDialog::accept();
}
