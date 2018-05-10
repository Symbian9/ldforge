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

#include "subfilereferenceeditor.h"
#include "ui_subfilereferenceeditor.h"
#include "../linetypes/modelobject.h"
#include "../primitives.h"
#include "../guiutilities.h"
#include "../dialogs/colorselector.h"

SubfileReferenceEditor::SubfileReferenceEditor(LDSubfileReference* reference, QWidget* parent) :
	QDialog {parent},
	reference {reference},
	ui {*new Ui::SubfileReferenceEditor}
{
	this->ui.setupUi(this);
	this->ui.referenceName->setText(reference->referenceName());
	this->color = reference->color();
	::setColorButton(this->ui.colorButton, this->color);
	this->ui.positionX->setValue(reference->position().x);
	this->ui.positionY->setValue(reference->position().y);
	this->ui.positionZ->setValue(reference->position().z);
	connect(
		this->ui.colorButton,
		&QPushButton::clicked,
		[&]()
		{
			if (ColorSelector::selectColor(this, this->color, this->color))
				::setColorButton(this->ui.colorButton, this->color);
		}
	);
	for (int i : {0, 1, 2})
	for (int j : {0, 1, 2})
	{
		QLayoutItem* item = this->ui.matrixLayout->itemAtPosition(i, j);
		QDoubleSpinBox* spinbox = item ? qobject_cast<QDoubleSpinBox*>(item->widget()) : nullptr;
		spinbox->blockSignals(true);
		spinbox->setValue(reference->transformationMatrix()(i, j));
		spinbox->blockSignals(false);
		connect(
			spinbox,
			qOverload<double>(&QDoubleSpinBox::valueChanged),
			this,
			&SubfileReferenceEditor::matrixChanged
		);
	}
	connect(
		this->ui.primitivesTreeView,
		&QTreeView::clicked,
		[&](const QModelIndex& index)
		{
			QAbstractItemModel* model = this->ui.primitivesTreeView->model();
			QVariant primitiveName = model->data(index, PrimitiveManager::PrimitiveNameRole);

			if (primitiveName.isValid())
				this->ui.referenceName->setText(primitiveName.toString());
		}
	);

	for (QDoubleSpinBox* spinbox : {this->ui.scalingX, this->ui.scalingY, this->ui.scalingZ})
	{
		connect(
			spinbox,
			qOverload<double>(&QDoubleSpinBox::valueChanged),
			this,
			&SubfileReferenceEditor::scalingChanged
		);
	}

	// Fill in the initial scaling values
	for (int column : {0, 1, 2})
	{
		QDoubleSpinBox* spinbox = this->vectorElement(column);
		spinbox->blockSignals(true);
		spinbox->setValue(this->matrixScaling(column));
		spinbox->blockSignals(false);
	}
}

SubfileReferenceEditor::~SubfileReferenceEditor()
{
	delete &this->ui;
}

/*
 * Returns a spinbox from the matrix grid at position (row, column).
 * Row and column must be within [0, 2].
 */
QDoubleSpinBox* SubfileReferenceEditor::matrixCell(int row, int column) const
{
	if (qBound(0, row, 2) != row or qBound(0, column, 2) != column)
	{
		throw std::out_of_range {"bad row and column values"};
	}
	else
	{
		QLayoutItem* item = this->ui.matrixLayout->itemAtPosition(row, column);
		return item ? qobject_cast<QDoubleSpinBox*>(item->widget()) : nullptr;
	}
}

/*
 * Returns a spinbox for the vector element at the given position
 * Index must be within [0, 2]
 */
QDoubleSpinBox* SubfileReferenceEditor::vectorElement(int index)
{
	switch (index)
	{
	case 0:
		return this->ui.scalingX;

	case 1:
		return this->ui.scalingY;

	case 2:
		return this->ui.scalingZ;

	default:
		throw std::out_of_range {"bad index"};
	}
}

void SubfileReferenceEditor::accept()
{
	this->reference->setReferenceName(this->ui.referenceName->text());
	Matrix transformationMatrix;
	for (int i : {0, 1, 2})
	for (int j : {0, 1, 2})
	{
		transformationMatrix(i, j) = this->matrixCell(i, j)->value();
	}
	this->reference->setTransformationMatrix(transformationMatrix);
	this->reference->setPosition({
		this->ui.positionX->value(),
		this->ui.positionY->value(),
		this->ui.positionZ->value()
	});
	this->reference->setColor(this->color);
	QDialog::accept();
}

void SubfileReferenceEditor::setPrimitivesTree(PrimitiveManager* primitives)
{
	this->ui.primitivesTreeView->setModel(primitives);
}

double SubfileReferenceEditor::matrixScaling(int column) const
{
	return sqrt(
		pow(this->matrixCell(0, column)->value(), 2) +
		pow(this->matrixCell(1, column)->value(), 2) +
		pow(this->matrixCell(2, column)->value(), 2)
	);
}

/*
 * Updates the appropriate matrix column when a scaling vector element is changed.
 */
void SubfileReferenceEditor::scalingChanged()
{
	for (int column : {0, 1, 2})
	{
		if (this->sender() == this->vectorElement(column))
		{
			double oldScaling = this->matrixScaling(column);
			double newScaling = static_cast<QDoubleSpinBox*>(this->sender())->value();

			if (not qFuzzyCompare(newScaling, 0.0))
			{
				for (int row : {0, 1, 2})
				{
					double cellValue = this->matrixCell(row, column)->value();
					cellValue *= newScaling / oldScaling;
					QDoubleSpinBox* cellWidget = this->matrixCell(row, column);
					cellWidget->blockSignals(true);
					cellWidget->setValue(cellValue);
					cellWidget->blockSignals(false);
				}
			}

			break;
		}
	}
}

/*
 * Finds the position for the given cell widget.
 */
QPair<int, int> SubfileReferenceEditor::cellPosition(QDoubleSpinBox* cellWidget)
{
	for (int row : {0, 1, 2})
	for (int column : {0, 1, 2})
	{
		if (this->matrixCell(row, column) == cellWidget)
			return {row, column};
	}

	throw std::out_of_range {"widget is not in the matrix"};
}

/*
 * Updates the appropriate scaling vector element when a matrix cell is changed.
 */
void SubfileReferenceEditor::matrixChanged()
{
	QDoubleSpinBox* cellWidget = static_cast<QDoubleSpinBox*>(this->sender());

	try
	{
		int column = this->cellPosition(cellWidget).second;
		QDoubleSpinBox* spinbox = this->vectorElement(column);
		spinbox->blockSignals(true);
		spinbox->setValue(this->matrixScaling(column));
		spinbox->blockSignals(false);
	}
	catch (const std::out_of_range&) {}
}
