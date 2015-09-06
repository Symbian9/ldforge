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

#pragma once
#include <QMap>

template<typename Key, typename Value>
class DoubleMap
{
public:
	void clear()
	{
		m_map.clear();
		m_reverseMap.clear();
	}

	void insert (const Key& key, const Value& value)
	{
		m_map[key] = value;
		m_reverseMap[value] = key;
	}

	bool containsKey (const Key& key) const
	{
		return m_map.contains (key);
	}

	bool containsValue (const Value& value) const
	{
		return m_reverseMap.contains (value);
	}

	void removeKey (const Key& key)
	{
		m_reverseMap.remove (m_map[key]);
		m_map.remove (key);
	}

	void removeValue (const Key& key)
	{
		m_reverseMap.remove (m_map[key]);
		m_map.remove (key);
	}

	Value& lookup (const Key& key)
	{
		return m_map[key];
	}

	const Value& lookup (const Key& key) const
	{
		return m_map[key];
	}

	Key& reverseLookup (const Value& key)
	{
		return m_reverseMap[key];
	}

	const Key& reverseLookup (const Value& key) const
	{
		return m_reverseMap[key];
	}

	Value* find (const Key& key)
	{
		auto iterator = m_map.find (key);
		return iterator == m_map.end() ? NULL : &(*iterator);
	}

	Key* reverseFind (const Value& value)
	{
		auto iterator = m_reverseMap.find (value);
		return iterator == m_reverseMap.end() ? NULL : &(*iterator);
	}

	Value& operator[] (const Key& key)
	{
		return lookup (key);
	}

	const Value& operator[] (const Key& key) const
	{
		return lookup (key);
	}

private:
	QMap<Key, Value> m_map;
	QMap<Value, Key> m_reverseMap;
};
