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
#include "abstractEditMode.h"

class SelectMode : public AbstractSelectMode
{
	QPoint m_rangeStart;
	bool m_rangepick;
	bool m_addpick;

	DEFINE_CLASS (SelectMode, AbstractSelectMode)

public:
	SelectMode (Canvas* canvas);

	void doSelection(const QRect& area);
	void render (QPainter& painter) const override;
	bool mousePressed (QMouseEvent* ev) override;
	bool mouseReleased (MouseEventData const& data) override;
	bool mouseDoubleClicked (QMouseEvent* ev) override;
	bool mouseMoved (QMouseEvent*) override;
	EditModeType type() const override;
};