#include "headerhistorymodel.h"
#include "lddocument.h"

HeaderHistoryModel::HeaderHistoryModel(LDHeader* header, QObject* parent) :
	QAbstractTableModel {parent},
	header {header} {}

int HeaderHistoryModel::rowCount(const QModelIndex&) const
{
	if (this->header)
		return this->header->history.size();
}

int HeaderHistoryModel::columnCount(const QModelIndex&) const
{
	return 3;
}

QVariant HeaderHistoryModel::data(const QModelIndex& index, int role) const
{
	if (this->header and role == Qt::DisplayRole)
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

void HeaderHistoryModel::setHeader(LDHeader* header)
{
	emit layoutAboutToBeChanged();
	this->header = header;
	emit layoutChanged();
}
