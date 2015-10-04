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
		return m_i < 0 or m_i >= m_list.size();
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
