/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Teemu Piippo
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
class PartDownloader : public QDialog
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
	PROPERTY (public,	LDDocumentPtr, 		primaryFile,		setPrimaryFile,		STOCK_WRITE)
	PROPERTY (public,	bool,				isAborted,			setAborted,			STOCK_WRITE)
	PROPERTY (private,	Ui_DownloadFrom*,	form,				setForm,			STOCK_WRITE)
	PROPERTY (private,	QStringList,		filesToDownload,	setFilesToDownload,	STOCK_WRITE)
	PROPERTY (private,	RequestList,		requests,			setRequests,		STOCK_WRITE)
	PROPERTY (private,	QPushButton*,		downloadButton,		setDownloadButton,	STOCK_WRITE)

public:
	explicit		PartDownloader (QWidget* parent = null);
	virtual			~PartDownloader();

	void			addFile (LDDocumentPtr f);
	bool			checkValidPath();
	void			downloadFile (QString dest, QString url, bool primary);
	void			downloadFromPartsTracker (QString file);
	QPushButton*	getButton (Button i);
	QString			getURL();
	Source			getSource() const;
	void			setSource (Source src);
	void			modifyDestination (QString& dest) const;

	static QString	getDownloadPath();
	static void		staticBegin();

public slots:
	void			buttonClicked (QAbstractButton* btn);
	void			checkIfFinished();
	void			sourceChanged (int i);

private:
	Source m_source;
	QList<LDDocumentPtr> m_files;
};

// =============================================================================
//
class PartDownloadRequest : public QObject
{
public:
    enum class State
    {
        Requesting,
        Downloading,
        Finished,
        Failed,
    };

	Q_OBJECT
	PROPERTY (public,	int,						tableRow,		setTableRow,		STOCK_WRITE)
    PROPERTY (private,	State,                      state,			setState,			STOCK_WRITE)
	PROPERTY (private,	PartDownloader*,			prompt,			setPrompt,			STOCK_WRITE)
	PROPERTY (private,	QString,					url,			setURL,				STOCK_WRITE)
	PROPERTY (private,	QString,					destinaton,		setDestination,		STOCK_WRITE)
	PROPERTY (private,	QString,					filePath,		setFilePath,		STOCK_WRITE)
	PROPERTY (private,	QNetworkAccessManager*,		networkManager,	setNetworkManager,	STOCK_WRITE)
	PROPERTY (private,	QNetworkReply*,				networkReply,	setNetworkReply,	STOCK_WRITE)
	PROPERTY (private,	bool,						isFirstUpdate,	setFirstUpdate,		STOCK_WRITE)
	PROPERTY (private,	int64,						numBytesRead,	setNumBytesRead,	STOCK_WRITE)
	PROPERTY (private,	int64,						numBytesTotal,	setNumBytesTotal,	STOCK_WRITE)
	PROPERTY (private,	bool,						isPrimary,		setPrimary,			STOCK_WRITE)
	PROPERTY (private,	QFile*,						filePointer,	setFilePointer,		STOCK_WRITE)

public:
	explicit PartDownloadRequest (QString url, QString dest, bool primary, PartDownloader* parent);
	PartDownloadRequest (const PartDownloadRequest&) = delete;
	virtual ~PartDownloadRequest();
	void updateToTable();
	bool isFinished() const;
	void operator= (const PartDownloadRequest&) = delete;

public slots:
	void downloadFinished();
	void readyRead();
	void downloadProgress (qint64 recv, qint64 total);
	void abort();
};
