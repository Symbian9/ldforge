#pragma once
#include <QWidget>
#include "../linetypes/modelobject.h"

class QDoubleSpinBox;
class Ui_MatrixEditor;

class MatrixEditor : public QWidget
{
	Q_OBJECT

public:
	MatrixEditor(
		const Matrix& matrix = Matrix::identity,
		const Vertex& position = {0, 0, 0},
		QWidget* parent = nullptr
	);
	MatrixEditor(QWidget* parent);
	~MatrixEditor();

	Vertex position() const;
	Matrix matrix() const;
	void setPosition(const Vertex& position);
	void setMatrix(const Matrix& matrix);

private slots:
	void scalingChanged();
	void matrixChanged();

private:
	QDoubleSpinBox* matrixCell(int row, int column) const;
	double matrixScaling(int column) const;
	QPair<int, int> cellPosition(QDoubleSpinBox* cellWidget);
	QDoubleSpinBox* vectorElement(int index);

	Ui_MatrixEditor& ui;
};
