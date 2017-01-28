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
#include "hierarchyelement.h"

class PartDownloader;

class PartDownloadRequest : public QObject, public HierarchyElement
{
	Q_OBJECT

public:
	enum class State
	{
		Requesting,
		Downloading,
		Finished,
		Failed,
	};

	explicit PartDownloadRequest (QString url, QString dest, bool primary, PartDownloader* parent);
	virtual ~PartDownloadRequest();

	Q_SLOT void abort();
	QString destination() const;
	Q_SLOT void downloadFinished();
	bool failed() const;
	QString filePath() const;
	bool isFinished() const;
	bool isFirstUpdate() const;
	bool isPrimary() const;
	QNetworkReply* networkReply() const;
	qint64 numBytesRead() const;
	qint64 numBytesTotal() const;
	PartDownloader* prompt() const;
	Q_SLOT void readFromNetworkReply();
	void setTableRow (int value);
	int tableRow() const;
	Q_SLOT void updateDownloadProgress (qint64 recv, qint64 total);
	void updateToTable();
	QString url() const;

private:
	int m_tableRow;
	State m_state;
	PartDownloader* m_prompt;
	QString m_url;
	QString m_destination;
	QString m_filePath;
	class QNetworkAccessManager* m_networkManager;
	class QNetworkReply* m_networkReply;
	bool m_isFirstUpdate;
	bool m_isPrimary;
	qint64 m_numBytesRead;
	qint64 m_numBytesTotal;
	QFile* m_filePointer;
};