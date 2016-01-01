/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2016 Teemu Piippo
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

class OpenProgressDialog : public QDialog
{
	Q_OBJECT

public:
	OpenProgressDialog (QWidget* parent = nullptr, Qt::WindowFlags f = 0);
	virtual ~OpenProgressDialog();

	int progress() const { return m_progress; }
	int numLines() const { return m_numLines; }
	void setNumLines (int value);

public slots:
	void setProgress (int progress);

private:
	class Ui_OpenProgressUI& ui;
	int m_progress;
	int m_numLines;

	void updateValues();
};