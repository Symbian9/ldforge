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

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDir>
#include <QProgressBar>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include "partDownloader.h"
#include "ui_downloadfrom.h"
#include "basics.h"
#include "mainwindow.h"
#include "ldDocument.h"
#include "glRenderer.h"

ConfigOption (QString DownloadFilePath)
ConfigOption (bool GuessDownloadPaths = true)
ConfigOption (bool AutoCloseDownloadDialog = true)

const char* g_unofficialLibraryURL = "http://ldraw.org/library/unofficial/";

// =============================================================================
//
void PartDownloader::staticBegin()
{
	PartDownloader dlg (g_win);

	if (not dlg.checkValidPath())
		return;

	dlg.exec();
}

// =============================================================================
//
QString PartDownloader::getDownloadPath()
{
	QString path = m_config->downloadFilePath();

	if (DIRSLASH[0] != '/')
		path.replace (DIRSLASH, "/");

	return path;
}

// =============================================================================
//
PartDownloader::PartDownloader (QWidget* parent) :
	QDialog (parent),
	HierarchyElement (parent),
	m_source (Source (0))
{
	setForm (new Ui_DownloadFrom);
	form()->setupUi (this);
	form()->fname->setFocus();

#ifdef USE_QT5
	form()->progress->horizontalHeader()->setSectionResizeMode (QHeaderView::Stretch);
#else
	form()->progress->horizontalHeader()->setResizeMode (PartLabelColumn, QHeaderView::Stretch);
#endif

	setDownloadButton (new QPushButton (tr ("Download")));
	form()->buttonBox->addButton (downloadButton(), QDialogButtonBox::ActionRole);
	getButton (Abort)->setEnabled (false);

	connect (form()->source, SIGNAL (currentIndexChanged (int)),
		this, SLOT (sourceChanged (int)));
	connect (form()->buttonBox, SIGNAL (clicked (QAbstractButton*)),
		this, SLOT (buttonClicked (QAbstractButton*)));
}

// =============================================================================
//
PartDownloader::~PartDownloader()
{
	delete form();
}

// =============================================================================
//
bool PartDownloader::checkValidPath()
{
	QString path = getDownloadPath();

	if (path.isEmpty() or not QDir (path).exists())
	{
		QMessageBox::information(this, "Notice", "Please input a path for files to download.");
		path = QFileDialog::getExistingDirectory (this, "Path for downloaded files:");

		if (path.isEmpty())
			return false;

		m_config->setDownloadFilePath (path);
	}

	return true;
}

// =============================================================================
//
QString PartDownloader::getURL()
{
	const Source src = getSource();
	QString dest;

	switch (src)
	{
		case PartsTracker:
			dest = form()->fname->text();
			modifyDestination (dest);
			form()->fname->setText (dest);
			return g_unofficialLibraryURL + dest;

		case CustomURL:
			return form()->fname->text();
	}

	// Shouldn't happen
	return "";
}

// =============================================================================
//
void PartDownloader::modifyDestination (QString& dest) const
{
	dest = dest.simplified();

	// If the user doesn't want us to guess, stop right here.
	if (not m_config->guessDownloadPaths())
		return;

	// Ensure .dat extension
	if (dest.right (4) != ".dat")
	{
		// Remove the existing extension, if any. It may be we're here over a
		// typo in the .dat extension.
		const int dotpos = dest.lastIndexOf (".");

		if ((dotpos != -1) and (dotpos >= dest.length() - 4))
			dest.chop (dest.length() - dotpos);

		dest += ".dat";
	}

	// If the part starts with s\ or s/, then use parts/s/. Same goes with
	// 48\ and p/48/.
	if (isOneOf (dest.left (2), "s\\", "s/"))
	{
		dest.remove (0, 2);
		dest.prepend ("parts/s/");
	}
	else if (isOneOf (dest.left (3), "48\\", "48/"))
	{
		dest.remove (0, 3);
		dest.prepend ("p/48/");
	}

	/* Try determine where to put this part. We have four directories:
	   parts/, parts/s/, p/, and p/48/. If we haven't already specified
	   either parts/ or p/, we need to add it automatically. Part files
	   are numbers wit a possible u prefix for parts with unknown number
	   which can be followed by any of:
	   - c** (composites)
	   - d** (formed stickers)
	   - p** (patterns)
	   - a lowercase alphabetic letter for variants

	   Subfiles (usually) have an s** prefix, in which case we use parts/s/.
	   Note that the regex starts with a '^' so it won't catch already fully
	   given part file names. */
	QString partRegex = "^u?[0-9]+(c[0-9][0-9]+)*(d[0-9][0-9]+)*[a-z]?(p[0-9a-z][0-9a-z]+)*";
	QString subpartRegex = partRegex + "s[0-9][0-9]+";

	partRegex += "\\.dat$";
	subpartRegex += "\\.dat$";

	if (QRegExp (subpartRegex).exactMatch (dest))
		dest.prepend ("parts/s/");
	else if (QRegExp (partRegex).exactMatch (dest))
		dest.prepend ("parts/");
	else if (not dest.startsWith ("parts/") and not dest.startsWith ("p/"))
		dest.prepend ("p/");
}

// =============================================================================
//
PartDownloader::Source PartDownloader::getSource() const
{
	return m_source;
}

// =============================================================================
//
void PartDownloader::setSource (Source src)
{
	m_source = src;
	form()->source->setCurrentIndex (int (src));
}

