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
#include "main.h"
#include "linetypes/modelobject.h"

class AbstractHistoryEntry;

class EditHistory : public QObject
{
	Q_OBJECT

public:
	using Changeset = QList<AbstractHistoryEntry*>;

	EditHistory (LDDocument* document);

	void add (AbstractHistoryEntry* entry);
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
	AbstractHistoryEntry();
	virtual ~AbstractHistoryEntry();

	EditHistory* parent() const;
	virtual void redo() const = 0;
	void setParent (EditHistory* parent);
	virtual void undo() const = 0;

private:
	EditHistory* m_parent;
};

class AddHistoryEntry : public AbstractHistoryEntry
{
public:
	AddHistoryEntry (int idx, LDObject* obj);
	void undo() const override;
	void redo() const override;
	
private:
	int m_index;
	QString m_code;
};

class DelHistoryEntry : public AddHistoryEntry
{
public:
	DelHistoryEntry (int idx, LDObject* obj);
	void undo() const override;
	void redo() const override;
};

class EditHistoryEntry : public AbstractHistoryEntry
{
public:
	EditHistoryEntry (int idx, QString oldCode, QString newCode);
	void undo() const override;
	void redo() const override;
	
private:
	int m_index;
	QString m_oldCode;
	QString m_newCode;
};

class SwapHistoryEntry : public AbstractHistoryEntry
{
public:
	SwapHistoryEntry (int a, int b);
	void undo() const override;
	void redo() const override;

private:
	int m_a;
	int m_b;
};
