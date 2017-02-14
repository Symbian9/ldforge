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
#include "linetypes/modelobject.h"
#include "lddocument.h"

template<typename T>
class LDObjectIterator
{
public:
	LDObjectIterator (Model* model) :
	    m_list (model->objects()),
		m_i (-1)
	{
		seekTillValid();
	}

	LDObjectIterator (const QVector<LDObject*>& objs) :
		m_list (objs),
		m_i (-1)
	{
		seekTillValid();
	}

	bool outOfBounds() const
	{
		return m_i < 0 or m_i >= countof(m_list);
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
	const QVector<LDObject*>& m_list;
	int m_i;
};

template<typename T, typename R>
QVector<T*> filterByType (const R& stuff)
{
	QVector<T*> result;

	for (LDObject* object : stuff)
	{
		T* casted = dynamic_cast<T*>(object);
		if (casted != nullptr)
			result << casted;
	}

	return result;
}
