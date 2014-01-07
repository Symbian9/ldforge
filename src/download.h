/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Santeri Piippo
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
#include "main.h"
#include "types.h"

class LDDocument;
class QFile;
class PartDownloadRequest;
class Ui_DownloadFrom;
class QNetworkAccessManager;
class QNetworkRequest;
class QNetworkReply;
class QAbstractButton;

// =============================================================================
// -----------------------------------------------------------------------------
class PartDownloader : public QDialog
{
	typedefs:
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

	properties:
		Q_OBJECT
		PROPERTY (public,	LDDocument*, 		PrimaryFile,		NO_OPS,		STOCK_WRITE)
		PROPERTY (public,	bool,				Aborted,			BOOL_OPS,	STOCK_WRITE)
		PROPERTY (private,	Ui_DownloadFrom*,	Interface,			NO_OPS,		STOCK_WRITE)
		PROPERTY (private,	QStringList,			FilesToDownload,		LIST_OPS,	STOCK_WRITE)
		PROPERTY (private,	RequestList,		Requests,			LIST_OPS,	STOCK_WRITE)
		PROPERTY (private,	QPushButton*,		DownloadButton,		NO_OPS,		STOCK_WRITE)

	public:
		explicit		PartDownloader (QWidget* parent = null);
		virtual			~PartDownloader();

		void			downloadFile (str dest, str url, bool primary);
		QPushButton*		getButton (Button i);
		str				getURL() const;
		Source			getSource() const;
		void			modifyDestination (str& dest) const;

		static str		getDownloadPath();
		static void		staticBegin();

	public slots:
		void			buttonClicked (QAbstractButton* btn);
		void			checkIfFinished();
		void			sourceChanged (int i);
};

// =============================================================================
// -----------------------------------------------------------------------------
class PartDownloadRequest : public QObject
{
	typedefs:
		enum EState
		{
			ERequesting,
			EDownloading,
			EFinished,
			EFailed,
		};

	properties:
		Q_OBJECT
		PROPERTY (public,	int,						TableRow,		NUM_OPS,	STOCK_WRITE)
		PROPERTY (private,	EState,					State,			NO_OPS,		STOCK_WRITE)
		PROPERTY (private,	PartDownloader*,			Prompt,			NO_OPS,		STOCK_WRITE)
		PROPERTY (private,	str,					URL,			STR_OPS,	STOCK_WRITE)
		PROPERTY (private,	str,					Destinaton,		STR_OPS,	STOCK_WRITE)
		PROPERTY (private,	str,					FilePath,		STR_OPS,	STOCK_WRITE)
		PROPERTY (private,	QNetworkAccessManager*,	NAM,			NO_OPS,		STOCK_WRITE)
		PROPERTY (private,	QNetworkReply*,			Reply,			NO_OPS,		STOCK_WRITE)
		PROPERTY (private,	bool,					FirstUpdate,	BOOL_OPS,	STOCK_WRITE)
		PROPERTY (private,	int64,					BytesRead,		NUM_OPS,	STOCK_WRITE)
		PROPERTY (private,	int64,					BytesTotal,		NUM_OPS,	STOCK_WRITE)
		PROPERTY (private,	bool,					Primary,		BOOL_OPS,	STOCK_WRITE)
		PROPERTY (private,	QFile*,					FilePointer,	NO_OPS,		STOCK_WRITE)

	public:
		explicit PartDownloadRequest (str url, str dest, bool primary, PartDownloader* parent);
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

#endif // LDFORGE_DOWNLOAD_H
