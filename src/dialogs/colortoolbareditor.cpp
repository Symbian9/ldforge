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

#include <QListView>
#include "colortoolbareditor.h"
#include "ui_colortoolbareditor.h"
#include "guiutilities.h"
#include "colorselector.h"

/*
 * Constructs a new color toolbar editor widget.
 */
ColorToolbarEditor::ColorToolbarEditor(QWidget *parent) :
	QWidget {parent},
	colorToolbar {config::quickColorToolbar()},
	model {colorToolbar},
	ui {*new Ui_ColorToolbarEditor}
{
	ui.setupUi(this);
	ui.colorToolbarView->setModel(&model);
	connect(ui.quickColor_add, &QAbstractButton::clicked, this, &ColorToolbarEditor::addColor);
	connect(ui.quickColor_remove, &QAbstractButton::clicked, this, &ColorToolbarEditor::removeColor);
	connect(ui.quickColor_edit, &QAbstractButton::clicked, this, &ColorToolbarEditor::editColor);
	connect(ui.quickColor_addSep, &QAbstractButton::clicked, this, &ColorToolbarEditor::addSeparator);
	connect(ui.quickColor_moveUp, &QAbstractButton::clicked, this, &ColorToolbarEditor::moveColor);
	connect(ui.quickColor_moveDown, &QAbstractButton::clicked, this, &ColorToolbarEditor::moveColor);
	connect(ui.quickColor_clear, &QAbstractButton::clicked, this, &ColorToolbarEditor::clearColors);
}

/*
 * Destroys the color toolbar editor widget.
 */
ColorToolbarEditor::~ColorToolbarEditor()
{
	delete &ui;
}

/*
 * Returns where a new color toolbar entry should be inserted to.
 * If the user has selected an entry, the new entry is placed below it.
 * Otherwise it goes to the end of the toolbar.
 */
int ColorToolbarEditor::newItemPosition()
{
	QModelIndexList const indexes = ui.colorToolbarView->selectionModel()->selectedIndexes();

	if (indexes.size() > 0)
		return indexes.last().row() + 1;
	else
		return model.rowCount();
}

/*
 * Adds a new color toolbar entry
 */
void ColorToolbarEditor::addColor()
{
	LDColor value;

	if (not ColorSelector::selectColor (this, value, LDColor::nullColor))
		return;

	int const position = newItemPosition();
	model.insertRow(position);
	model.setColorAt(model.index(position), value);
}

/*
 * Changes an existing color toolbar entry
 */
void ColorToolbarEditor::editColor()
{
	QModelIndexList const indexes = ui.colorToolbarView->selectionModel()->selectedIndexes();

	if (indexes.size() > 0)
	{
		QModelIndex const& position = indexes[0];
		LDColor const color = model.colorAt(position);

		if (color == LDColor::nullColor)
			return; // don't color separators

		LDColor newColor;

		if (ColorSelector::selectColor(this, newColor, color))
			model.setColorAt(position, newColor);
	}
}

//
// Remove a quick color
//
void ColorToolbarEditor::removeColor()
{
	QModelIndexList const selection = ui.colorToolbarView->selectionModel()->selectedIndexes();

	if (not selection.empty())
		model.removeRow(selection[0].row());
}

//
// Move a quick color up/down
//
void ColorToolbarEditor::moveColor()
{
	bool const up = (static_cast<QPushButton*>(sender()) == ui.quickColor_moveUp);
	QModelIndexList const selection = ui.colorToolbarView->selectionModel()->selectedIndexes();

	if (not selection.empty())
		model.moveColor(selection[0], up);
}

/*
 * Adds a new separator into the color toolbar.
 */
void ColorToolbarEditor::addSeparator()
{
	int const position = newItemPosition();
	model.insertRow(position);
	model.setColorAt(model.index(position), LDColor::nullColor);
}

/*
 * Clears the color toolbar.
 */
void ColorToolbarEditor::clearColors()
{
	model.removeRows(0, model.rowCount());
}

/*
 * Saves the changes done in the color toolbar editor
 */
void ColorToolbarEditor::saveChanges()
{
	config::setQuickColorToolbar(colorToolbar);
}

/*
 * Constructs a new model for editing the color toolbar.
 */
ColorToolbarModel::ColorToolbarModel(QVector<LDColor> &colorToolbar) :
	colorToolbar {colorToolbar} {}

/*
 * Returns the amount of entries in the color toolbar.
 */
int ColorToolbarModel::rowCount(QModelIndex const&) const
{
	return colorToolbar.size();
}

/*
 * Returns data of the color toolbar.
 */
QVariant ColorToolbarModel::data(const QModelIndex& index, int role) const
{
	int const row = index.row();

	if (row < 0 or row >= rowCount())
		return {};

	LDColor const color = colorToolbar[row];

	switch(role)
	{
	case Qt::DecorationRole:
		if (color == LDColor::nullColor)
			return {};
		else
			return makeColorIcon(color, 16);

	case Qt::DisplayRole:
		if (color == LDColor::nullColor)
			return "";
		else
			return color.name();

	default:
		return {};
	}
}

/*
 * Returns a color in the color toolbar.
 */
LDColor ColorToolbarModel::colorAt(QModelIndex const& index) const
{
	if (isValidIndex(index))
		return colorToolbar[index.row()];
	else
		return {};
}

/*
 * Changes a color in the color toolbar.
 */
void ColorToolbarModel::setColorAt(QModelIndex const& index, LDColor newColor)
{
	if (isValidIndex(index))
	{
		colorToolbar[index.row()] = newColor;
		emit dataChanged(index, index);
	}
}

/*
 * Moves a color up or down in the color toolbar.
 */
void ColorToolbarModel::moveColor(const QModelIndex &index, bool const up)
{
	int const position = index.row();
	int const destination = position + (up ? -1 : 1);
	int const end = destination + (up ? 0 : 1);

	if (isValidRow(position) and isValidRow(destination))
	{
		emit beginMoveRows({}, position, position, {}, end);
		qSwap(colorToolbar[destination], colorToolbar[position]);
		emit endMoveRows();
	}
}

/*
 * Inserts entries in the color toolbar.
 */
bool ColorToolbarModel::insertRows(int row, int count, QModelIndex const&)
{
	if (row >= 0 and row <= colorToolbar.size())
	{
		emit beginInsertRows({}, row, count);
		while (count > 0)
		{
			colorToolbar.insert(row, {});
			count -= 1;
			row += 1;
		}
		emit endInsertRows();
		return true;
	}
	else
	{
		return false;
	}
}

/*
 * Removes entries in the color toolbar.
 */
bool ColorToolbarModel::removeRows(int row, int count, QModelIndex const&)
{
	if (row >= 0 and row + count <= colorToolbar.size())
	{
		emit beginRemoveRows({}, row, row);
		while (count > 0)
		{
			colorToolbar.removeAt(row);
			count -= 1;
		}
		emit endRemoveRows();
		return true;
	}
	else
	{
		return false;
	}
}

/*
 * Returns whether or not the specified index is valid in the color toolbar.
 */
bool ColorToolbarModel::isValidIndex(const QModelIndex &index) const
{
	return isValidRow(index.row());
}

/*
 * Returns whether or not the specified row is valid in the color toolbar.
 */
bool ColorToolbarModel::isValidRow(int row) const
{
	return row >= 0 and row < colorToolbar.size();
}
