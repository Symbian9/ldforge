#include "cylindereditor.h"
#include "ui_cylindereditor.h"

CylinderEditor::CylinderEditor(QWidget* parent) :
	QDialog {parent},
	ui {*new Ui_CylinderEditor}
{
	ui.setupUi(this);
}

CylinderEditor::~CylinderEditor()
{
	delete &ui;
}
