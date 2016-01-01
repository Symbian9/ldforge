/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2016 Teemu Piippo
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
#include "abstractEditMode.h"

class RectangleMode : public AbstractDrawMode
{
	DEFINE_CLASS (RectangleMode, AbstractDrawMode)
	QVector<Vertex> m_rectangleVerts;

public:
	RectangleMode (GLRenderer* renderer);

	void render (QPainter& painter) const override;
	EditModeType type() const override;
	void endDraw() override;
	bool mouseMoved (QMouseEvent*) override;
	int maxVertices() const override { return 2; }

private:
	void updateRectVerts();
};
