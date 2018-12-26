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
#include <QWidget>
#include <QAbstractListModel>
#include "colors.h"

class ColorToolbarModel : public QAbstractListModel
{
	Q_OBJECT

public:
	ColorToolbarModel(QVector<LDColor>& colorToolbar);
	int rowCount(QModelIndex const& = {}) const override;
	QVariant data(QModelIndex const& index, int role) const override;
	LDColor colorAt(QModelIndex const& index) const;
	void setColorAt(QModelIndex const& index, LDColor newColor);
	bool isValidIndex(QModelIndex const& index) const;
	void moveColor(const QModelIndex &index, const bool up);
	bool insertRows(int row, int count, QModelIndex const& = {}) override;
	bool removeRows(int row, int count, QModelIndex const& = {}) override;

	bool isValidRow(int row) const;
private:
	QVector<LDColor>& colorToolbar;
};

class ColorToolbarEditor : public QWidget
{
	Q_OBJECT

public:
	explicit ColorToolbarEditor(QWidget *parent = nullptr);
	~ColorToolbarEditor();

public slots:
	void addColor();
	void editColor();
	void removeColor();
	void moveColor();
	void addSeparator();
	void clearColors();
	void saveChanges();

private:
	QVector<LDColor> colorToolbar;
	ColorToolbarModel model;
	class Ui_ColorToolbarEditor &ui;
	int newItemPosition();
};
