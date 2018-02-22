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

#include <QMouseEvent>
#include "magicWandMode.h"
#include "../lddocument.h"
#include "../mainwindow.h"
#include "../canvas.h"

MagicWandMode::MagicWandMode(Canvas* canvas) :
	Super {canvas}
{
	QSet<LineSegment> boundarySegments;

	// Get vertex<->object data
	for (const QModelIndex& index : currentDocument()->indices())
	{
		// Note: this deliberately only takes vertex-objects into account.
		// The magic wand does not process subparts.
		LDObject* object = currentDocument()->lookup(index);

		for (int i = 0; i < object->numVertices(); ++i)
		{
			LineSegment segment {
				object->vertex(i),
				object->vertex((i + 1) % object->numVertices())
			};
			m_vertices[object->vertex(i)].insert(index);

			if (object->type() == LDObjectType::EdgeLine)
				boundarySegments.insert(segment);
			else
				this->segments[segment].insert(index);
		}
	}

	// Remove all edge lines from the set of available segments because they get in the way.
	for (const LineSegment& boundarySegment : boundarySegments)
		this->segments.remove(boundarySegment);
}

EditModeType MagicWandMode::type() const
{
	return EditModeType::MagicWand;
}

void MagicWandMode::edgeFill(
	QModelIndex index,
	QItemSelection& selection,
	QSet<QModelIndex>& processed
) const {
	QItemSelection result;
	processed.insert(index);
	result.select(index, index);
	QSet<QPersistentModelIndex> candidates;
	LDObject* object = currentDocument()->lookup(index);

	// Get the list of objects that touch this object, i.e. share a vertex with it.
	for (int i = 0; i < object->numVertices(); ++i)
		candidates |= m_vertices[object->vertex(i)];

	candidates.remove(index);

	for (const QModelIndex& candidate : candidates)
	{
		LDObject* candidateObject = currentDocument()->lookup(candidate);

		if (candidateObject->type() == LDObjectType::EdgeLine
			and candidateObject->color() == object->color()
		) {
			selection.select(candidate, candidate);

			if (not processed.contains(candidate))
				edgeFill(candidate, selection, processed);
		}
	}
}

void MagicWandMode::surfaceFill(
	QModelIndex index,
	QItemSelection& selection,
	QSet<QModelIndex>& processed
) const {
	LDObject* object = currentDocument()->lookup(index);
	selection.select(index, index);
	processed.insert(index);

	for (int i = 0; i < object->numVertices(); i += 1)
	{
		Vertex v_1 = object->vertex(i);
		Vertex v_2 = object->vertex((i + 1) % object->numVertices());
		LineSegment segment {v_1, v_2};

		for (const QModelIndex& candidate : this->segments[segment])
		{
			if (currentDocument()->lookup(candidate)->color() == object->color())
			{
				selection.select(candidate, candidate);

				if (not processed.contains(candidate))
					surfaceFill(candidate, selection, processed);
			}
		}
	}
}

QItemSelection MagicWandMode::doMagic(const QModelIndex& index) const
{
	QItemSelection selection;
	LDObject* object = currentDocument()->lookup(index);

	if (object)
	{
		QSet<QModelIndex> processed;

		if (object->type() == LDObjectType::EdgeLine)
			edgeFill(index, selection, processed);
		else if (object->numPolygonVertices() >= 3)
			surfaceFill(index, selection, processed);
	}

	return selection;
}

bool MagicWandMode::mouseReleased (MouseEventData const& data)
{
	if (Super::mouseReleased (data))
		return true;

	if (data.releasedButtons & Qt::LeftButton and not data.mouseMoved)
	{
		QItemSelection selection = this->doMagic(renderer()->pick(data.ev->x(), data.ev->y()));
		QItemSelectionModel::SelectionFlags command = QItemSelectionModel::ClearAndSelect;

		if (data.keymods & Qt::ShiftModifier)
			command = QItemSelectionModel::Select;
		else if (data.keymods & Qt::ControlModifier)
			command = QItemSelectionModel::Deselect;

		renderer()->selectionModel()->select(selection, command);
		return true;
	}

	return false;
}
