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
#include "../lddocument.h"

class HeaderEdit : public QWidget
{
	Q_OBJECT

public:
	HeaderEdit(QWidget* parent = nullptr);
	~HeaderEdit();

	void setDocument(LDDocument* document);
	bool hasValidHeader() const;

signals:
	void descriptionChanged(const QString& newDescription);

private:
	class Ui_HeaderEdit& ui;
	class HeaderHistoryModel* headerHistoryModel = nullptr;
	LDHeader* m_header = nullptr;
	Model* m_model = nullptr;

	void moveRows(int direction);
};
