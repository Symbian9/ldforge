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
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QTableWidget>
#include "partdownloader.h"
#include "partdownloadrequest.h"
#include "ui_partdownloader.h"
#include "basics.h"
#include "mainwindow.h"
#include "ldDocument.h"
#include "glRenderer.h"
#include "documentmanager.h"

ConfigOption (QString DownloadFilePath)
ConfigOption (bool GuessDownloadPaths = true)
ConfigOption (bool AutoCloseDownloadDialog = true)

const char* g_unofficialLibraryURL = "http://ldraw.org/library/unofficial/";

PartDownloader::PartDownloader (QWidget* parent) :
	QDialog (parent),
	HierarchyElement (parent),
	ui (*new Ui_PartDownloader),
	m_source (SourceType (0))
{
	ui.setupUi (this);

#ifdef USE_QT5
	ui.progressTable->horizontalHeader()->setSectionResizeMode (QHeaderView::Stretch);
#else
	ui.progressTable->horizontalHeader()->setResizeMode (PartLabelColumn, QHeaderView::Stretch);
#endif

	m_downloadButton = new QPushButton (tr ("Download"));
	ui.buttonBox->addButton (m_downloadButton, QDialogButtonBox::ActionRole);
	button (Abort)->setEnabled (false);
	connect (ui.source, SIGNAL (currentIndexChanged (int)), this, SLOT (sourceChanged (int)));
	connect (ui.buttonBox, SIGNAL (clicked (QAbstractButton*)), this, SLOT (buttonClicked (QAbstractButton*)));
}

PartDownloader::~PartDownloader()
{
	delete &ui;
}

void PartDownloader::checkValidPath()
{
	QString path = downloadPath();

	if (path.isEmpty() or not QDir (path).exists())
	{
		QMessageBox::information(this, "Notice", "Please input a path for files to download.");
		path = QFileDialog::getExistingDirectory (this, "Path for downloaded files:");

		if (path.isEmpty())
			reject();
		else
			m_config->setDownloadFilePath (path);
	}
}

QString PartDownloader::url()
{
	QString destination;

	switch (sourceType())
	{
	case PartsTracker:
		destination = ui.filename->text();
		modifyDestination (destination);
		ui.filename->setText (destination);
		return g_unofficialLibraryURL + destination;

	case CustomURL:
		return ui.filename->text();
	}

	// Shouldn't happen
	return "";
}

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

PartDownloader::SourceType PartDownloader::sourceType() const
{
	return m_source;
}

void PartDownloader::setSourceType (SourceType src)
{
	m_source = src;
	ui.source->setCurrentIndex (int (src));
}

void PartDownloader::sourceChanged (int i)
{
	if (i == CustomURL)
		ui.fileNameLabel->setText (tr ("URL:"));
	else
		ui.fileNameLabel->setText (tr ("File name:"));

	m_source = SourceType (i);
}

void PartDownloader::buttonClicked (QAbstractButton* btn)
{
	if (btn == button (Close))
	{
		reject();
	}
	else if (btn == button (Abort))
	{
		m_isAborted = true;

		for (PartDownloadRequest* req : m_requests)
			req->abort();
	}
	else if (btn == button (Download))
	{
		QString dest = ui.filename->text();
		setPrimaryFile (nullptr);
		m_isAborted = false;

		if (sourceType() == CustomURL)
			dest = Basename (url());

		modifyDestination (dest);

		if (QFile::exists (downloadPath() + DIRSLASH + dest))
		{
			const QString overwritemsg = format (tr ("%1 already exists in download directory. Overwrite?"), dest);
			if (not Confirm (tr ("Overwrite?"), overwritemsg))
				return;
		}

		downloadFile (dest, url(), true);
	}
}

void PartDownloader::downloadFile (QString dest, QString url, bool primary)
{
	int row = ui.progressTable->rowCount();

	// Don't download files repeadetly.
	if (m_filesToDownload.indexOf (dest) != -1)
		return;

	print ("Downloading %1 from %2\n", dest, url);
	modifyDestination (dest);
	PartDownloadRequest* req = new PartDownloadRequest (url, dest, primary, this);
	m_filesToDownload << dest;
	m_requests << req;
	ui.progressTable->insertRow (row);
	req->setTableRow (row);
	req->updateToTable();
	m_downloadButton->setEnabled (false);
	ui.progressTable->setEnabled (true);
	ui.filename->setEnabled (false);
	ui.source->setEnabled (false);
	button (Close)->setEnabled (false);
	button (Abort)->setEnabled (true);
	button (Download)->setEnabled (false);
}

void PartDownloader::downloadFromPartsTracker (QString file)
{
	modifyDestination (file);
	downloadFile (file, g_unofficialLibraryURL + file, false);
}

void PartDownloader::checkIfFinished()
{
	bool failed = isAborted();

	// If there is some download still working, we're not finished.
	for (PartDownloadRequest* req : m_requests)
	{
		if (not req->isFinished())
			return;

		if (req->failed())
			failed = true;
	}

	for (PartDownloadRequest* req : m_requests)
		delete req;

	m_requests.clear();

	if (primaryFile())
		emit primaryFileDownloaded();

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
		button (Abort)->setEnabled (false);
		button (Close)->setEnabled (true);
	}
}

QPushButton* PartDownloader::button (PartDownloader::Button i)
{
	switch (i)
	{
		case Download:
			return m_downloadButton;

		case Abort:
			return qobject_cast<QPushButton*> (ui.buttonBox->button (QDialogButtonBox::Abort));

		case Close:
			return qobject_cast<QPushButton*> (ui.buttonBox->button (QDialogButtonBox::Close));
	}

	return nullptr;
}

void PartDownloader::addFile (LDDocument* f)
{
	m_files << f;
}

bool PartDownloader::isAborted() const
{
	return m_isAborted;
}

LDDocument* PartDownloader::primaryFile() const
{
	return m_primaryFile;
}

void PartDownloader::setPrimaryFile (LDDocument* document)
{
	m_primaryFile = document;
}

QString PartDownloader::downloadPath()
{
	QString path = m_config->downloadFilePath();

	if (DIRSLASH[0] != '/')
		path.replace (DIRSLASH, "/");

	return path;
}

QTableWidget* PartDownloader::progressTable() const
{
	return ui.progressTable;
}
