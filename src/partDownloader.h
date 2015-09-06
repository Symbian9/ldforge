/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2015 Teemu Piippo
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
#include "main.h"
#include "basics.h"
#include "ldDocument.h"

class LDDocument;
class QFile;
class PartDownloadRequest;
class Ui_DownloadFrom;
class QNetworkAccessManager;
class QNetworkRequest;
class QNetworkReply;
class QAbstractButton;

// =============================================================================
//
class PartDownloader : public QDialog, public HierarchyElement
{
public:
	enum Source
	{
		PartsTracker,
		CustomURL,
	};

	enum Button
	{
		Download,
		Abort,
		Close
	};

	enum TableColumn
	{
		PartLabelColumn,
		ProgressColumn,
	};

	using RequestList = QList<PartDownloadRequest*>;

	Q_OBJECT
	PROPERTY (public,	LDDocument*, 		primaryFile,		setPrimaryFile,		STOCK_WRITE)
	PROPERTY (public,	bool,				isAborted,			setAborted,			STOCK_WRITE)
	PROPERTY (private,	Ui_DownloadFrom*,	form,				setForm,			STOCK_WRITE)
	PROPERTY (private,	QStringList,		filesToDownload,	setFilesToDownload,	STOCK_WRITE)
	PROPERTY (private,	RequestList,		requests,			setRequests,		STOCK_WRITE)
	PROPERTY (private,	QPushButton*,		downloadButton,		setDownloadButton,	STOCK_WRITE)

public:
	explicit		PartDownloader (QWidget* parent = nullptr);
	virtual			~PartDownloader();

	void			addFile (LDDocument* f);
	bool			checkValidPath();
	void			downloadFile (QString dest, QString url, bool primary);
	void			downloadFromPartsTracker (QString file);
	QPushButton*	getButton (Button i);
	QString			getURL();
	Source			getSource() const;
	void			setSource (Source src);
	void			modifyDestination (QString& dest) const;

	QString			getDownloadPath();
	static void		staticBegin();

public slots:
	void			buttonClicked (QAbstractButton* btn);
	void			checkIfFinished();
	void			sourceChanged (int i);

private:
	Source m_source;
	QList<LDDocument*> m_files;
};

class PartDownloadRequest : public QObject
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

	QString destination() const;
	bool failed() const;
	QString filePath() const;
	bool isFinished() const;
	bool isFirstUpdate() const;
	bool isPrimary() const;
	QNetworkReply* networkReply() const;
	qint64 numBytesRead() const;
	qint64 numBytesTotal() const;
	PartDownloader* prompt() const;
	void setTableRow (int value);
	int tableRow() const;
	void updateToTable();
	QString url() const;

public slots:
	void abort();
	void downloadFinished();
	void downloadProgress (qint64 recv, qint64 total);
	void readyRead();

private:
	int m_tableRow;
	State m_state;
	PartDownloader* m_prompt;
	QString m_url;
	QString m_destination;
	QString m_filePath;
	QNetworkAccessManager* m_networkManager;
	QNetworkReply* m_networkReply;
	bool m_isFirstUpdate;
	bool m_isPrimary;
	qint64 m_numBytesRead;
	qint64 m_numBytesTotal;
	QFile* m_filePointer;
};
