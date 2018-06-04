#ifndef CYLINDEREDITOR_H
#define CYLINDEREDITOR_H

#include <QDialog>

namespace Ui {
class CylinderEditor;
}

class CylinderEditor : public QDialog
{
	Q_OBJECT

public:
	explicit CylinderEditor(QWidget *parent = 0);
	~CylinderEditor();

private:
	Ui::CylinderEditor *ui;
};

#endif // CYLINDEREDITOR_H
