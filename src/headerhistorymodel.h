#pragma once
#include <QAbstractTableModel>

class LDHeader;

class HeaderHistoryModel : public QAbstractTableModel
{
public:
	enum Column
	{
		DateColumn,
		AuthorColumn,
		DescriptionColumn,
	};

	HeaderHistoryModel(LDHeader* header, QObject* parent);

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	void setHeader(LDHeader* header);
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
	LDHeader* header;
};
