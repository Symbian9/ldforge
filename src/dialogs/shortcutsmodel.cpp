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

#include <QKeySequenceEdit>
#include <QSettings>
#include "mainwindow.h"
#include "widgets/extendedkeysequenceeditor.h"
#include "shortcutsmodel.h"

/*
 * Constructs a new shortcuts model.
 * Actions are acquired from the provided mainwindow.
 */
ShortcutsModel::ShortcutsModel(MainWindow* parent)
{
	for (QAction* action : parent->findChildren<QAction*>())
	{
		if (not action->objectName().isEmpty())
			shortcuts.append({action, action->shortcut(), parent->defaultShortcut(action)});
	}
}

/*
 * Returns the amount of shortcuts.
 */
int ShortcutsModel::rowCount(const QModelIndex &) const
{
	return shortcuts.size();
}

/*
 * Returns the amount of columns.
 */
int ShortcutsModel::columnCount(const QModelIndex &) const
{
	return 2;
}

/*
 * Returns various shortcut data.
 */
QVariant ShortcutsModel::data(const QModelIndex &index, int role) const
{
	if (not isValidIndex(index))
		return {};

	Item const& entry = shortcuts[index.row()];

	switch (role)
	{
	case Qt::DisplayRole:
		switch (index.column())
		{
		case ActionColumn:
			return entry.action->text().replace("&", "");

		case KeySequenceColumn:
			return entry.sequence.toString(QKeySequence::NativeText);

		default:
			return "";
		}

	case Qt::DecorationRole:
		if (index.column() == ActionColumn)
			return entry.action->icon();
		else
			return {};

	case Qt::EditRole:
		switch (index.column())
		{
		case KeySequenceColumn:
			return QVariant::fromValue(entry.sequence);

		default:
			return {};
		}

	case DefaultKeySequenceRole:
		return entry.defaultSequence;

	default:
		return {};
	}
}

/*
 * Reimplementation of QAbstractItemModel::flags that supplies the editable flag
 * for the key sequence cells.
 */
Qt::ItemFlags ShortcutsModel::flags(const QModelIndex& index) const
{
	Qt::ItemFlags flags = QAbstractItemModel::flags(index);

	if (index.column() == KeySequenceColumn)
		flags |= Qt::ItemIsEditable;

	return flags;
}

/*
 * Returns whether or not the specified row is valid.
 */
bool ShortcutsModel::isValidRow(int row) const
{
	return row >= 0 and row < shortcuts.size();
}

/*
 * Returns whether or not the specified model index is valid.
 */
bool ShortcutsModel::isValidIndex(const QModelIndex &index) const
{
	return index.column() >= 0 and index.column() < 2 and isValidRow(index.row());
}

/*
 * Provides an interface for changing the key sequence.
 */
bool ShortcutsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (isValidIndex(index) and index.column() == KeySequenceColumn and role == Qt::EditRole)
	{
		shortcuts[index.row()].sequence = value.value<QKeySequence>();
		return true;
	}
	else
	{
		return false;
	}
}

/*
 * Saves shortcuts to the settings object and updates the actions.
 */
void ShortcutsModel::saveChanges()
{
	for (Item& shortcut : shortcuts)
	{
		shortcut.action->setShortcut(shortcut.sequence);
		QString const key = "shortcut_" + shortcut.action->objectName();

		if (shortcut.defaultSequence != shortcut.sequence)
			settingsObject().setValue(key, shortcut.sequence);
		else
			settingsObject().remove(key);
	}
}

/*
 * Returns an index.
 */
QModelIndex ShortcutsModel::index(int row, int column, const QModelIndex& parent) const
{
	ignore(parent);
	return createIndex(row, column);
}

/*
 * Returns nothing, because the shortcuts model does not use parents.
 */
QModelIndex ShortcutsModel::parent(const QModelIndex &child) const
{
	ignore(child);
	return {};
}

/*
 * Returns headers.
 */
QVariant ShortcutsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole and orientation == Qt::Horizontal)
	{
		switch (section)
		{
		case ActionColumn:
			return "Action";

		case KeySequenceColumn:
			return "Shortcut";

		default:
			return "";
		}
	}
	else
	{
		return {};
	}
}

/*
 * Constructs a new key sequence delegate.
 */
KeySequenceDelegate::KeySequenceDelegate(QObject *parent) :
	QStyledItemDelegate {parent} {}

/*
 * Creates a key sequence editor.
 */
QWidget *KeySequenceDelegate::createEditor(
	QWidget *parent,
	const QStyleOptionViewItem &option,
	const QModelIndex &index) const
{
	ignore(option, index);
	const QVariant variant = index.model()->data(index, ShortcutsModel::DefaultKeySequenceRole);
	const QKeySequence defaultSequence = variant.value<QKeySequence>();
	ExtendedKeySequenceEditor* editor = new ExtendedKeySequenceEditor {{}, defaultSequence, parent};
	return editor;
}

/*
 * Sets the initial key sequence used in the key sequence editor.
 */
void KeySequenceDelegate::setEditorData(QWidget *widget, const QModelIndex &index) const
{
	QKeySequence sequence = index.model()->data(index, Qt::EditRole).value<QKeySequence>();
	ExtendedKeySequenceEditor* editor = static_cast<ExtendedKeySequenceEditor*>(widget);
	editor->setKeySequence(sequence);
}

/*
 * Updates the shortcuts model when the key sequence has been accepted by the user.
 */
void KeySequenceDelegate::setModelData(
	QWidget *widget,
	QAbstractItemModel *model,
	const QModelIndex &index) const
{
	ExtendedKeySequenceEditor* editor = static_cast<ExtendedKeySequenceEditor*>(widget);
	model->setData(index, editor->keySequence(), Qt::EditRole);
}

/*
 * Updates editor geometry.
 */
void KeySequenceDelegate::updateEditorGeometry(
	QWidget *editor,
	const QStyleOptionViewItem &option,
	const QModelIndex &index) const
{
	ignore(index);
	editor->setGeometry(option.rect);
}
