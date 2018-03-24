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

#include "editHistory.h"
#include "lddocument.h"

EditHistory::EditHistory (LDDocument* document) :
	m_document (document),
	m_isIgnoring (false),
	m_position (-1) {}

EditHistory::~EditHistory()
{
	clear();
}

void EditHistory::undo()
{
	if (m_changesets.isEmpty() or position() == -1)
		return;

	// Don't take the changes done here as actual edits to the document
	setIgnoring (true);
	const Changeset& set = changesetAt (position());

	// Iterate the list in reverse and undo all actions
	for (int i = countof(set) - 1; i >= 0; --i)
	{
		AbstractHistoryEntry* change = set[i];
		change->undo();
	}

	m_position--;
	setIgnoring (false);
	emit undone();
}

void EditHistory::redo()
{
	if (position() == countof(m_changesets))
		return;

	setIgnoring (true);
	const Changeset& set = changesetAt (position() + 1);

	// Redo things in original order
	for (AbstractHistoryEntry* change : set)
		change->redo();

	++m_position;
	setIgnoring (false);
	emit redone();
}

void EditHistory::clear()
{
	for (Changeset set : m_changesets)
	for (AbstractHistoryEntry* change : set)
		delete change;

	m_changesets.clear();
}

void EditHistory::addStep()
{
	if (m_currentChangeset.isEmpty())
		return;

	while (position() < size() - 1)
	{
		Changeset last = m_changesets.last();

		for (AbstractHistoryEntry* entry : last)
			delete entry;

		m_changesets.removeLast();
	}

	m_changesets << m_currentChangeset;
	m_currentChangeset.clear();
	++m_position;
	emit stepAdded();
}

int EditHistory::size() const
{
	return countof(m_changesets);
}

const EditHistory::Changeset& EditHistory::changesetAt (int pos) const
{
	return m_changesets[pos];
}

int EditHistory::position()
{
	return m_position;
}

bool EditHistory::isIgnoring() const
{
	return m_isIgnoring;
}

void EditHistory::setIgnoring (bool value)
{
	m_isIgnoring = value;
}

LDDocument* EditHistory::document() const
{
	return m_document;
}

AbstractHistoryEntry::AbstractHistoryEntry(EditHistory* parent) :
	m_parent {parent} {}

AbstractHistoryEntry::~AbstractHistoryEntry() {}

EditHistory* AbstractHistoryEntry::parent() const
{
	return m_parent;
}

AddHistoryEntry::AddHistoryEntry(const QModelIndex& index, EditHistory* parent) :
	AbstractHistoryEntry {parent},
	m_row {index.row()},
	m_code {Serializer::store(parent->document()->lookup(index))} {}

void AddHistoryEntry::undo()
{
	LDObject* object = parent()->document()->getObject(m_row);
	parent()->document()->remove(object);
}

void AddHistoryEntry::redo()
{
	parent()->document()->insertFromArchive(m_row, m_code);
}

void DelHistoryEntry::undo()
{
	AddHistoryEntry::redo();
}

void DelHistoryEntry::redo()
{
	AddHistoryEntry::undo();
}

EditHistoryEntry::EditHistoryEntry(
	const QModelIndex& index,
	const Serializer::Archive& oldState,
	const Serializer::Archive& newState,
	EditHistory* parent
) :
	AbstractHistoryEntry {parent},
	row {index.row()},
	oldState {oldState},
	newState {newState} {}

void EditHistoryEntry::undo()
{
	parent()->document()->setObjectAt(row, oldState);
}

void EditHistoryEntry::redo()
{
	parent()->document()->setObjectAt(row, newState);
}

MoveHistoryEntry::MoveHistoryEntry(int top, int bottom, int destination, EditHistory* parent) :
	AbstractHistoryEntry {parent},
	top {top},
	bottom {bottom},
	destination {destination} {}

void MoveHistoryEntry::undo()
{
	bool downwards = (destination < top);
	parent()->document()->moveRows(
		{},
		downwards ? (destination) : (destination - (bottom - top) - 1),
		bottom - top + 1,
		{},
		downwards ? (bottom + 1) : top
	);
}

void MoveHistoryEntry::redo()
{
	parent()->document()->moveRows(
		{},
		top,
		bottom - top + 1,
		{},
		destination
	);
}
