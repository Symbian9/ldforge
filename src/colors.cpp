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

#include <QFile>
#include <QMessageBox>
#include "main.h"
#include "colors.h"
#include "ldDocument.h"
#include "miscallenous.h"
#include "mainwindow.h"
#include "documentmanager.h"
#include "ldpaths.h"

static ColorData* colorData = nullptr;

/*
 * initColors
 *
 * Initializes the color information module.
 */
void initColors()
{
	print ("Initializing color information.\n");
	static ColorData colors;
	colorData = &colors;
}

/*
 * LDColor :: LDColor
 *
 * Default-constructs an LDColor to 0 (black).
 */
LDColor::LDColor() :
    m_index (0) {}

/*
 * LDColor :: LDColor
 *
 * Constructs an LDColor by index.
 */
LDColor::LDColor (qint32 index)
    : m_index (index) {}

/*
 * LDColor :: isValid
 *
 * Returns whether or not the color is valid.
 */
bool LDColor::isValid() const
{
	if (isLDConfigColor() and data().name.isEmpty())
		return false; // Unknown LDConfig color
	else
		return m_index != -1;
}

/*
 * LDColor :: isLDConfigColor
 *
 * Returns whether or not this color is defined in LDConfig.ldr.
 * This is false for e.g. direct colors.
 */
bool LDColor::isLDConfigColor() const
{
	return colorData->contains(index());
}

/*
 * LDColor :: data
 *
 * Returns the ColorData entry for this color.
 */
const ColorData::Entry& LDColor::data() const
{
	return colorData->get(index());
}

/*
 * LDColor :: name
 *
 * Returns the name of this color.
 */
QString LDColor::name() const
{
	if (isDirect())
		return "0x" + QString::number (index(), 16).toUpper();
	else if (isLDConfigColor())
		return data().name;
	else if (index() == -1)
		return "null color";
	else
		return "unknown";
}

/*
 * LDColor :: hexcode
 *
 * Returns the hexadecimal code of this color.
 */
QString LDColor::hexcode() const
{
	return faceColor().name();
}

/*
 * LDColor :: faceColor
 *
 * Returns the color used for surfaces.
 */
QColor LDColor::faceColor() const
{
	if (isDirect())
	{
		// Direct color -- compute from the index.
		QColor color;
		color.setRed((index() & 0x0FF0000) >> 16);
		color.setGreen((index() & 0x000FF00) >> 8);
		color.setBlue(index() & 0x00000FF);

		if (index() >= 0x3000000)
			color.setAlpha(128);

		return color;
	}
	else if (isLDConfigColor())
	{
		return data().faceColor;
	}
	else
	{
		return Qt::black;
	}
}

/*
 * LDColor :: edgeColor
 *
 * Returns the color used for edge lines.
 */
QColor LDColor::edgeColor() const
{
	if (isDirect())
		return luma(faceColor()) < 48 ? Qt::white : Qt::black;
	else if (isLDConfigColor())
		return data().edgeColor;
	else
		return Qt::black;
}

/*
 * LDColor :: index
 *
 * Returns the index number of this color.
 */
qint32 LDColor::index() const
{
	return m_index;
}

/*
 * LDColor :: indexString
 *
 * Returns a string containing the preferred representation of the index.
 */
QString LDColor::indexString() const
{
	if (isDirect())
	{
		// Use hexadecimal notation for direct colors.
		return "0x" + QString::number(index(), 16).toUpper();
	}
	else
	{
		return QString::number(index());
	}
}

/*
 * LDColor :: isDirect
 *
 * Returns whether or not this color is a direct color.
 * Direct colors are picked by RGB value and are not defined in LDConfig.ldr.
 */
bool LDColor::isDirect() const
{
	return index() >= 0x02000000;
}

/*
 * LDColor :: nullColor
 *
 * Returns a color that is guaranteed to be invalid.
 */
LDColor LDColor::nullColor()
{
	return LDColor {-1};
}

/*
 * qHash
 *
 * LDColors are hashed by their index.
 */
uint qHash(LDColor color)
{
	return color.index();
}

/*
 * luma
 *
 * Calculates the luma-value for the given color.
 * c.f. https://en.wikipedia.org/wiki/Luma_(video)
 */
int luma (const QColor& color)
{
	return round((0.2126 * color.red()) + (0.7152 * color.green()) + (0.0722 * color.blue()));
}

/*
 * ColorData :: ColorData
 *
 * Constructs the color data array.
 */
ColorData::ColorData()
{
	if (colorData == nullptr)
		colorData = this;

	// Initialize main and edge colors, they're special like that.
	m_data[MainColor].faceColor = "#AAAAAA";
	m_data[MainColor].edgeColor = Qt::black;
	m_data[MainColor].name = "Main color";
	m_data[EdgeColor].faceColor =
	m_data[EdgeColor].edgeColor = "#000000";
	m_data[EdgeColor].name = "Edge color";

	// Load the rest from LDConfig.ldr.
	loadFromLdconfig();
}

