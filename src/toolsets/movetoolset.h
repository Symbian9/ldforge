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
#include "toolset.h"

class MoveToolset : public Toolset
{
	Q_OBJECT

public:
	explicit MoveToolset (MainWindow* parent);

	Q_INVOKABLE void configureRotationPoint();
	Q_INVOKABLE void gridCoarse();
	Q_INVOKABLE void gridFine();
	Q_INVOKABLE void gridMedium();
	Q_INVOKABLE void moveDown();
	Q_INVOKABLE void moveUp();
	Q_INVOKABLE void moveXNeg();
	Q_INVOKABLE void moveXPos();
	Q_INVOKABLE void moveYNeg();
	Q_INVOKABLE void moveYPos();
	Q_INVOKABLE void moveZNeg();
	Q_INVOKABLE void moveZPos();
	Q_INVOKABLE void rotateXNeg();
	Q_INVOKABLE void rotateXPos();
	Q_INVOKABLE void rotateYNeg();
	Q_INVOKABLE void rotateYPos();
	Q_INVOKABLE void rotateZNeg();
	Q_INVOKABLE void rotateZPos();

private:
	void moveSelection (bool up);
	void moveObjects (Vertex vect);
};
