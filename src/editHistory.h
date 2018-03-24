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
#include "main.h"
#include "serializer.h"
#include "linetypes/modelobject.h"

class AbstractHistoryEntry;

class EditHistory : public QObject
{
	Q_OBJECT

public:
	using Changeset = QList<AbstractHistoryEntry*>;

	EditHistory (LDDocument* document);
	~EditHistory();

	template<typename T, typename... Args>
	void add(Args&&... args)
	{
		if (not isIgnoring())
			m_currentChangeset << new T {args..., this};
	}

	void addStep();
	const Changeset& changesetAt (int pos) const;
	void clear();
	LDDocument* document() const;
	bool isIgnoring() const;
	int position();
	void redo();
	void setIgnoring (bool value);
	int size() const;
	void undo();

signals:
	void undone();
	void redone();
	void stepAdded();

private:
	LDDocument* m_document;
	Changeset m_currentChangeset;
	QList<Changeset> m_changesets;
	bool m_isIgnoring;
	int m_position;
};

class AbstractHistoryEntry
{
public:
	AbstractHistoryEntry(EditHistory* parent);
	virtual ~AbstractHistoryEntry();

	EditHistory* parent() const;
	virtual void redo() = 0;
	virtual void undo() = 0;

private:
	EditHistory* const m_parent;
};

class AddHistoryEntry : public AbstractHistoryEntry
{
public:
	AddHistoryEntry (const QModelIndex& index, EditHistory* parent);
	void undo() override;
	void redo() override;
	
private:
	int m_row;
	Serializer::Archive m_code;
};

class DelHistoryEntry : public AddHistoryEntry
{
public:
	using AddHistoryEntry::AddHistoryEntry;
	void undo() override;
	void redo() override;
};

class EditHistoryEntry : public AbstractHistoryEntry
{
public:
	EditHistoryEntry(
		const QModelIndex& index,
		const Serializer::Archive& oldCode,
		const Serializer::Archive& newCode,
		EditHistory* parent
	);
	void undo() override;
	void redo() override;
	
private:
	int row;
	Serializer::Archive oldState;
	Serializer::Archive newState;
};

class MoveHistoryEntry : public AbstractHistoryEntry
{
public:
	MoveHistoryEntry(int top, int bottom, int destination, EditHistory* parent);
	void undo() override;
	void redo() override;

private:
	int top;
	int bottom;
	int destination;
};
