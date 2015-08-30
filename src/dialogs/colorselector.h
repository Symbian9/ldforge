/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2015 Teemu Piippo
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

class ColorSelector : public QDialog
{
	Q_OBJECT
	PROPERTY (private, LDColor, selection, setSelection, STOCK_WRITE)

public:
	explicit ColorSelector (LDColor defaultvalue = LDColor::nullColor(), QWidget* parent = null);
	virtual ~ColorSelector();
	static bool selectColor (LDColor& val, LDColor defval = LDColor::nullColor(), QWidget* parent = null);

private:
	class Ui_ColorSelUi& ui;
	QMap<int, QPushButton*> m_buttons;
	QMap<QPushButton*, int> m_buttonsReversed;
	bool m_firstResize;

	void drawColorInfo();
	void selectDirectColor (QColor col);

private slots:
	void colorButtonClicked();
	void chooseDirectColor();
	void transparentCheckboxClicked();
};
