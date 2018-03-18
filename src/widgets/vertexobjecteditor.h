#pragma once
#include <QDialog>
#include "../main.h"

class VertexObjectEditor : public QDialog
{
	Q_OBJECT

public:
	VertexObjectEditor(LDObject* object = nullptr, QWidget* parent = nullptr);
	~VertexObjectEditor();

	void accept() override;

private:
	class QDoubleSpinBox* spinboxAt(int i, Axis axis);

	class Ui_VertexObjectEditor& ui;
	class QGridLayout* vertexGrid;
	LDObject* const object;
	LDColor currentColor;
};
