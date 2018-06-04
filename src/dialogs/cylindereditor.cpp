#include "cylindereditor.h"
#include "ui_cylindereditor.h"

CylinderEditor::CylinderEditor(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::CylinderEditor)
{
	ui->setupUi(this);
}

CylinderEditor::~CylinderEditor()
{
	delete ui;
}
