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
