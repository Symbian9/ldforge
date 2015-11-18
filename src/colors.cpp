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

void initColors()
{
	print ("Initializing color information.\n");
	static ColorData colors;
	colorData = &colors;
}

bool LDColor::isValid() const
{
	if (isLDConfigColor() and data().name.isEmpty())
		return false; // Unknown LDConfig color

	return m_index != -1;
}

bool LDColor::isLDConfigColor() const
{
	return colorData->contains (index());
}

const ColorData::Entry& LDColor::data() const
{
	return colorData->get (index());
}

QString LDColor::name() const
{
	if (isDirect())
		return "0x" + QString::number (index(), 16).toUpper();
	else if (isLDConfigColor())
		return data().name;
	else if (index() == -1)
		return "null color";
	else
		return "";
}

QString LDColor::hexcode() const
{
	return faceColor().name();
}

QColor LDColor::faceColor() const
{
	if (isDirect())
	{
		QColor color;
		color.setRed ((index() & 0x0FF0000) >> 16);
		color.setGreen ((index() & 0x000FF00) >> 8);
		color.setBlue (index() & 0x00000FF);

		if (index() >= 0x3000000)
			color.setAlpha (128);

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

QColor LDColor::edgeColor() const
{
	if (isDirect())
		return ::luma (faceColor()) < 48 ? Qt::white : Qt::black;
	else if (isLDConfigColor())
		return data().edgeColor;
	else
		return Qt::black;
}

int LDColor::luma() const
{
	return ::luma (faceColor());
}

int LDColor::edgeLuma() const
{
	return ::luma (edgeColor());
}

qint32 LDColor::index() const
{
	return m_index;
}

QString LDColor::indexString() const
{
	if (isDirect())
		return "0x" + QString::number (index(), 16).toUpper();

	return QString::number (index());
}

bool LDColor::isDirect() const
{
	return index() >= 0x02000000;
}

int luma (const QColor& col)
{
	return (0.2126f * col.red()) + (0.7152f * col.green()) + (0.0722f * col.blue());
}

ColorData::ColorData()
{
	if (colorData == nullptr)
		colorData = this;

	// Initialize main and edge colors, they're special like that.
	m_data[MainColor].faceColor =
	m_data[MainColor].hexcode = "#AAAAAA";
	m_data[MainColor].edgeColor = Qt::black;
	m_data[MainColor].name = "Main color";
	m_data[EdgeColor].faceColor =
	m_data[EdgeColor].edgeColor =
	m_data[EdgeColor].hexcode = "#000000";
	m_data[EdgeColor].name = "Edge color";
	loadFromLdconfig();
}

ColorData::~ColorData()
{
	if (colorData == this)
		colorData = nullptr;
}

bool ColorData::contains (int code) const
{
	return code >= 0 and code < EntryCount;
}

const ColorData::Entry& ColorData::get (int code) const
{
	if (not contains (code))
		throw std::runtime_error ("Attempted to get non-existant color information");

	return m_data[code];
}

void ColorData::loadFromLdconfig()
{
	QString path = LDPaths::ldConfigPath();
	QFile fp (path);

	if (not fp.open (QIODevice::ReadOnly))
	{
		QMessageBox::critical (nullptr, "Error", "Unable to open LDConfig.ldr for parsing: " + fp.errorString());
		return;
	}

	// TODO: maybe LDConfig can be loaded as a Document? Or would that be overkill?
	while (not fp.atEnd())
	{
		QString line = QString::fromUtf8 (fp.readLine());

		if (line.isEmpty() or line[0] != '0')
			continue; // empty or illogical

		line.remove ('\r');
		line.remove ('\n');

		// Parse the line
		LDConfigParser parser (line, ' ');
		QString name;
		QString facename;
		QString edgename;
		QString codestring;

		// Check 0 !COLOUR, parse the name
		if (not parser.compareToken (0, "0") or not parser.compareToken (1, "!COLOUR") or not parser.getToken (name, 2))
			continue;

		// Replace underscores in the name with spaces for readability
		name.replace ("_", " ");

		if (not parser.parseTag ("CODE", codestring))
			continue;

		bool ok;
		int code = codestring.toShort (&ok);

		if (not ok or not contains (code))
			continue;

		if (not parser.parseTag ("VALUE", facename) or not parser.parseTag ("EDGE", edgename))
			continue;

		// Ensure that our colors are correct
		QColor faceColor (facename);
		QColor edgeColor (edgename);

		if (not faceColor.isValid() or not edgeColor.isValid())
			continue;

		Entry& entry = m_data[code];
		entry.name = name;
		entry.faceColor = faceColor;
		entry.edgeColor = edgeColor;
		entry.hexcode = facename;

		if (parser.parseTag ("ALPHA", codestring))
			entry.faceColor.setAlpha (qBound (0, codestring.toInt(), 255));
	}
}

// =============================================================================
//
LDConfigParser::LDConfigParser (QString inText, char sep)
{
	m_tokens = inText.split (sep, QString::SkipEmptyParts);
	m_pos = -1;
}

// =============================================================================
//
bool LDConfigParser::getToken (QString& val, const int pos)
{
	if (pos >= m_tokens.size())
		return false;

	val = m_tokens[pos];
	return true;
}

// =============================================================================
//
bool LDConfigParser::findToken (int& result, char const* needle, int args)
{
	for (int i = 0; i < (m_tokens.size() - args); ++i)
	{
		if (m_tokens[i] == needle)
		{
			result = i;
			return true;
		}
	}

	return false;
}

// =============================================================================
//
bool LDConfigParser::compareToken (int inPos, QString text)
{
	QString tok;

	if (not getToken (tok, inPos))
		return false;

	return (tok == text);
}

// =============================================================================
//
// Helper function for parseLDConfig
//
bool LDConfigParser::parseTag (char const* tag, QString& val)
{
	int pos;

	// Try find the token and get its position
	if (not findToken (pos, tag, 1))
		return false;

	// Get the token after it and store it into val
	return getToken (val, pos + 1);
}