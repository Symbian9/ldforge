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
#include <QColor>
#include "basics.h"

class ColorData
{
public:
	struct Entry
	{
		QString name;
		QString hexcode;
		QColor faceColor;
		QColor edgeColor;
	};

	enum { EntryCount = 512 };
	ColorData();
	~ColorData();
	void loadFromLdconfig();
	bool contains (int code) const;
	const Entry& get (int code) const;

private:
	Entry m_data[EntryCount];
};

class LDColor
{
public:
	LDColor() : m_index (0) {}
	LDColor (qint32 index) : m_index (index) {}
	LDColor (const LDColor& other) : m_index (other.m_index) {}

	bool isLDConfigColor() const;
	bool isValid() const;
	QString name() const;
	QString hexcode() const;
	QColor faceColor() const;
	QColor edgeColor() const;
	qint32 index() const;
	int luma() const;
	int edgeLuma() const;
	bool isDirect() const;
	QString indexString() const;

	static LDColor nullColor() { return LDColor (-1); }

	LDColor& operator= (qint32 index) { m_index = index; return *this; }
	LDColor& operator= (LDColor other) { m_index = other.index(); return *this; }
	LDColor operator++() { return ++m_index; }
	LDColor operator++ (int) { return m_index++; }
	LDColor operator--() { return --m_index; }
	LDColor operator-- (int) { return m_index--; }

	bool operator== (LDColor other) const { return index() == other.index(); }
	bool operator!= (LDColor other) const { return index() != other.index(); }
	bool operator< (LDColor other) const { return index() < other.index(); }
	bool operator<= (LDColor other) const { return index() <= other.index(); }
	bool operator> (LDColor other) const { return index() > other.index(); }
	bool operator>= (LDColor other) const { return index() >= other.index(); }

private:
	const ColorData::Entry& data() const;

	qint32 m_index;
};

//
// Parses ldconfig.ldr
//
class LDConfigParser
{
public:
	LDConfigParser (QString inText, char sep);

	bool getToken (QString& val, const int pos);
	bool findToken (int& result, char const* needle, int args);
	bool compareToken (int inPos, QString text);
	bool parseTag (char const* tag, QString& val);

	inline QString operator[] (const int idx)
	{
		return m_tokens[idx];
	}

private:
	QStringList m_tokens;
	int m_pos;
};

void initColors();
int luma (const QColor& col);

enum
{
	MainColor = 16,
	EdgeColor = 24,
};
