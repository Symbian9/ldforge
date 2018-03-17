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

	int rowCount(const QModelIndex& parent = {}) const override;
	int columnCount(const QModelIndex& parent = {}) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex& index, const QVariant& value, int role) override;
	void setHeader(LDHeader* header);
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	bool insertRows(int row, int count, const QModelIndex&) override;
	bool removeRows(int row, int count, const QModelIndex&) override;
	bool moveRows(
		const QModelIndex&,
		int sourceRow,
		int count,
		const QModelIndex&,
		int destinationRow
	) override;
	Qt::ItemFlags flags(const QModelIndex& index) const override;

private:
	LDHeader* header;
};
