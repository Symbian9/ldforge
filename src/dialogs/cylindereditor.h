#pragma once
#include <QDialog>

class CylinderEditor : public QDialog
{
	Q_OBJECT

public:
	explicit CylinderEditor(QWidget* parent = nullptr);
	~CylinderEditor();

private:
	class Ui_CylinderEditor& ui;
};
