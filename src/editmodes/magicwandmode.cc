/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Santeri Piippo
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
#include "magicwandmode.h"
#include "../ldDocument.h"
#include "../mainWindow.h"

MagicWandMode::MagicWandMode (GLRenderer* renderer) :
	Super (renderer)
{
	// Get vertex<->object data
	for (LDObjectPtr obj : getCurrentDocument()->objects())
	{
		// Note: this deliberately only takes vertex-objects into account.
		// The magic wand does not process subparts.
		for (int i = 0; i < obj->numVertices(); ++i)
			_vertices[obj->vertex (i)] << obj;
	}
}

EditModeType MagicWandMode::type() const
{
	return EditModeType::MagicWand;
}

bool MagicWandMode::allowFreeCamera() const
{
	return true;
}

void MagicWandMode::fillBoundaries (LDObjectPtr obj, QVector<BoundaryType>& boundaries, QVector<LDObjectPtr>& candidates)
{
	// All boundaries obviously share vertices with the object, therefore they're all in the list
	// of candidates.
	for (auto it = candidates.begin(); it != candidates.end(); ++it)
	{
		if ((*it)->type() != OBJ_Line || (*it)->vertex (0) == (*it)->vertex (1))
			continue;

		int matches = 0;

		for (int i = 0; i < obj->numVertices(); ++i)
		{
			if (not eq (obj->vertex (i), (*it)->vertex (0), (*it)->vertex (1)))
				continue;

			if (++matches == 2)
			{
				// Boundary found. Add to boundaries list and get it off the candidates list.
				boundaries.append (std::make_tuple ((*it)->vertex (0), (*it)->vertex (1)));
				break;
			}
		}
	}
}

void MagicWandMode::doMagic (LDObjectPtr obj, MagicWandMode::MagicType type)
{
	if (obj == null)
	{
		if (type == Set)
		{
			getCurrentDocument()->clearSelection();
			g_win->buildObjList();
		}

		return;
	}

	int matchesneeded = 0;
	QVector<BoundaryType> boundaries;
	LDObjectType objtype = obj->type();

	if (type != InternalRecursion)
	{
		_selection.clear();
		_selection.append (obj);
	}

	switch (obj->type())
	{
		case OBJ_Line:
		case OBJ_CondLine:
			matchesneeded = 1;
			break;

		case OBJ_Triangle:
		case OBJ_Quad:
			matchesneeded = 2;
			break;

		default:
			return;
	}

	QVector<LDObjectPtr> candidates;

	// Get the list of objects that touch this object, i.e. share a vertex
	// with this.
	for (int i = 0; i < obj->numVertices(); ++i)
		candidates += _vertices[obj->vertex (i)];

	removeDuplicates (candidates);

	// If we're dealing with surfaces, get a list of boundaries.
	if (matchesneeded > 1)
		fillBoundaries (obj, boundaries, candidates);

	for (LDObjectPtr candidate : candidates)
	{
		try
		{
			// If we're doing this on lines, we need exact type match. Surface types (quads and
			// triangles) can be mixed. Also don't consider self a candidate, and don't consider
			// objects we have already processed.
			if ((candidate == obj) ||
				(candidate->color() != obj->color()) ||
				(_selection.contains (candidate)) ||
				(matchesneeded == 1 && (candidate->type() != objtype)) ||
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

			if (matches.size() < matchesneeded)
				throw 0; // Not enough matches.

			// Check if a boundary gets in between the objects.
			for (auto boundary : boundaries)
			{
				if (eq (matches[0], std::get<0> (boundary), std::get<1> (boundary)) &&
					eq (matches[1], std::get<0> (boundary), std::get<1> (boundary)))
				{
					throw 0;
				}
			}

			_selection.append (candidate);
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
			getCurrentDocument()->clearSelection();
		case Additive:
			for (LDObjectPtr obj : _selection)
				obj->select();
			break;

		case Subtractive:
			for (LDObjectPtr obj : _selection)
				obj->deselect();
			break;

		case InternalRecursion:
			break;
	}

	if (type != InternalRecursion)
		g_win->buildObjList();
}

bool MagicWandMode::mouseReleased (MouseEventData const& data)
{
	if (Super::mouseReleased (data))
		return true;

	MagicType wandtype = MagicWandMode::Set;

	if (data.keymods & Qt::ShiftModifier)
		wandtype = MagicWandMode::Additive;
	elif (data.keymods & Qt::ControlModifier)
		wandtype = MagicWandMode::Subtractive;
	
	doMagic (renderer()->pickOneObject (data.ev->x(), data.ev->y()), wandtype);
	return true;
}
