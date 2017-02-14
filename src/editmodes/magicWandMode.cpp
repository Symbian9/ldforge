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
#include "../ldDocument.h"
#include "../mainwindow.h"
#include "../canvas.h"

MagicWandMode::MagicWandMode (Canvas* canvas) :
    Super (canvas)
{
	// Get vertex<->object data
	for (LDObject* obj : currentDocument()->objects())
	{
		// Note: this deliberately only takes vertex-objects into account.
		// The magic wand does not process subparts.
		for (int i = 0; i < obj->numVertices(); ++i)
			m_vertices[obj->vertex (i)] << obj;
	}
}

EditModeType MagicWandMode::type() const
{
	return EditModeType::MagicWand;
}

void MagicWandMode::fillBoundaries (LDObject* obj, QVector<BoundaryType>& boundaries, QSet<LDObject*>& candidates)
{
	// All boundaries obviously share vertices with the object, therefore they're all in the list
	// of candidates.
	for (LDObject* candidate : candidates)
	{
		if (not isOneOf(candidate->type(), LDObjectType::EdgeLine, LDObjectType::ConditionalEdge)
			or candidate->vertex (0) == candidate->vertex (1))
		{
			continue;
		}

		int matches = 0;

		for (int i = 0; i < obj->numVertices(); ++i)
		{
			if (not isOneOf (obj->vertex (i), candidate->vertex (0), candidate->vertex (1)))
				continue;

			if (++matches == 2)
			{
				// Boundary found. If it's an edgeline, add it to the boundaries list, if a
				// conditional line, select it.
				if (candidate->type() == LDObjectType::ConditionalEdge)
					m_selection << candidate;
				else
					boundaries.append (std::make_tuple (candidate->vertex (0), candidate->vertex (1)));

				break;
			}
		}
	}
}

void MagicWandMode::doMagic (LDObject* obj, MagicWandMode::MagicType type)
{
	if (obj == nullptr)
	{
		if (type == Set)
		{
			currentDocument()->clearSelection();
			m_window->buildObjectList();
		}

		return;
	}

	int matchesneeded = 0;
	QVector<BoundaryType> boundaries;
	LDObjectType objtype = obj->type();

	if (type != InternalRecursion)
	{
		m_selection.clear();
		m_selection.append (obj);
	}

	switch (obj->type())
	{
		case LDObjectType::EdgeLine:
		case LDObjectType::ConditionalEdge:
			matchesneeded = 1;
			break;

		case LDObjectType::Triangle:
		case LDObjectType::Quadrilateral:
			matchesneeded = 2;
			break;

		default:
			return;
	}

	QSet<LDObject*> candidates;

	// Get the list of objects that touch this object, i.e. share a vertex
	// with this.
	for (int i = 0; i < obj->numVertices(); ++i)
		candidates += m_vertices[obj->vertex (i)];

	// If we're dealing with surfaces, get a list of boundaries.
	if (matchesneeded > 1)
		fillBoundaries (obj, boundaries, candidates);

	for (LDObject* candidate : candidates)
	{
		try
		{
			// If we're doing this on lines, we need exact type match. Surface types (quads and
			// triangles) can be mixed. Also don't consider self a candidate, and don't consider
			// objects we have already processed.
			if ((candidate == obj) or
				(candidate->color() != obj->color()) or
				(m_selection.contains (candidate)) or
				(matchesneeded == 1 and (candidate->type() != objtype)) or
				((candidate->numVertices() > 2) ^ (matchesneeded == 2)))
			{
				throw 0;
			}

			// Now ensure the two objects share enough vertices.
			QVector<Vertex> matches;

			for (int i = 0; i < obj->numVertices(); ++i)
			{
				for (int j = 0; j < candidate->numVertices(); ++j)
				{
					if (obj->vertex(i) == candidate->vertex(j))
					{
						matches << obj->vertex(i);
						break;
					}
				}
			}

			if (countof(matches) < matchesneeded)
				throw 0; // Not enough matches.

			// Check if a boundary gets in between the objects.
			for (auto boundary : boundaries)
			{
				if (isOneOf (matches[0], std::get<0> (boundary), std::get<1> (boundary)) and
					isOneOf (matches[1], std::get<0> (boundary), std::get<1> (boundary)))
				{
					throw 0;
				}
			}

			m_selection.append (candidate);
			doMagic (candidate, InternalRecursion);
		}
		catch (int&)
		{
			continue;
		}
	}

	switch (type)
	{
	case Set:
		currentDocument()->clearSelection();
	case Additive:
		for (LDObject* obj : m_selection)
			currentDocument()->addToSelection(obj);
		break;

	case Subtractive:
		for (LDObject* obj : m_selection)
			currentDocument()->removeFromSelection(obj);
		break;

	case InternalRecursion:
		break;
	}

	if (type != InternalRecursion)
		m_window->buildObjectList();
}

bool MagicWandMode::mouseReleased (MouseEventData const& data)
{
	if (Super::mouseReleased (data))
		return true;

	if (data.releasedButtons & Qt::LeftButton and not data.mouseMoved)
	{
		MagicType wandtype = MagicWandMode::Set;

		if (data.keymods & Qt::ShiftModifier)
			wandtype = MagicWandMode::Additive;
		else if (data.keymods & Qt::ControlModifier)
			wandtype = MagicWandMode::Subtractive;

		doMagic (renderer()->pick (data.ev->x(), data.ev->y()), wandtype);
		return true;
	}

	return false;
}
