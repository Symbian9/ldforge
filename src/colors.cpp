/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Teemu Piippo
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
#include "main.h"
#include "colors.h"
#include "ldDocument.h"
#include "miscallenous.h"
#include "mainWindow.h"

struct ColorDataEntry
{
	QString name;
	QString hexcode;
	QColor faceColor;
	QColor edgeColor;
};

static ColorDataEntry ColorData[512];

void InitColors()
{
	print ("Initializing color information.\n");

	// Always make sure there's 16 and 24 available. They're special like that.
	ColorData[MainColor].faceColor =
	ColorData[MainColor].hexcode = "#AAAAAA";
	ColorData[MainColor].edgeColor = Qt::black;
	ColorData[MainColor].name = "Main color";

	ColorData[EdgeColor].faceColor =
	ColorData[EdgeColor].edgeColor =
	ColorData[EdgeColor].hexcode = "#000000";
	ColorData[EdgeColor].name = "Edge color";

	parseLDConfig();
}

bool LDColor::isValid() const
{
	if (isLDConfigColor() and ColorData[index()].name.isEmpty())
		return false; // Unknown LDConfig color

	return m_index != -1;
}

bool LDColor::isLDConfigColor() const
{
	return index() >= 0 and index() < countof (ColorData);
}

QString LDColor::name() const
{
	if (isDirect())
		return "0x" + QString::number (index(), 16).toUpper();
	else if (isLDConfigColor())
		return ColorData[index()].name;
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
		return ColorData[index()].faceColor;
	}
	else
	{
		return Qt::black;
	}
}

QColor LDColor::edgeColor() const
{
	if (isDirect())
		return Luma (faceColor()) < 48 ? Qt::white : Qt::black;
	else if (isLDConfigColor())
		return ColorData[index()].edgeColor;
	else
		return Qt::black;
}

int LDColor::luma() const
{
	return Luma (faceColor());
}

int LDColor::edgeLuma() const
{
	return Luma (edgeColor());
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

int Luma (const QColor& col)
{
	return (0.2126f * col.red()) + (0.7152f * col.green()) + (0.0722f * col.blue());
}

int CountLDConfigColors()
{
	return countof (ColorData);
}

void parseLDConfig()
{
	QFile* fp = OpenLDrawFile ("LDConfig.ldr", false);

	if (fp == nullptr)
	{
		Critical (QObject::tr ("Unable to open LDConfig.ldr for parsing."));
		return;
	}

	// Read in the lines
	while (not fp->atEnd())
	{
		QString line = QString::fromUtf8 (fp->readLine());

		if (line.isEmpty() or line[0] != '0')
			continue; // empty or illogical

		line.remove ('\r');
		line.remove ('\n');

		// Parse the line
		LDConfigParser pars (line, ' ');

		int code = 0, alpha = 255;
		QString name, facename, edgename, valuestr;

		// Check 0 !COLOUR, parse the name
		if (not pars.compareToken (0, "0") or
			not pars.compareToken (1, "!COLOUR") or
			not pars.getToken (name, 2))
		{
			continue;
		}

		// Replace underscores in the name with spaces for readability
		name.replace ("_", " ");

		// Get the CODE tag
		if (not pars.parseLDConfigTag ("CODE", valuestr))
			continue;

		// Ensure that the code is within [0 - 511]
		bool ok;
		code = valuestr.toShort (&ok);

		if (not ok or code < 0 or code >= 512)
			continue;

		// VALUE and EDGE tags
		if (not pars.parseLDConfigTag ("VALUE", facename) or not pars.parseLDConfigTag ("EDGE", edgename))
			continue;

		// Ensure that our colors are correct
		QColor faceColor (facename),
			edgeColor (edgename);

		if (not faceColor.isValid() or not edgeColor.isValid())
			continue;

		// Parse alpha if given.
		if (pars.parseLDConfigTag ("ALPHA", valuestr))
			alpha = Clamp (valuestr.toInt(), 0, 255);

		ColorDataEntry& entry = ColorData[code];
		entry.name = name;
		entry.faceColor = faceColor;
		entry.edgeColor = edgeColor;
		entry.hexcode = facename;
		entry.faceColor.setAlpha (alpha);
	}

	fp->close();
	fp->deleteLater();
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
bool LDConfigParser::parseLDConfigTag (char const* tag, QString& val)
{
	int pos;

	// Try find the token and get its position
	if (not findToken (pos, tag, 1))
		return false;

	// Get the token after it and store it into val
	return getToken (val, pos + 1);
}