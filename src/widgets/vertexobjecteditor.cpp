#include <QDoubleSpinBox>
#include "../linetypes/modelobject.h"
#include "vertexobjecteditor.h"
#include "ui_vertexobjecteditor.h"
#include "../dialogs/colorselector.h"

VertexObjectEditor::VertexObjectEditor(LDObject* object, QWidget *parent) :
	QDialog {parent},
	object {object},
	ui {*new Ui_VertexObjectEditor},
	vertexGrid {new QGridLayout}
{
	this->ui.setupUi(this);
	this->ui.verticesContainer->setLayout(this->vertexGrid);
	connect(
		this->ui.color,
		&QPushButton::clicked,
		[&]()
		{
			ColorSelector::selectColor(this, this->currentColor, this->currentColor);
			this->setColorButton(this->currentColor);
		}
	);

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

	this->currentColor = this->object->color();
	this->setColorButton(this->object->color());
}

VertexObjectEditor::~VertexObjectEditor()
{
	delete &this->ui;
}

void VertexObjectEditor::setColorButton(LDColor color)
{
	if (color.isValid())
	{
		this->ui.color->setText(color.name());
		this->ui.color->setStyleSheet(format("background-color: %1", color.hexcode()));
		this->ui.color->setStyleSheet(format("color: %1", color.edgeColor().name()));
	}
	else
	{
		this->ui.color->setText("");
		this->ui.color->setStyleSheet("");
	}
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

	this->object->setColor(this->currentColor);
	QDialog::accept();
}
