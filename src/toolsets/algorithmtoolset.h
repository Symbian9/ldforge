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
#include "toolset.h"

class AlgorithmToolset : public Toolset
{
	Q_OBJECT
public:
	explicit AlgorithmToolset (MainWindow* parent);

	Q_INVOKABLE void addHistoryLine();
	Q_INVOKABLE void autocolor();
	Q_INVOKABLE void demote();
	Q_INVOKABLE void editRaw();
	Q_INVOKABLE void flip();
	Q_INVOKABLE void makeBorders();
	Q_INVOKABLE void replaceCoordinates();
	Q_INVOKABLE void roundCoordinates();
	Q_INVOKABLE void splitLines();
	Q_INVOKABLE void splitQuads();
	Q_INVOKABLE void subfileSelection();

private:
	bool isColorUsed (class LDColor color);
	LDObject* next(LDObject* object);
};
