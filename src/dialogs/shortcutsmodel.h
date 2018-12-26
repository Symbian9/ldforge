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
#include <QAbstractItemModel>
#include <QAction>
#include <QStyledItemDelegate>
#include "main.h"

/*
 * Implements a delegate for editing key sequence cells.
 */
class KeySequenceDelegate : public QStyledItemDelegate
{
public:
	KeySequenceDelegate(QObject* parent = nullptr);
	QWidget* createEditor(
		QWidget *parent,
		const QStyleOptionViewItem &option,
		const QModelIndex &index) const override;
	void setEditorData(QWidget *widget, const QModelIndex &index) const override;
	void setModelData(
		QWidget *widget,
		QAbstractItemModel *model,
		const QModelIndex &index) const override;
	void updateEditorGeometry(
		QWidget *editor,
		const QStyleOptionViewItem &option,
		const QModelIndex &index) const override;
};

/*
 * Models a table of shortcuts. Each action in the provided main window is given
 * a row, which contains editable shortcuts.
 *
 * Calling saveChanges updates the actions and updates the settings object.
 */
class ShortcutsModel : public QAbstractItemModel
{
public:
	enum Column
	{
		ActionColumn,
		KeySequenceColumn
	};

	enum
	{
		DefaultKeySequenceRole = Qt::UserRole
	};

	ShortcutsModel(class MainWindow* parent);
	int rowCount(QModelIndex const&) const override;
	int columnCount(QModelIndex const&) const override;
	QVariant data(const QModelIndex &index, int role) const override;
	Qt::ItemFlags flags(const QModelIndex& index) const override;
	bool isValidRow(int row) const;
	bool isValidIndex(const QModelIndex &index) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;
	void saveChanges();
	QModelIndex index(int row, int column, const QModelIndex& parent) const override;
	QModelIndex parent(const QModelIndex& child) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
	struct Item
	{
		QAction* action;
		QKeySequence sequence;
		const QKeySequence defaultSequence;
	};

	QVector<Item> shortcuts;
};
