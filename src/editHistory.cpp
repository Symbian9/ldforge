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

#include "editHistory.h"
#include "ldObject.h"
#include "ldDocument.h"
#include "miscallenous.h"
#include "mainwindow.h"
#include "glRenderer.h"

EditHistory::EditHistory (LDDocument* document) :
	m_document (document),
	m_isIgnoring (false),
	m_position (-1) {}

void EditHistory::undo()
{
	if (m_changesets.isEmpty() or position() == -1)
		return;

	// Don't take the changes done here as actual edits to the document
	setIgnoring (true);
	const Changeset& set = changesetAt (position());

	// Iterate the list in reverse and undo all actions
	for (int i = set.size() - 1; i >= 0; --i)
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
	if (position() == m_changesets.size())
		return;

	setIgnoring (true);
	const Changeset& set = changesetAt (position() + 1);

	// Redo things in original order
	for (const AbstractHistoryEntry* change : set)
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

void EditHistory::add (AbstractHistoryEntry* entry)
{
	if (isIgnoring())
	{
		delete entry;
		return;
	}

	entry->setParent (this);
	m_currentChangeset << entry;
}

int EditHistory::size() const
{
	return m_changesets.size();
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

//
// ---------------------------------------------------------------------------------------------------------------------
//

AbstractHistoryEntry::AbstractHistoryEntry() {}
AbstractHistoryEntry::~AbstractHistoryEntry() {}

EditHistory* AbstractHistoryEntry::parent() const
{
	return m_parent;
}

void AbstractHistoryEntry::setParent (EditHistory* parent)
{
	m_parent = parent;
}

//
// ---------------------------------------------------------------------------------------------------------------------
//

AddHistoryEntry::AddHistoryEntry (int idx, LDObject* obj) :
	m_index (idx),
	m_code (obj->asText()) {}

void AddHistoryEntry::undo() const
{
	parent()->document()->getObject (m_index)->destroy();
}

void AddHistoryEntry::redo() const
{
	parent()->document()->insertObj (m_index, ParseLine (m_code));
}

//
// ---------------------------------------------------------------------------------------------------------------------
//

DelHistoryEntry::DelHistoryEntry (int idx, LDObject* obj) :
	AddHistoryEntry (idx, obj) {}

void DelHistoryEntry::undo() const
{
	AddHistoryEntry::redo();
}

void DelHistoryEntry::redo() const
{
	AddHistoryEntry::undo();
}

//
// ---------------------------------------------------------------------------------------------------------------------
//

EditHistoryEntry::EditHistoryEntry (int idx, QString oldCode, QString newCode) :
	m_index (idx),
	m_oldCode (oldCode),
	m_newCode (newCode) {}

void EditHistoryEntry::undo() const
{
	LDObject* obj = parent()->document()->getObject (m_index);
	LDObject* newobj = ParseLine (m_oldCode);
	obj->replace (newobj);
}

void EditHistoryEntry::redo() const
{
	LDObject* obj = parent()->document()->getObject (m_index);
	LDObject* newobj = ParseLine (m_newCode);
	obj->replace (newobj);
}

//
// ---------------------------------------------------------------------------------------------------------------------
//

SwapHistoryEntry::SwapHistoryEntry (int a, int b) :
	m_a (a),
	m_b (b) {}


void SwapHistoryEntry::undo() const
{
	LDObject::fromID (m_a)->swap (LDObject::fromID (m_b));
}

void SwapHistoryEntry::redo() const
{
	undo();
}
