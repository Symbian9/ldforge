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
#include <QColor>
#include "basics.h"

/*
 * ColorData
 *
 * This class contains the model data for LDConfig-defined colors.
 */
class ColorData
{
public:
	struct Entry
	{
		QString name;
		QColor faceColor;
		QColor edgeColor;
	};

	ColorData();
	void loadFromLdconfig();
	void loadFromFile(QIODevice& device);
	bool contains(int code) const;
	const Entry& get(int code) const;

private:
	Entry m_data[512];
};

/*
 * LDColor
 *
 * This class represents an LDraw color. For a color index, this class provides information for that color.
 * It only contains the index of the color. Therefore it is a wrapper around an integer and should be passed by value.
 */
class LDColor
{
public:
	LDColor();
	explicit LDColor(qint32 index);
	LDColor(const LDColor& other) = default;

	bool isLDConfigColor() const;
	bool isValid() const;
	QString name() const;
	QString hexcode() const;
	QColor faceColor() const;
	QColor edgeColor() const;
	qint32 index() const;
	bool isDirect() const;
	QString indexString() const;

	static const LDColor nullColor;
	static LDColor directColor(QColor color, bool transparent = false);
	static void initColors();

	LDColor& operator=(qint32 index) { m_index = index; return *this; }
	LDColor& operator=(const LDColor &other) = default;
	LDColor operator++() { return LDColor {++m_index}; }
	LDColor operator++(int) { return LDColor {m_index++}; }
	LDColor operator--() { return LDColor {--m_index}; }
	LDColor operator--(int) { return LDColor {m_index--}; }
	bool operator==(LDColor other) const { return index() == other.index(); }
	bool operator!=(LDColor other) const { return index() != other.index(); }
	bool operator<(LDColor other) const { return index() < other.index(); }
	bool operator<=(LDColor other) const { return index() <= other.index(); }
	bool operator>(LDColor other) const { return index() > other.index(); }
	bool operator>=(LDColor other) const { return index() >= other.index(); }

private:
	static ColorData* colorData;
	const ColorData::Entry& data() const;

	qint32 m_index;
};

Q_DECLARE_METATYPE(LDColor)
uint qHash(LDColor color);

/*
 * LDConfigParser
 *
 * Parses LDConfig.ldr.
 */
class LDConfigParser
{
public:
	LDConfigParser(QString inputText);

	bool getToken(QString& tokenText, int position);
	bool findToken(int& tokenPosition, QString needle, int args);
	bool compareToken(int inPos, QString text);
	bool parseTag(QString key, QString& value);

private:
	QStringList m_tokens;
};

int luma(const QColor& col);

static const LDColor MainColor {16};
static const LDColor EdgeColor {24};
static const QVector<LDColor> defaultColorToolbar {
	LDColor(4), LDColor(25), LDColor(14), LDColor(27),
	LDColor(2), LDColor(3), LDColor(11), LDColor(1), LDColor(22),
	LDColor::nullColor,
	LDColor(0), LDColor(72), LDColor(71), LDColor(15)
};
