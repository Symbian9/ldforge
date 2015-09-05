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

#include <QComboBox>
#include <QPainter>
#include "colors.h"
#include "guiutilities.h"
#include "ldDocument.h"
#include "mainwindow.h"

GuiUtilities::GuiUtilities (QObject* parent) :
	QObject (parent),
	HierarchyElement (parent) {}

QIcon GuiUtilities::makeColorIcon (LDColor ldcolor, int size)
{
	// Create an image object and link a painter to it.
	QImage img (size, size, QImage::Format_ARGB32);
	QPainter painter (&img);
	QColor color = ldcolor.faceColor();

	if (ldcolor == MainColor)
	{
		// Use the user preferences for main color here
		color = m_config->mainColor();
		color.setAlphaF (m_config->mainColorAlpha());
	}

	// Paint the icon border
	painter.fillRect (QRect (0, 0, size, size), ldcolor.edgeColor());

	// Paint the checkerboard background, visible with translucent icons
	painter.drawPixmap (QRect (1, 1, size - 2, size - 2), GetIcon ("checkerboard"), QRect (0, 0, 8, 8));

	// Paint the color above the checkerboard
	painter.fillRect (QRect (1, 1, size - 2, size - 2), color);
	return QIcon (QPixmap::fromImage (img));
}

void GuiUtilities::fillUsedColorsToComboBox (QComboBox* box)
{
	QMap<LDColor, int> counts;

	for (LDObject* obj : currentDocument()->objects())
	{
		if (not obj->isColored() or not obj->color().isValid())
			continue;

		if (not counts.contains (obj->color()))
			counts[obj->color()] = 1;
		else
			counts[obj->color()] += 1;
	}

	box->clear();
	int row = 0;

	QMapIterator<LDColor, int> it (counts);
	while (it.hasNext())
	{
		it.next();
		QIcon ico = makeColorIcon (it.key(), 16);
		box->addItem (ico, format ("[%1] %2 (%3 object%4)",
			it.key(), it.key().name(), it.value(), plural (it.value())));
		box->setItemData (row, it.key().index());
		++row;
	}
}