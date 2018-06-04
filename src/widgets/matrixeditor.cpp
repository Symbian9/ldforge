#include "matrixeditor.h"
#include "ui_matrixeditor.h"

MatrixEditor::MatrixEditor(const Matrix& matrix, const Vertex& position, QWidget *parent) :
	QWidget {parent},
	ui {*new Ui_MatrixEditor}
{
	this->ui.setupUi(this);
	this->setPosition(position);
	this->setMatrix(matrix);

	for (int i : {0, 1, 2})
	for (int j : {0, 1, 2})
	{
		connect(
			matrixCell(i, j),
			qOverload<double>(&QDoubleSpinBox::valueChanged),
			this,
			&MatrixEditor::matrixChanged
		);
	}

	for (QDoubleSpinBox* spinbox : {this->ui.scalingX, this->ui.scalingY, this->ui.scalingZ})
	{
		connect(
			spinbox,
			qOverload<double>(&QDoubleSpinBox::valueChanged),
			this,
			&MatrixEditor::scalingChanged
		);
	}
}

MatrixEditor::MatrixEditor(QWidget* parent) :
	MatrixEditor {Matrix::identity, {0, 0, 0}, parent} {}

MatrixEditor::~MatrixEditor()
{
	delete &this->ui;
}

/*
 * Returns a spinbox from the matrix grid at position (row, column).
 * Row and column must be within [0, 2].
 */
QDoubleSpinBox* MatrixEditor::matrixCell(int row, int column) const
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
QDoubleSpinBox* MatrixEditor::vectorElement(int index)
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

double MatrixEditor::matrixScaling(int column) const
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
void MatrixEditor::scalingChanged()
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
					withSignalsBlocked(cellWidget, [&]()
					{
						cellWidget->setValue(cellValue);
					});
				}
			}

			break;
		}
	}
}

/*
 * Finds the position for the given cell widget.
 */
QPair<int, int> MatrixEditor::cellPosition(QDoubleSpinBox* cellWidget)
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
void MatrixEditor::matrixChanged()
{
	QDoubleSpinBox* cellWidget = static_cast<QDoubleSpinBox*>(this->sender());

	try
	{
		int column = this->cellPosition(cellWidget).second;
		QDoubleSpinBox* spinbox = this->vectorElement(column);
		withSignalsBlocked(spinbox, [&]()
		{
			spinbox->setValue(this->matrixScaling(column));
		});
	}
	catch (const std::out_of_range&) {}
}

Vertex MatrixEditor::position() const
{
	return {
		this->ui.positionX->value(),
		this->ui.positionY->value(),
		this->ui.positionZ->value()
	};
}

Matrix MatrixEditor::matrix() const
{
	Matrix transformationMatrix;

	for (int i : {0, 1, 2})
	for (int j : {0, 1, 2})
	{
		transformationMatrix(i, j) = this->matrixCell(i, j)->value();
	}

	return transformationMatrix;
}

void MatrixEditor::setPosition(const Vertex& position)
{
	this->ui.positionX->setValue(position.x);
	this->ui.positionY->setValue(position.y);
	this->ui.positionZ->setValue(position.z);
}

void MatrixEditor::setMatrix(const Matrix& matrix)
{
	for (int i : {0, 1, 2})
	for (int j : {0, 1, 2})
	{
		QDoubleSpinBox* spinbox = matrixCell(i, j);
		withSignalsBlocked(spinbox, [&](){ spinbox->setValue(matrix(i, j)); });
	}

	// Fill in the initial scaling values
	for (int column : {0, 1, 2})
	{
		QDoubleSpinBox* spinbox = this->vectorElement(column);
		withSignalsBlocked(spinbox, [&]()
		{
			spinbox->setValue(this->matrixScaling(column));
		});
	}
}
