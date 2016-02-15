#include "../ldObject.h"
#include "generateprimitivedialog.h"
#include "ui_generateprimitivedialog.h"

GeneratePrimitiveDialog::GeneratePrimitiveDialog (QWidget* parent, Qt::WindowFlags f) :
	QDialog (parent, f),
	ui(*new Ui_GeneratePrimitiveDialog)
{
	ui.setupUi (this);
	connect (ui.highResolution, &QCheckBox::toggled, this, &GeneratePrimitiveDialog::highResolutionToggled);
}


GeneratePrimitiveDialog::~GeneratePrimitiveDialog()
{
	delete &ui;
}


void GeneratePrimitiveDialog::highResolutionToggled (bool on)
{
	ui.segments->setMaximum (on ? HighResolution : LowResolution);

	// If the current value is 16 and we switch to hi-res, default the
	// spinbox to 48. (should we scale this?)
	if (on and ui.segments->value() == LowResolution)
		ui.segments->setValue(HighResolution);
}


PrimitiveSpec GeneratePrimitiveDialog::spec() const
{
	PrimitiveSpec result;
	result.type =
		ui.typeCircle->isChecked()       ? Circle :
		ui.typeCylinder->isChecked()     ? Cylinder :
		ui.typeDisc->isChecked()         ? Disc :
		ui.typeDiscNegative->isChecked() ? DiscNegative :
		ui.typeRing->isChecked()         ? Ring :
										   Cone;
	result.divisions = ui.highResolution->isChecked() ? HighResolution : LowResolution;
	result.segments = ui.segments->value();
	result.ringNumber = ui.ringNumber->value();
	return result;
}