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

#include "colorbutton.h"
#include "../dialogs/colorselector.h"

/*
 * Initializes the color button and sets the default color
 */
ColorButton::ColorButton(LDColor color, QWidget* parent) :
	ColorButton (parent)
{
	setColor(color);
}

/*
 * Initializes the color button
 */
ColorButton::ColorButton(QWidget* parent) :
	QPushButton (parent)
{
	connect(
		this,
		&QPushButton::clicked,
		[&]()
		{
			// The dialog must not have the button as the parent or the dialog inherits the stylesheet.
			ColorSelector dialog {qobject_cast<QWidget*>(this->parent()), _color};

			if (dialog.exec() and dialog.selectedColor().isValid())
				setColor(dialog.selectedColor());
		}
	);

	connect(
		this,
		&ColorButton::colorChanged,
		[&](LDColor)
		{
			if (_color.isValid())
			{
				setFlat(true);
				setText(_color.name());
				setStyleSheet(format(
					"background-color: %1; color: %2; border:none;",
					_color.hexcode(),
					_color.edgeColor().name()
				));
			}
			else
			{
				setFlat(false);
				setText("");
				setStyleSheet("");
			}
		}
	);
}

/*
 * Returns the currently selected color.
 */
LDColor ColorButton::color() const
{
	return _color;
}

/*
 * Sets the currently selected color.
 */
void ColorButton::setColor(LDColor color)
{
	_color = color;
	emit colorChanged(color);
}
