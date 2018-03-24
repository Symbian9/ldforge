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

#include "headerhistorymodel.h"
#include "lddocument.h"
#include "generics/migrate.h"

HeaderHistoryModel::HeaderHistoryModel(LDHeader* header, QObject* parent) :
	QAbstractTableModel {parent},
	header {header} {}

int HeaderHistoryModel::rowCount(const QModelIndex&) const
{
	if (this->header)
		return this->header->history.size();
	else
		return 0;
}

int HeaderHistoryModel::columnCount(const QModelIndex&) const
{
	return 3;
}

QVariant HeaderHistoryModel::data(const QModelIndex& index, int role) const
{
	if (this->header and (role == Qt::DisplayRole || role == Qt::EditRole))
	{
		const auto& entry = this->header->history[index.row()];
		switch (static_cast<Column>(index.column()))
		{
		case DateColumn:
			return entry.date;

		case AuthorColumn:
			return entry.author;

		case DescriptionColumn:
			return entry.description;

		default:
			return {};
		}
	}
	else
	{
		return {};
	}
}

QVariant HeaderHistoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal and role == Qt::DisplayRole)
	{
		switch (static_cast<Column>(section))
		{
		case DateColumn:
			return tr("Date");

		case AuthorColumn:
			return tr("Author");

		case DescriptionColumn:
			return tr("Description");

		default:
			return {};
		}
	}
	else
	{
		return {};
	}
}

Qt::ItemFlags HeaderHistoryModel::flags(const QModelIndex& index) const
{
	Qt::ItemFlags flags = QAbstractTableModel::flags(index);

	if (index.isValid())
		flags |= Qt::ItemIsEditable;

	return flags;
}

bool HeaderHistoryModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (role == Qt::EditRole)
	{
		LDHeader::HistoryEntry& entry = this->header->history[index.row()];

		switch (static_cast<Column>(index.column()))
		{
		case DateColumn:
			entry.date = value.toDate();
			return true;

		case AuthorColumn:
			entry.author = value.toString();
			return true;

		case DescriptionColumn:
			entry.description = value.toString();
			return true;

		default:
			return false;
		}
	}
	else
	{
		return false;
	}
}

bool HeaderHistoryModel::moveRows(
	const QModelIndex&,
	int sourceRow,
	int count,
	const QModelIndex&,
	int destinationRow
) {
	int sourceRowLast = sourceRow + count - 1;
	this->beginMoveRows({}, sourceRow, sourceRowLast, {}, destinationRow);
	::migrate(this->header->history, sourceRow, sourceRowLast, destinationRow);
	this->endMoveRows();
	return true;
}

bool HeaderHistoryModel::removeRows(int row, int count, const QModelIndex&)
{
	if (row >= 0 and row + count - 1 < this->rowCount())
	{
		this->beginRemoveRows({}, row, row + count - 1);
		this->header->history.remove(row, count);
		this->endRemoveRows();
		return true;
	}
	else
	{
		return false;
	}
}

bool HeaderHistoryModel::insertRows(int startRow, int count, const QModelIndex&)
{
	if (startRow >= 0 and startRow <= this->rowCount())
	{
		this->beginInsertRows({}, startRow, startRow + count - 1);

		for (int row : range(startRow, startRow + 1, startRow + count - 1))
		{
			this->header->history.insert(row, {});
			this->header->history[row].date = QDate::currentDate();
			this->header->history[row].author = config::defaultUser();
		}

		this->endInsertRows();
		return true;
	}
	else
	{
		return false;
	}
}

void HeaderHistoryModel::setHeader(LDHeader* header)
{
	emit layoutAboutToBeChanged();
	this->header = header;
	emit layoutChanged();
}
