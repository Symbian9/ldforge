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
#include "abstractEditMode.h"
#include "../basics.h"
#include "../geometry/linesegment.h"
#include <QMap>
#include <QVector>

class MagicWandMode : public AbstractSelectMode
{
	QMap<Vertex, QSet<QPersistentModelIndex>> m_vertices;
	QMap<LineSegment, QSet<QPersistentModelIndex>> segments;
	QItemSelection m_selection;

	DEFINE_CLASS (MagicWandMode, AbstractSelectMode)

public:
	MagicWandMode(Canvas* canvas);

	QItemSelection doMagic(const QModelIndex& index) const;
	virtual EditModeType type() const override;
	virtual bool mouseReleased(MouseEventData const& data) override;

private:
	void edgeFill(
		QModelIndex index,
		QItemSelection& selection,
		QSet<QModelIndex>& processed) const;
	void surfaceFill(
		QModelIndex index,
		QItemSelection& selection,
		QSet<QModelIndex>& processed) const;
};
