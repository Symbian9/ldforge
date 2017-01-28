/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2016 Teemu Piippo
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
#include "ldObject.h"
#include "ldDocument.h"

template<typename T>
class LDObjectIterator
{
public:
	LDObjectIterator (LDDocument* doc) :
		m_list (doc->objects()),
		m_i (-1)
	{
		seekTillValid();
	}

	LDObjectIterator (const LDObjectList& objs) :
		m_list (objs),
		m_i (-1)
	{
		seekTillValid();
	}

	bool outOfBounds() const
	{
		return m_i < 0 or m_i >= length(m_list);
	}

	T* get() const
	{
		return static_cast<T*> (m_list[m_i]);
	}

	bool isValid() const
	{
		return not outOfBounds() and get()->type() == T::SubclassType;
	}

	void seek (int i)
	{
		m_i = i;
	}

	void seekTillValid()
	{
		do ++m_i;
		while (not outOfBounds() and not isValid());
	}

	void rewindTillValid()
	{
		do --m_i;
		while (m_i >= 0 and not isValid());
	}

	int tell() const
	{
		return m_i;
	}

	T* operator*() const
	{
		return get();
	}

	T* operator->() const
	{
		return get();
	}

	void operator++()
	{
		seekTillValid();
	}

	void operator++ (int)
	{
		seekTillValid(); 
	}

	void operator--()
	{
		rewindTillValid();
	}

	void operator-- (int)
	{
		rewindTillValid();
	}

	operator T*()
	{
		return get();
	}

private:
	const LDObjectList& m_list;
	int m_i;
};

template<typename T, typename R>
QVector<T*> filterByType (const R& stuff)
{
	QVector<T*> result;

	for (LDObjectIterator<T> it (stuff); it.isValid(); ++it)
		result << it;

	return result;
}