/*
 * ColorData :: ~ColorData
 *
 * Cleanup the colorData pointer after the array is deleted.
 */
ColorData::~ColorData()
{
	if (colorData == this)
		colorData = nullptr;
}

/*
 * ColorData :: contains
 *
 * Returns whether or not the given color index is present in the array.
 */
bool ColorData::contains(int code) const
{
	return code >= 0 and code < length(m_data);
}

/*
 * ColorData :: get
 *
 * Returns an entry in the color array.
 */
const ColorData::Entry& ColorData::get(int code) const
{
	if (not contains(code))
		throw std::runtime_error {"Attempted to get non-existant color information"};

	return m_data[code];
}

/*
 * ColorData :: loadFromLdconfig
 *
 * Loads color information from LDConfig.ldr.
 */
void ColorData::loadFromLdconfig()
{
	QString path = LDPaths::ldConfigPath();
	QFile file {path};

	if (not file.open (QIODevice::ReadOnly))
	{
		QMessageBox::critical(nullptr, "Error", "Unable to open LDConfig.ldr for parsing: " + file.errorString());
		return;
	}

	// TODO: maybe LDConfig can be loaded as a Document? Or would that be overkill?
	while (not file.atEnd())
	{
		QString line = QString::fromUtf8 (file.readLine());

		if (line.isEmpty() or line[0] != '0')
			continue; // empty or illogical

		line.remove('\r');
		line.remove('\n');

		// Parse the line
		LDConfigParser parser = {line};
		QString name;
		QString facename;
		QString edgename;
		QString codestring;

		// Check 0 !COLOUR, parse the name
		if (not parser.compareToken(0, "0") or not parser.compareToken(1, "!COLOUR") or not parser.getToken(name, 2))
			continue;

		// Replace underscores in the name with spaces for readability
		name.replace("_", " ");

		if (not parser.parseTag ("CODE", codestring))
			continue;

		bool ok;
		int code = codestring.toShort(&ok);

		if (not ok or not contains(code))
			continue;

		if (not parser.parseTag("VALUE", facename) or not parser.parseTag("EDGE", edgename))
			continue;

		// Ensure that our colors are correct
		QColor faceColor = {facename};
		QColor edgeColor = {edgename};

		if (not faceColor.isValid() or not edgeColor.isValid())
			continue;

		// Fill in the entry now.
		Entry& entry = m_data[code];
		entry.name = name;
		entry.faceColor = faceColor;
		entry.edgeColor = edgeColor;

		// If the alpha tag is present, fill in that too.
		if (parser.parseTag("ALPHA", codestring))
			entry.faceColor.setAlpha(qBound(0, codestring.toInt(), 255));
	}
}

/*
 * LDConfigParser :: LDConfigParser
 *
 * Constructs the LDConfig.ldr parser.
 */
LDConfigParser::LDConfigParser(QString inputText)
{
	m_tokens = inputText.split (' ', QString::SkipEmptyParts);
}

/*
 * LDConfigParser :: getToken
 *
 * Returns whether or not there is a token at the given position.
 * If there is, fills in the value parameter with it.
 */
bool LDConfigParser::getToken(QString& tokenText, int position)
{
	if (position >= m_tokens.size())
	{
		return false;
	}
	else
	{
		tokenText = m_tokens[position];
		return true;
	}
}

/*
 * LDConfigParser :: findToken
 *
 * Attempts to find the provided token in the parsed LDConfig.ldr line.
 * If found, fills in the first parameter with the position of the token.
 *
 * The args parameter specifies how many arguments (i.e. following tokens) the token needs to have.
 */
bool LDConfigParser::findToken(int& tokenPosition, QString needle, int args)
{
	for (int i = 0; i < (m_tokens.size() - args); ++i)
	{
		if (m_tokens[i] == needle)
		{
			tokenPosition = i;
			return true;
		}
	}

	return false;
}

/*
 * LDConfigParser :: compareToken
 *
 * Returns whether or not the token at the given position has the given text value.
 */
bool LDConfigParser::compareToken (int position, QString text)
{
	QString token;

	if (not getToken(token, position))
		return false;
	else
		return (token == text);
}

/*
 * LDConfig :: parseTag
 *
 * Finds an attribute in the line, and fills in its value.
 * For instance, if the line contains "ALPHA 128", this function can find the "128" for "ALPHA".
 * Returns whether or not the attribute was found.
 */
bool LDConfigParser::parseTag (QString key, QString& value)
{
	int position;

	// Try find the token and get its position
	if (not findToken (position, key, 1))
	{
		return false;
	}
	else
	{
		// Get the token after it and store it in.
		return getToken (value, position + 1);
	}
}
