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

#include "main.h"
#include "colors.h"
#include "ldDocument.h"
#include "miscallenous.h"
#include "mainWindow.h"
#include "ldConfig.h"
#include <QColor>

static LDColor g_LDConfigColors[512];

void InitColors()
{
	LDColorData* col;
	print ("Initializing color information.\n");

	// Always make sure there's 16 and 24 available. They're special like that.
	col = new LDColorData;
	col->m_faceColor =
	col->m_hexcode = "#AAAAAA";
	col->m_edgeColor = Qt::black;
	g_LDConfigColors[16] = col;

	col = new LDColorData;
	col->m_faceColor =
	col->m_edgeColor =
	col->m_hexcode = "#000000";
	g_LDConfigColors[24] = col;

	LDConfigParser::parseLDConfig();
}

LDColor MainColor()
{
	return g_LDConfigColors[MainColorIndex];
}

LDColor EdgeColor()
{
	return g_LDConfigColors[EdgeColorIndex];
}

void LDColor::addLDConfigColor (qint32 index, LDColor color)
{
	assert (index >= 0 and index < countof (g_LDConfigColors));
	g_LDConfigColors[index] = color;
}

LDColor LDColor::fromIndex (qint32 index)
{
	if (index < countof (g_LDConfigColors) and g_LDConfigColors[index] != null)
		return g_LDConfigColors[index];

	if (index >= 0x2000000)
	{
		// Direct color
		QColor col;
		col.setRed ((index & 0x0FF0000) >> 16);
		col.setGreen ((index & 0x000FF00) >> 8);
		col.setBlue (index & 0x00000FF);

		if (index >= 0x3000000)
			col.setAlpha (128);

		LDColorData* color = new LDColorData;
		color->m_name = "0x" + QString::number (index, 16).toUpper();
		color->m_faceColor = col;
		color->m_edgeColor = Luma (col) < 48 ? Qt::white : Qt::black;
		color->m_hexcode = col.name();
		color->m_index = index;
		return LDColor (color);
	}

	return null;
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

bool LDColor::operator== (LDColor const& other)
{
	if ((data() == nullptr) ^ (other == nullptr))
		return false;

	if (data() != nullptr)
		return index() == other.index();

	// both are null
	return true;
}

int Luma (const QColor& col)
{
	return (0.2126f * col.red()) +
		   (0.7152f * col.green()) +
		   (0.0722f * col.blue());
}

int CountLDConfigColors()
{
	return countof (g_LDConfigColors);
}
