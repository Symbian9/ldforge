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

#include <QComboBox>
#include <QPainter>
#include "colors.h"
#include "guiutilities.h"
#include "lddocument.h"
#include "mainwindow.h"

GuiUtilities::GuiUtilities (QObject* parent) :
	QObject (parent),
	HierarchyElement (parent) {}

/*
 * makeColorIcon
 *
 * Creates an icon to represent an LDraw color.
 */
QIcon GuiUtilities::makeColorIcon (LDColor ldColor, int size)
{
	QImage image {size, size, QImage::Format_ARGB32};
	QPainter painter {&image};
	QColor color;

	if (ldColor == MainColor)
	{
		// Use the user preferences for the main color.
		color = m_config->mainColor();
		color.setAlphaF(m_config->mainColorAlpha());
	}
	else
		color = ldColor.faceColor();

	// Paint the icon border
	painter.fillRect(QRect {0, 0, size, size}, ldColor.edgeColor());

	// Paint the checkerboard background, visible with translucent icons
	painter.drawPixmap(QRect {1, 1, size - 2, size - 2}, GetIcon("checkerboard"), QRect {0, 0, 8, 8});

	// Paint the color above the checkerboard
	painter.fillRect (QRect {1, 1, size - 2, size - 2}, color);
	return QIcon {QPixmap::fromImage(image)};
}

/*
 * fillUsedColorsToComboBox
 *
 * Fills the provided combo box with the colors used in the current document.
 */
void GuiUtilities::fillUsedColorsToComboBox (QComboBox* box)
{
	QMap<LDColor, int> frequencies;

	for (LDObject* object : currentDocument()->objects())
	{
		if (object->isColored() and object->color().isValid())
		{
			if (not frequencies.contains(object->color()))
				frequencies[object->color()] = 1;
			else
				frequencies[object->color()] += 1;
		}
	}

	box->clear();
	int row = 0;
	QMapIterator<LDColor, int> iterator {frequencies};

	while (iterator.hasNext())
	{
		iterator.next();
		LDColor color = iterator.key();
		int frequency = iterator.value();
		QIcon icon = makeColorIcon(color, 16);
		box->addItem(icon, format("[%1] %2 (%3 object%4)", color.index(), color.name(), frequency, plural(frequency)));
		box->setItemData(row, color.index());
		++row;
	}
}

/*
 * mainColorRepresentation
 *
 * Returns the user-preferred appearance for the main color.
 */
QColor GuiUtilities::mainColorRepresentation()
{
	QColor result = {m_config->mainColor()};

	if (result.isValid())
	{
		result.setAlpha(m_config->mainColorAlpha() * 255.f);
		return result;
	}
	else
	{
		return QColor {0, 0, 0};
	}
}

/*
 * loadQuickColorList
 *
 * Returns a list of contents for the color toolbar, based on configuration.
 */
QVector<ColorToolbarItem> GuiUtilities::loadQuickColorList()
{
	QVector<ColorToolbarItem> colors;

	for (QString colorName : m_config->quickColorToolbar().split(":"))
	{
		if (colorName == "|")
		{
			colors << ColorToolbarItem::makeSeparator();
		}
		else
		{
			LDColor color = colorName.toInt();

			if (color.isValid())
				colors << ColorToolbarItem {color, nullptr};
		}
	}

	return colors;
}
