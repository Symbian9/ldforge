#pragma once
#include <QAbstractTableModel>
#include "main.h"

class LibrariesModel : public QAbstractTableModel
{
public:
	enum Column { RoleColumn, PathColumn };

	LibrariesModel(Libraries& libraries, QObject* parent);

	QVariant data(const QModelIndex& index, int role) const override;
	int rowCount(const QModelIndex& parent = {}) const override;
	int columnCount(const QModelIndex& parent = {}) const override;
	bool setData(const QModelIndex& index, const QVariant& value, int role) override;
	Qt::ItemFlags flags(const QModelIndex& index) const override;
	bool moveRows(
		const QModelIndex&,
		int sourceRow,
		int count,
		const QModelIndex&,
		int destinationRow
	) override;
	bool removeRows(int row, int count, const QModelIndex&) override;
	bool insertRows(int startRow, int count, const QModelIndex&) override;

private:
	Libraries& libraries;
};
