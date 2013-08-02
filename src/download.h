/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 Santeri Piippo
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

#ifndef LDFORGE_DOWNLOAD_H
#define LDFORGE_DOWNLOAD_H

#include <QDialog>
#include "common.h"
#include "types.h"

class Ui_DownloadFrom;
class QNetworkAccessManager;
class QNetworkRequest;
class QNetworkReply;

// =============================================================================
// -----------------------------------------------------------------------------
extern class PartDownloader {
public:
	constexpr static const char* k_OfficialURL = "http://ldraw.org/library/official/",
		*k_UnofficialURL = "http://ldraw.org/library/unofficial/";
	
	PartDownloader() {}
	void download();
	void operator()() { download(); }
} g_PartDownloader;

// =============================================================================
// -----------------------------------------------------------------------------
class PartDownloadPrompt : public QDialog {
	Q_OBJECT
	
public:
	enum Source {
		OfficialLibrary,
		PartsTracker,
		CustomURL,
	};
	
	explicit PartDownloadPrompt (QWidget* parent = null);
	virtual ~PartDownloadPrompt();
	str getURL() const;
	str fullFilePath() const;
	str getDest() const;
	Source getSource() const;
	
public slots:
	void sourceChanged (int i);
	void startDownload();
	
protected:
	Ui_DownloadFrom* ui;
	friend class PartDownloadRequest;
};

// =============================================================================
// -----------------------------------------------------------------------------
class PartDownloadRequest : public QObject {
	Q_OBJECT
	PROPERTY (int, tableRow, setTableRow)
	
public:
	enum TableColumn {
		PartLabelColumn,
		ProgressColumn,
	};
	
	enum State {
		Requesting,
		Downloading,
		Finished,
		Aborted,
	};
	
	explicit PartDownloadRequest (str url, str dest, PartDownloadPrompt* parent);
	         PartDownloadRequest (const PartDownloadRequest&) = delete;
	virtual ~PartDownloadRequest();
	void updateToTable();
	
	void operator= (const PartDownloadRequest&) = delete;
	
public slots:
	void downloadFinished();
	void readyRead();
	void downloadProgress (qint64 recv, qint64 total);
	
private:
	PartDownloadPrompt* m_prompt;
	str m_url, m_dest, m_fpath;
	QNetworkAccessManager* m_nam;
	QNetworkReply* m_reply;
	bool m_firstUpdate;
	State m_state;
	int64 m_bytesRead, m_bytesTotal;
};

#endif // LDFORGE_DOWNLOAD_H