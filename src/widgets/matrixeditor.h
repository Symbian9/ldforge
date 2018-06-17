#pragma once
#include <QWidget>
#include "../linetypes/modelobject.h"

class QDoubleSpinBox;
class Ui_MatrixEditor;

class MatrixEditor : public QWidget
{
	Q_OBJECT

public:
	MatrixEditor(const QMatrix4x4& matrix = {}, QWidget* parent = nullptr);
	MatrixEditor(QWidget* parent);
	~MatrixEditor();

	QMatrix4x4 matrix() const;
	void setMatrix(const QMatrix4x4& matrix);

signals:
	void matrixChanged(const QMatrix4x4& matrix);

private slots:
	void scalingChanged();
	void matrix3x3Changed();

private:
	QDoubleSpinBox* matrixCell(int row, int column) const;
	double matrixScaling(int column) const;
	QPair<int, int> cellPosition(QDoubleSpinBox* cellWidget);
	QDoubleSpinBox* vectorElement(int index);

	Ui_MatrixEditor& ui;
};
