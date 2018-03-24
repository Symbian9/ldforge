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

#pragma once
#include "toolset.h"

class BasicToolset : public Toolset
{
	Q_OBJECT

public:
	BasicToolset (MainWindow *parent);

	Q_INVOKABLE void copy();
	Q_INVOKABLE void cut();
	Q_INVOKABLE void edit();
	Q_INVOKABLE void inlineDeep();
	Q_INVOKABLE void inlineShallow();
	Q_INVOKABLE void insertRaw();
	Q_INVOKABLE void invert();
	Q_INVOKABLE void modeCircle();
	Q_INVOKABLE void modeDraw();
	Q_INVOKABLE void modeLinePath();
	Q_INVOKABLE void modeMagicWand();
	Q_INVOKABLE void modeRectangle();
	Q_INVOKABLE void modeSelect();
	Q_INVOKABLE void modeCurve();
	Q_INVOKABLE void newComment();
	Q_INVOKABLE void newConditionalLine();
	Q_INVOKABLE void newLine();
	Q_INVOKABLE void newQuadrilateral();
	Q_INVOKABLE void newSubfile();
	Q_INVOKABLE void newTriangle();
	Q_INVOKABLE void paste();
	Q_INVOKABLE void redo();
	Q_INVOKABLE void remove();
	Q_INVOKABLE void setColor();
	Q_INVOKABLE void uncolor();
	Q_INVOKABLE void undo();

private:
	int copyToClipboard();
	void doInline (bool deep);
};
