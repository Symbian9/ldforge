#pragma once
#include <QDialog>
#include "../primitives.h"

class Ui_GeneratePrimitiveDialog;

class GeneratePrimitiveDialog : public QDialog
{
	Q_OBJECT

public:
	GeneratePrimitiveDialog(QWidget* parent = nullptr, Qt::WindowFlags f = 0);
	virtual ~GeneratePrimitiveDialog();
	PrimitiveSpec spec() const;

public slots:
	void highResolutionToggled (bool on);

private:
	Ui_GeneratePrimitiveDialog& ui;
};
