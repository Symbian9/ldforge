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

#pragma once
#include <QDialog>
#include "../main.h"
#include "../colors.h"

/*
 * Implements a dialog that asks the user to choose an LDraw color from a grid of available colors. Direct colors are also supported.
 */
class ColorSelector : public QDialog
{
	Q_OBJECT

public:
	ColorSelector(QWidget* parent, LDColor defaultvalue = LDColor::nullColor);
	~ColorSelector();

	static bool selectColor(QWidget* parent, LDColor& color, LDColor defaultColor = LDColor::nullColor);
	LDColor selectedColor() const;
	Q_SLOT void setSelectedColor(LDColor color);

private:
	static const int columnCount = 16;

	Q_SLOT void colorButtonClicked();
	Q_SLOT void chooseDirectColor();
	Q_SLOT void transparentCheckboxClicked();

	class Ui_ColorSelUi& ui;
	QMap<LDColor, QPushButton*> m_buttons;
	QMap<QPushButton*, LDColor> m_buttonsReversed;
	LDColor m_selectedColor;
};