// =============================================================================
//
void PartDownloader::sourceChanged (int i)
{
	if (i == CustomURL)
		form()->fileNameLabel->setText (tr ("URL:"));
	else
		form()->fileNameLabel->setText (tr ("File name:"));

	m_source = Source (i);
}

// =============================================================================
//
void PartDownloader::buttonClicked (QAbstractButton* btn)
{
	if (btn == getButton (Close))
	{
		reject();
	}
	else if (btn == getButton (Abort))
	{
		setAborted (true);

		for (PartDownloadRequest* req : requests())
			req->abort();
	}
	else if (btn == getButton (Download))
	{
		QString dest = form()->fname->text();
		setPrimaryFile (nullptr);
		setAborted (false);

		if (getSource() == CustomURL)
			dest = Basename (getURL());

		modifyDestination (dest);

		if (QFile::exists (getDownloadPath() + DIRSLASH + dest))
		{
			const QString overwritemsg = format (tr ("%1 already exists in download directory. Overwrite?"), dest);
			if (not Confirm (tr ("Overwrite?"), overwritemsg))
				return;
		}

		downloadFile (dest, getURL(), true);
	}
}

// =============================================================================
//
void PartDownloader::downloadFile (QString dest, QString url, bool primary)
{
	int row = form()->progress->rowCount();

	// Don't download files repeadetly.
	if (filesToDownload().indexOf (dest) != -1)
		return;

	print ("Downloading %1 from %2\n", dest, url);
	modifyDestination (dest);
	PartDownloadRequest* req = new PartDownloadRequest (url, dest, primary, this);
	m_filesToDownload << dest;
	m_requests << req;
	form()->progress->insertRow (row);
	req->setTableRow (row);
	req->updateToTable();
	downloadButton()->setEnabled (false);
	form()->progress->setEnabled (true);
	form()->fname->setEnabled (false);
	form()->source->setEnabled (false);
	getButton (Close)->setEnabled (false);
	getButton (Abort)->setEnabled (true);
	getButton (Download)->setEnabled (false);
}

// =============================================================================
//
void PartDownloader::downloadFromPartsTracker (QString file)
{
	modifyDestination (file);
	downloadFile (file, g_unofficialLibraryURL + file, false);
}

// =============================================================================
//
void PartDownloader::checkIfFinished()
{
	bool failed = isAborted();

	// If there is some download still working, we're not finished.
	for (PartDownloadRequest* req : requests())
	{
		if (not req->isFinished())
			return;

		if (req->failed())
			failed = true;
	}

	for (PartDownloadRequest* req : requests())
		delete req;

	m_requests.clear();

	// Update everything now
	if (primaryFile())
	{
		g_win->changeDocument (primaryFile());
		g_win->doFullRefresh();
		g_win->renderer()->resetAngles();
	}

	for (LDDocument* f : m_files)
		f->reloadAllSubfiles();

	if (m_config->autoCloseDownloadDialog() and not failed)
	{
		// Close automatically if desired.
		accept();
	}
	else
	{
		// Allow the prompt be closed now.
		getButton (Abort)->setEnabled (false);
		getButton (Close)->setEnabled (true);
	}
}

// =============================================================================
//
QPushButton* PartDownloader::getButton (PartDownloader::Button i)
{
	switch (i)
	{
		case Download:
			return downloadButton();

		case Abort:
			return qobject_cast<QPushButton*> (form()->buttonBox->button (QDialogButtonBox::Abort));

		case Close:
			return qobject_cast<QPushButton*> (form()->buttonBox->button (QDialogButtonBox::Close));
	}

	return nullptr;
}

void PartDownloader::addFile (LDDocument* f)
{
	m_files << f;
}

//
// ---------------------------------------------------------------------------------------------------------------------
//

PartDownloadRequest::PartDownloadRequest (QString url, QString dest, bool primary, PartDownloader* parent) :
	QObject (parent),
	m_state (State::Requesting),
	m_prompt (parent),
	m_url (url),
	m_destination (dest),
	m_filePath (parent->getDownloadPath() + DIRSLASH + dest),
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
	connect (networkReply(), SIGNAL (readyRead()), this, SLOT (readyRead()));
	connect (networkReply(), SIGNAL (downloadProgress (qint64, qint64)),
		this, SLOT (downloadProgress (qint64, qint64)));
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
	QTableWidget* table = prompt()->form()->progress;

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
	LDDocument* f = OpenDocument (filePath(), false, not isPrimary());

	if (f == nullptr)
		return;

	// Iterate through this file and check for errors. If there's any that stems
	// from unknown file references, try resolve that by downloading the reference.
	// This is why downloading a part may end up downloading multiple files, as
	// it resolves dependencies.
	for (LDObject* obj : f->objects())
	{
		LDError* err = dynamic_cast<LDError*> (obj);

		if (err == nullptr or err->fileReferenced().isEmpty())
			continue;

		QString dest = err->fileReferenced();
		prompt()->downloadFromPartsTracker (dest);
	}

	prompt()->addFile (f);

	if (isPrimary())
	{
		AddRecentFile (filePath());
		prompt()->setPrimaryFile (f);
	}

	prompt()->checkIfFinished();
}

void PartDownloadRequest::downloadProgress (int64 recv, int64 total)
{
	m_numBytesRead = recv;
	m_numBytesTotal = total;
	m_state = State::Downloading;
	updateToTable();
}

void PartDownloadRequest::readyRead()
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
