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

#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTableWidget>
#include <QProgressBar>
#include <QLabel>
#include "partdownloader.h"
#include "partdownloadrequest.h"
#include "mainwindow.h"
#include "documentmanager.h"

PartDownloadRequest::PartDownloadRequest (QString url, QString dest, bool primary, PartDownloader* parent) :
	QObject (parent),
	HierarchyElement (parent),
	m_state (State::Requesting),
	m_prompt (parent),
	m_url (url),
	m_destination (dest),
	m_filePath (parent->downloadPath() + DIRSLASH + dest),
	m_networkManager (new QNetworkAccessManager),
	m_isFirstUpdate (true),
	m_isPrimary (primary),
	m_filePointer (nullptr)
{
	// Make sure that we have a valid destination.
	QString dirpath = Dirname (filePath());
	QDir dir (dirpath);

	if (not dir.exists())
	{
		print ("Creating %1...\n", dirpath);

		if (not dir.mkpath (dirpath))
			Critical (format (tr ("Couldn't create the directory %1!"), dirpath));
	}

	m_networkReply = m_networkManager->get (QNetworkRequest (QUrl (url)));
	connect (networkReply(), SIGNAL (finished()), this, SLOT (downloadFinished()));
	connect (networkReply(), SIGNAL (readyRead()), this, SLOT (readFromNetworkReply()));
	connect (networkReply(), SIGNAL (downloadProgress (qint64, qint64)),
		this, SLOT (updateDownloadProgress (qint64, qint64)));
}

PartDownloadRequest::~PartDownloadRequest() {}

bool PartDownloadRequest::failed() const
{
	return m_state == State::Failed;
}

int PartDownloadRequest::tableRow() const
{
	return m_tableRow;
}

void PartDownloadRequest::setTableRow (int value)
{
	m_tableRow = value;
}

PartDownloader* PartDownloadRequest::prompt() const
{
	return m_prompt;
}

QString PartDownloadRequest::url() const
{
	return m_url;
}

QString PartDownloadRequest::destination() const
{
	return m_destination;
}

QString PartDownloadRequest::filePath() const
{
	return m_filePath;
}

bool PartDownloadRequest::isFirstUpdate() const
{
	return m_isFirstUpdate;
}

qint64 PartDownloadRequest::numBytesRead() const
{
	return m_numBytesRead;
}

qint64 PartDownloadRequest::numBytesTotal() const
{
	return m_numBytesTotal;
}

bool PartDownloadRequest::isPrimary() const
{
	return m_isPrimary;
}

QNetworkReply* PartDownloadRequest::networkReply() const
{
	return m_networkReply;
}

void PartDownloadRequest::updateToTable()
{
	QTableWidget* table = prompt()->progressTable();

	switch (m_state)
	{
	case State::Requesting:
	case State::Downloading:
		{
			QWidget* cellwidget = table->cellWidget (tableRow(), PartDownloader::ProgressColumn);
			QProgressBar* progressBar = qobject_cast<QProgressBar*> (cellwidget);

			if (not progressBar)
			{
				progressBar = new QProgressBar;
				table->setCellWidget (tableRow(), PartDownloader::ProgressColumn, progressBar);
			}

			progressBar->setRange (0, numBytesTotal());
			progressBar->setValue (numBytesRead());
		}
		break;

	case State::Finished:
	case State::Failed:
		{
			const QString text = (m_state == State::Finished)
				? "<b><span style=\"color: #080\">FINISHED</span></b>"
				: "<b><span style=\"color: #800\">FAILED</span></b>";

			QLabel* lb = new QLabel (text);
			lb->setAlignment (Qt::AlignCenter);
			table->setCellWidget (tableRow(), PartDownloader::ProgressColumn, lb);
		}
		break;
	}

	QLabel* label = qobject_cast<QLabel*> (table->cellWidget (tableRow(), PartDownloader::PartLabelColumn));

	if (isFirstUpdate())
	{
		label = new QLabel (format ("<b>%1</b>", destination()), table);
		table->setCellWidget (tableRow(), PartDownloader::PartLabelColumn, label);
	}

	// Make sure that the cell is big enough to contain the label
	if (table->columnWidth (PartDownloader::PartLabelColumn) < label->width())
		table->setColumnWidth (PartDownloader::PartLabelColumn, label->width());

	m_isFirstUpdate = false;
}

void PartDownloadRequest::downloadFinished()
{
	if (networkReply()->error() != QNetworkReply::NoError)
	{
		if (isPrimary() and not prompt()->isAborted())
			Critical (networkReply()->errorString());

		print ("Unable to download %1: %2\n", destination(), networkReply()->errorString());
		m_state = State::Failed;
	}
	else if (m_state != State::Failed)
	{
		m_state = State::Finished;
	}

	m_numBytesRead = numBytesTotal();
	updateToTable();

	if (m_filePointer)
	{
		m_filePointer->close();
		delete m_filePointer;
		m_filePointer = nullptr;

		if (m_state == State::Failed)
			QFile::remove (filePath());
	}

	if (m_state != State::Finished)
	{
		prompt()->checkIfFinished();
		return;
	}

	// Try to load this file now.
	LDDocument* document = m_documents->openDocument (filePath(), false, not isPrimary());

	if (document == nullptr)
		return;

	// Iterate through this file and check for errors. If there's any that stems
	// from unknown file references, try resolve that by downloading the reference.
	// This is why downloading a part may end up downloading multiple files, as
	// it resolves dependencies.
	for (LDObject* obj : document->objects())
	{
		LDError* err = dynamic_cast<LDError*> (obj);

		if (err == nullptr or err->fileReferenced().isEmpty())
			continue;

		QString dest = err->fileReferenced();
		prompt()->downloadFromPartsTracker (dest);
	}

	prompt()->addFile (document);

	if (isPrimary())
	{
		m_documents->addRecentFile (filePath());
		prompt()->setPrimaryFile (document);
	}

	prompt()->checkIfFinished();
}

void PartDownloadRequest::updateDownloadProgress (int64 recv, int64 total)
{
	m_numBytesRead = recv;
	m_numBytesTotal = total;
	m_state = State::Downloading;
	updateToTable();
}

void PartDownloadRequest::readFromNetworkReply()
{
	if (m_state == State::Failed)
		return;

	if (m_filePointer == nullptr)
	{
		m_filePath.replace ("\\", "/");

		// We have already asked the user whether we can overwrite so we're good to go here.
		m_filePointer = new QFile (filePath().toLocal8Bit());

		if (not m_filePointer->open (QIODevice::WriteOnly))
		{
			Critical (format (tr ("Couldn't open %1 for writing: %2"), filePath(), strerror (errno)));
			m_state = State::Failed;
			networkReply()->abort();
			updateToTable();
			prompt()->checkIfFinished();
			return;
		}
	}

	m_filePointer->write (networkReply()->readAll());
}

bool PartDownloadRequest::isFinished() const
{
	return isOneOf (m_state, State::Finished, State::Failed);
}

void PartDownloadRequest::abort()
{
	networkReply()->abort();
}