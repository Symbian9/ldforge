/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2018 Teemu Piippo
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "circularprimitiveeditor.h"
#include "ui_circularprimitiveeditor.h"
#include "../primitives.h"

// offsetof doesn't work if this doesn't hold true
static_assert(std::is_standard_layout<Ui_CircularPrimitiveEditor>::value, "UI type is not of standard layout");

// Contains offsets to radio buttons and the choice they represent.
static const struct
{
	std::ptrdiff_t offset;
	PrimitiveModel::Type primitiveType;

	QRadioButton* resolve(Ui_CircularPrimitiveEditor& ui) const
	{
		// Find the radio button by pointer arithmetic
		return *reinterpret_cast<QRadioButton**>(reinterpret_cast<char*>(&ui) + offset);
	}
} radioButtonMap[] = {
#define MAP_RADIO_BUTTON(widget, type) { offsetof(Ui_CircularPrimitiveEditor, widget), PrimitiveModel::type }
	MAP_RADIO_BUTTON(circle, Circle),
	MAP_RADIO_BUTTON(cylinder, Cylinder),
	MAP_RADIO_BUTTON(disc, Disc),
	MAP_RADIO_BUTTON(discNegative, DiscNegative),
	MAP_RADIO_BUTTON(cylinderClosed, CylinderClosed),
	MAP_RADIO_BUTTON(cylinderOpen, CylinderOpen),
	MAP_RADIO_BUTTON(chord, Chord),
#undef MAP_RADIO_BUTTON
};

/*
 * Constructs a new circular primitive editor and sets up connections.
 */
CircularPrimitiveEditor::CircularPrimitiveEditor(LDCircularPrimitive* primitive, QWidget* parent) :
	QDialog {parent},
	ui {*new Ui_CircularPrimitiveEditor},
	primitive {primitive}
{
	ui.setupUi(this);

	// Set the initial values of the dialog
	updateWidgets();

	if (primitive)
	{
		// Store the original state of the object. If the user presses "Reset" then the object is restored
		// from this archive.
		Serializer serializer {originalState, Serializer::Store};
		primitive->serialize(serializer);
	}

	for (const auto& mapping : ::radioButtonMap)
	{
		QRadioButton* button = mapping.resolve(ui);

		// If the radio button gets checked, update the type of the circular primitive.
		connect(
			button,
			&QRadioButton::toggled,
			[&](bool checked)
			{
				if (checked and this->primitive)
					this->primitive->setPrimitiveType(mapping.primitiveType);
			}
		);
	}

	// Connect various widgets so that changing them changes the primitive object.
	connect(
		ui.segments,
		qOverload<int>(&QSpinBox::valueChanged),
		[&](int newSegments)
		{
			if (this->primitive)
				this->primitive->setSegments(newSegments);
		}
	);
	connect(
		ui.divisions,
		&QComboBox::currentTextChanged,
		[&](const QString& newDivisions)
		{
			if (this->primitive)
				this->primitive->setDivisions(newDivisions.toInt());
		}
	);
	connect(
		ui.color,
		&ColorButton::colorChanged,
		[&](LDColor newColor)
		{
			if (this->primitive)
				this->primitive->setColor(newColor);
		}
	);
	connect(
		ui.matrix,
		&MatrixEditor::matrixChanged,
		[&](const QMatrix4x4& newMatrix)
		{
			if (this->primitive)
				this->primitive->setTransformationMatrix(newMatrix);
		}
	);
	// Connect the reset button, "reset button" here meaning any button with the reset role.
	connect(
		ui.buttonBox,
		&QDialogButtonBox::clicked,
		[&](QAbstractButton* button)
		{
			if (ui.buttonBox->buttonRole(button) == QDialogButtonBox::ResetRole)
				reset();
		}
	);

	if (this->primitive)
	{
		// If the primitive is changed by some other thing (e.g. by resetting it), update the widgets.
		connect(this->primitive, &LDObject::modified, this, &CircularPrimitiveEditor::updateWidgets);

		// If the object is deleted, then destroy the dialog.
		connect(this->primitive, &LDObject::destroyed, this, &QDialog::reject);
	}
}

/*
 * Frees the user interface memory after the dialog is destroyed.
 */
CircularPrimitiveEditor::~CircularPrimitiveEditor()
{
	delete &ui;
}

/*
 * Updates the widgets of the editor to reflect the properites of the object being modified.
 */
void CircularPrimitiveEditor::updateWidgets()
{
	setEnabled(primitive != nullptr);

	if (primitive)
	{
		for (const auto& mapping : ::radioButtonMap)
		{
			// Choose the correct radio button
			QRadioButton* button = mapping.resolve(ui);
			withSignalsBlocked(button, [&]()
			{
				button->setChecked(primitive->primitiveType() == mapping.primitiveType);
			});
		}

		// Set the values of the form.
		withSignalsBlocked(ui.segments, [&](){ ui.segments->setValue(primitive->segments()); });
		withSignalsBlocked(ui.divisions, [&]()
		{
			ui.divisions->setCurrentText(QString::number(primitive->divisions()));
		});
		withSignalsBlocked(ui.color, [&](){ ui.color->setColor(primitive->color()); });
		withSignalsBlocked(ui.matrix, [&](){ ui.matrix->setMatrix(primitive->transformationMatrix()); });
	}
}

/*
 * Resets the object being modified. The object will emit a signal that is connected to updateWidgets.
 */
void CircularPrimitiveEditor::reset()
{
	if (primitive)
		primitive->restore(originalState); // Restoring does not change 'originalState'
}
