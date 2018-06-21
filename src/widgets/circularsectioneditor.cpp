#include "circularsectioneditor.h"
#include "ui_circularsectioneditor.h"
#include "../primitives.h"

CircularSectionEditor::CircularSectionEditor(QWidget* parent) :
    QWidget {parent},
    ui {*new Ui_CircularSectionEditor}
{
	ui.setupUi(this);
	ui.divisions->setValidator(new QIntValidator {1, INT_MAX});
	connect(ui.segments, qOverload<int>(&QSpinBox::valueChanged), this, &CircularSectionEditor::segmentsChanged);
	connect(ui.divisions, qOverload<const QString&>(&QComboBox::activated), this, &CircularSectionEditor::divisionsChanged);
	previousDivisions = ui.divisions->currentText().toInt();
	updateFractionLabel();
}

CircularSectionEditor::~CircularSectionEditor()
{
	delete &ui;
}

CircularSection CircularSectionEditor::section()
{
	return {ui.segments->value(), ui.divisions->currentText().toInt()};
}

void CircularSectionEditor::setSection(const CircularSection& newSection)
{
	ui.divisions->setCurrentText(QString::number(newSection.divisions));
	ui.segments->setValue(newSection.segments);
}

void CircularSectionEditor::updateFractionLabel()
{
	CircularSection section = this->section();
	int numerator = section.segments;
	int denominator = section.divisions;
	simplify(numerator, denominator);
	ui.fraction->setText(fractionRep(numerator, denominator));
}

void CircularSectionEditor::divisionsChanged()
{
	// Scale the segments value to fit.
	int divisions = ui.divisions->currentText().toInt();

	if (divisions <= 0)
	{
		ui.divisions->setCurrentText(QString::number(1));
	}
	else
	{
		int newSegments = static_cast<int>(round(
			ui.segments->value() * double(divisions) / this->previousDivisions
		));
		ui.segments->setMaximum(divisions);
		ui.segments->setValue(newSegments);
		previousDivisions = divisions;
	}

	emit sectionChanged(section());
}

void CircularSectionEditor::segmentsChanged()
{
	updateFractionLabel();
	emit sectionChanged(section());
}
