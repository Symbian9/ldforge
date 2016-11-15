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
#include "main.h"
#include "basics.h"
#include "ldDocument.h"

class LDDocument;
class QFile;
class PartDownloadRequest;
class Ui_PartDownloader;
class QNetworkAccessManager;
class QNetworkRequest;
class QNetworkReply;
class QAbstractButton;
class QTableWidget;

class PartDownloader : public QDialog, public HierarchyElement
{
	Q_OBJECT

public:
	enum SourceType
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

	explicit PartDownloader(QWidget* parent = nullptr);
	virtual ~PartDownloader();

	void addFile(LDDocument* file);
	QAbstractButton *button(Button which);
	void checkValidPath();
	void downloadFile (QString dest, QString url, bool isPrimary);
	void downloadFromPartsTracker (QString file);
	QString downloadPath();
	bool isAborted() const;
	void modifyDestination (QString& dest) const;
	LDDocument* primaryFile() const;
	QTableWidget* progressTable() const;
	void setPrimaryFile (LDDocument* document);
	void setSourceType (SourceType src);
	SourceType sourceType() const;
	QString url();

public slots:
	void buttonClicked (QAbstractButton* btn);
	void checkIfFinished();
	void sourceChanged (int i);


signals:
	void primaryFileDownloaded();

private:
	Ui_PartDownloader& ui;
	QStringList m_filesToDownload;
	QVector<PartDownloadRequest*> m_requests;
	QPushButton* m_downloadButton;
	SourceType m_source;
	QVector<LDDocument*> m_files;
	LDDocument* m_primaryFile;
	bool m_isAborted;
};
