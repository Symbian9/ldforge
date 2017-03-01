/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2017 Teemu Piippo
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

#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QMouseEvent>
#include <QScrollBar>
#include <QColorDialog>
#include "../colors.h"
#include "../guiutilities.h"
#include "../main.h"
#include "../mainwindow.h"
#include "../miscallenous.h"
#include "colorselector.h"
#include "ui_colorselector.h"

/*
 * Constructs a color selection dialog.
 */
ColorSelector::ColorSelector(QWidget* parent, LDColor defaultColor) :
    QDialog {parent},
    HierarchyElement {parent},
    ui {*new Ui_ColorSelUi},
    m_selectedColor {LDColor::nullColor}
{
	ui.setupUi(this);

	QGridLayout* gridLayout = new QGridLayout;

	// Spawn color selector buttons
	for (LDColor color; color.isLDConfigColor(); ++color)
	{
		QPushButton* button = new QPushButton {this};
		button->setMinimumSize({32, 32});
		button->setMaximumSize(button->minimumSize());

		if (color.isValid())
		{
			QColor faceColor = color.faceColor();

			if (color == MainColor)
			{
				faceColor = m_config->mainColor();
				faceColor.setAlphaF(m_config->mainColorAlpha());
			}

			QString edgeColor = luma(faceColor) < 80 ? "white" : "black";
			button->setAutoFillBackground(true);
			button->setStyleSheet(format("background-color: rgba(%1, %2, %3, %4); color: %5",
			                             faceColor.red(), faceColor.green(), faceColor.blue(), faceColor.alpha(), edgeColor));
			button->setCheckable(true);
			button->setText(QString::number(color.index()));
			button->setToolTip(format("%1: %2", color.index(), color.name()));
			m_buttons[color] = button;
			m_buttonsReversed[button] = color;
			connect(button, SIGNAL(clicked(bool)), this, SLOT(colorButtonClicked()));
		}
		else
		{
			button->setEnabled(false);
		}

		gridLayout->addWidget(button, color.index() / columnCount, color.index() % columnCount);
	}

	QWidget* gridContainerWidget = new QWidget;
	gridContainerWidget->setLayout(gridLayout);
	ui.definedColors->setWidget(gridContainerWidget);
	connect(ui.directColor, SIGNAL(clicked(bool)), this, SLOT(chooseDirectColor()));
	ui.definedColors->setMinimumWidth(ui.definedColors->widget()->width() + 16);

#ifdef TRANSPARENT_DIRECT_COLORS
	connect(ui.transparentDirectColor, SIGNAL(clicked(bool)), this, SLOT(transparentCheckboxClicked()));
#else
	ui.transparentDirectColor->hide();
#endif

	setSelectedColor(defaultColor);
}

/*
 * Destructs the color selection dialog.
 */
ColorSelector::~ColorSelector()
{
	delete &ui;
}

/*
 * Handles the press of a color button.
 */
void ColorSelector::colorButtonClicked()
{
	QPushButton* button = qobject_cast<QPushButton*>(sender());
	LDColor color = m_buttonsReversed.value(button, LDColor::nullColor);

	if (color.isValid())
		setSelectedColor(color);
}

/*
 * Asks the user for a direct color.
 */
void ColorSelector::chooseDirectColor()
{
	QColor defaultColor = selectedColor() != -1 ? selectedColor().faceColor() : Qt::white;
	QColor newColor = QColorDialog::getColor(defaultColor);

	if (newColor.isValid())
		setSelectedColor({newColor, ui.transparentDirectColor->isChecked()});
}

/*
 * Handles the click of the transparent direct color option (only available of transparent direct colors are enabled).
 */
void ColorSelector::transparentCheckboxClicked()
{
	if (selectedColor().isDirect())
		setSelectedColor({selectedColor().faceColor(), ui.transparentDirectColor->isChecked()});
}

/*
 * Convenience function for invoking the color selection dialog.
 */
bool ColorSelector::selectColor(QWidget* parent, LDColor& color, LDColor defaultColor)
{
	ColorSelector dialog {parent, defaultColor};

	if (dialog.exec() and dialog.selectedColor().isValid())
	{
		color = dialog.selectedColor();
		return true;
	}

	return false;
}

/*
 * Returns the currently selected color.
 */
LDColor ColorSelector::selectedColor() const
{
	return m_selectedColor;
}

/*
 * Changes the selected color to the one provided, and updates all relevant widgets.
 */
void ColorSelector::setSelectedColor(LDColor newColor)
{
	// Uncheck the button we previously had pressed.
	QPushButton* button = m_buttons.value(m_selectedColor);

	if (button)
		button->setChecked(false);

	// Select the new color.
	m_selectedColor = newColor;
	button = m_buttons.value(newColor);

	if (button)
		button->setChecked(true);

	if (m_selectedColor.isValid())
	{
		ui.colorLabel->setText(format("%1 - %2",
		                              newColor.indexString(),
		                              newColor.isDirect() ? newColor.faceColor().name() : newColor.name()));
		ui.iconLabel->setPixmap(guiUtilities()->makeColorIcon(newColor, 16).pixmap(16, 16));
		ui.transparentDirectColor->setEnabled(newColor.isDirect());
		ui.transparentDirectColor->setChecked(newColor.isDirect() and newColor.faceColor().alphaF() < 1.0);
	}
	else
	{
		ui.colorLabel->setText("---");
		ui.iconLabel->setPixmap({});
		ui.transparentDirectColor->setChecked(false);
	}
}
