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

#include "librariesmodel.h"
#include "generics/migrate.h"

LibrariesModel::LibrariesModel(Libraries& libraries, QObject* parent) :
	QAbstractTableModel {parent},
	libraries {libraries} {}

QString libraryRoleName(decltype(Library::role) role)
{
	switch (role)
	{
	case Library::ReadOnlyStorage:
		return QObject::tr("Storage");

	case Library::UnofficialFiles:
		return QObject::tr("Unofficial files");

	case Library::WorkingDirectory:
		return QObject::tr("Working directory");

	default:
		return "";
	}
}

int LibrariesModel::rowCount(const QModelIndex&) const
{
	return this->libraries.size();
}

int LibrariesModel::columnCount(const QModelIndex&) const
{
	return 2;
}

QVariant LibrariesModel::data(const QModelIndex& index, int role) const
{
	Column column = static_cast<Column>(index.column());

	if (index.row() >= 0 and index.row() < this->rowCount())
	{
		const Library& library = this->libraries[index.row()];

		switch (column)
		{
		case PathColumn:
			switch (role)
			{
			case Qt::DisplayRole:
			case Qt::EditRole:
				return library.path;
			}
			break;

		case RoleColumn:
			switch (role)
			{
			case Qt::DisplayRole:
				return libraryRoleName(library.role);

			case Qt::EditRole:
				return library.role;
			}
			break;
		}
	}

	return {};
}

bool LibrariesModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (index.row() >= 0 and index.row() < this->rowCount({}) and role == Qt::EditRole)
	{
		Library& library = this->libraries[index.row()];
		Column column = static_cast<Column>(index.column());

		switch (column)
		{
		case PathColumn:
			library.path = value.toString();
			return true;

		case RoleColumn:
			{
				int intValue = value.toInt();

				if (intValue >= 0 and intValue < 3)
				{
					library.role = static_cast<decltype(Library::role)>(intValue);
					return true;
				}
			}
			break;
		}
	}

	return false;
}

Qt::ItemFlags LibrariesModel::flags(const QModelIndex& index) const
{
	Qt::ItemFlags flags = QAbstractTableModel::flags(index);

	if (index.isValid())
		flags |= Qt::ItemIsEditable;

	return flags;
}

bool LibrariesModel::moveRows(
	const QModelIndex&,
	int sourceRow,
	int count,
	const QModelIndex&,
	int destinationRow
) {
	int sourceRowLast = sourceRow + count - 1;
	this->beginMoveRows({}, sourceRow, sourceRowLast, {}, destinationRow);
	::migrate(this->libraries, sourceRow, sourceRowLast, destinationRow);
	this->endMoveRows();
	return true;
}

bool LibrariesModel::removeRows(int row, int count, const QModelIndex&)
{
	if (row >= 0 and row + count - 1 < this->rowCount())
	{
		this->beginRemoveRows({}, row, row + count - 1);
		this->libraries.remove(row, count);
		this->endRemoveRows();
		return true;
	}
	else
	{
		return false;
	}
}

bool LibrariesModel::insertRows(int startRow, int count, const QModelIndex&)
{
	if (startRow >= 0 and startRow <= this->rowCount())
	{
		this->beginInsertRows({}, startRow, startRow + count - 1);

		for (int row : range(startRow, startRow + 1, startRow + count - 1))
			this->libraries.insert(row, {});

		this->endInsertRows();
		return true;
	}
	else
	{
		return false;
	}
}
