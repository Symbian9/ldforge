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
#include "main.h"
#include "model.h"

struct LDHeader;

//
// DocumentLoader
//
// Loads the given file and parses it to LDObjects. It's a separate class so as to be able to do the work progressively
// through the event loop, allowing the program to maintain responsivity during loading.
//
class DocumentLoader : public QObject
{
	Q_OBJECT

public:
	DocumentLoader(
		Model* model,
		LDHeader& header,
		bool onForeground = false,
		QObject* parent = nullptr,
	);

	Q_SLOT void abort();
	bool hasAborted();
	bool isDone() const;
	bool isOnForeground() const;
	const QVector<LDObject*>& objects() const;
	int progress() const;
	void read (QIODevice* fp);
	Q_SLOT void start();
	int warningCount() const;

private:
	class OpenProgressDialog* m_progressDialog;
	Model* _model;
	LDHeader& _header;
	QStringList m_lines;
	int m_progress;
	int m_warningCount = 0;
	bool m_isDone = false;
	bool m_hasAborted = false;
	const bool m_isOnForeground = false;

private slots:
	void work (int i);

signals:
	void progressUpdate (int progress);
	void workDone();
	void parseErrorMessage(QString message);
};
