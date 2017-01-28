/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2017 Teemu Piippo
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
#include <QDialog>
#include "../main.h"

class LDrawPathDialog : public QDialog
{
	Q_OBJECT

public:
	LDrawPathDialog (const QString& defaultPath, bool validDefault, QWidget* parent = nullptr, Qt::WindowFlags f = 0);
	virtual ~LDrawPathDialog();
	QString path() const;
	void setPath (QString path);
	void setStatusText (const QString& statusText, bool ok);

signals:
	void pathChanged (QString newPath);

private:
	const bool m_hasValidDefault;
	class Ui_LDrawPathDialog& ui;
	QPushButton* okButton();
	QPushButton* cancelButton();

private slots:
	void searchButtonClicked();
};