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
#include "abstractEditMode.h"

class LinePathMode : public AbstractDrawMode
{
	DEFINE_CLASS (LinePathMode, AbstractDrawMode)

public:
	LinePathMode (GLRenderer* renderer);

	void render (QPainter& painter) const override;
	EditModeType type() const override { return EditModeType::LinePath; }
	bool mouseReleased (MouseEventData const& data) override;
	bool preAddVertex (Vertex const& pos) override;
	bool keyReleased (QKeyEvent*) override;
	void endDraw() override;
};

