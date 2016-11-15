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

ConfigOption(QString DownloadFilePath)
ConfigOption(bool GuessDownloadPaths = true)
ConfigOption (bool AutoCloseDownloadDialog = true)

const char* g_unofficialLibraryURL = "http://ldraw.org/library/unofficial/";

PartDownloader::PartDownloader (QWidget* parent)
    : QDialog(parent)
    , HierarchyElement(parent)
    , ui(*new Ui_PartDownloader)
    , m_source(SourceType{})
{
	ui.setupUi(this);

#ifdef USE_QT5
	ui.progressTable->horizontalHeader()->setSectionResizeMode (QHeaderView::Stretch);
#else
	ui.progressTable->horizontalHeader()->setResizeMode (PartLabelColumn, QHeaderView::Stretch);
#endif

	m_downloadButton = new QPushButton {tr("Download")};
	ui.buttonBox->addButton(m_downloadButton, QDialogButtonBox::ActionRole);
	button(Abort)->setEnabled(false);
	connect(ui.source, SIGNAL(currentIndexChanged(int)), this, SLOT(sourceChanged(int)));
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonClicked(QAbstractButton*)));
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
		path = QFileDialog::getExistingDirectory(this, "Path for downloaded files:");

		if (path.isEmpty())
			reject();
		else
			m_config->setDownloadFilePath(path);
	}
}

QString PartDownloader::url()
{
	switch (sourceType())
	{
	case PartsTracker:
		{
			QString destination;
			destination = ui.filename->text();
			modifyDestination(destination);
			ui.filename->setText(destination);
			return g_unofficialLibraryURL + destination;
		}

	case CustomURL:
		return ui.filename->text();
	}

	return "";
}

void PartDownloader::modifyDestination(QString& destination) const
{
	destination = destination.simplified();

	// If the user doesn't want us to guess, stop right here.
	if (not m_config->guessDownloadPaths())
		return;

	// Ensure .dat extension
	if (destination.right (4) != ".dat")
	{
		// Remove the existing extension, if any. It may be we're here over a typo in the .dat extension.
		int dotPosition = destination.lastIndexOf(".");

		if ((dotPosition != -1) and (dotPosition >= destination.length() - 4))
			destination.chop (destination.length() - dotPosition);

		destination += ".dat";
	}

	// If the part starts with s\ or s/, then use parts/s/. Same goes with 48\ and p/48/.
	if (isOneOf(destination.left(2), "s\\", "s/"))
	{
		destination.remove(0, 2);
		destination.prepend("parts/s/");
	}
	else if (isOneOf(destination.left(3), "48\\", "48/"))
	{
		destination.remove(0, 3);
		destination.prepend("p/48/");
	}

	/* Try determine where to put this part. We have four directories:
	 * parts/, parts/s/, p/, and p/48/. If we haven't already specified
	 * either parts/ or p/, we need to add it automatically. Part files
	 * are numbers wit a possible u prefix for parts with unknown number
	 * which can be followed by any of:
	 * - c** (composites)
	 * - d** (formed stickers)
	 * - p** (patterns)
	 * - a lowercase alphabetic letter for variants
	 *
	 * Subfiles (usually) have an s** prefix, in which case we use parts/s/.
	 * Note that the regex starts with a '^' so it won't catch already fully
	 * given part file names.
	 */
	QString partRegex = "^u?[0-9]+(c[0-9][0-9]+)*(d[0-9][0-9]+)*[a-z]?(p[0-9a-z][0-9a-z]+)*";
	QString subpartRegex = partRegex + "s[0-9][0-9]+";

	partRegex += "\\.dat$";
	subpartRegex += "\\.dat$";

	if (QRegExp {subpartRegex}.exactMatch(destination))
		destination.prepend("parts/s/");
	else if (QRegExp {partRegex}.exactMatch(destination))
		destination.prepend("parts/");
	else if (not destination.startsWith("parts/") and not destination.startsWith("p/"))
		destination.prepend("p/");
}

PartDownloader::SourceType PartDownloader::sourceType() const
{
	return m_source;
}

void PartDownloader::setSourceType(SourceType src)
{
	m_source = src;
	ui.source->setCurrentIndex(int {src});
}

void PartDownloader::sourceChanged(int sourceType)
{
	if (sourceType == CustomURL)
		ui.fileNameLabel->setText(tr("URL:"));
	else
		ui.fileNameLabel->setText(tr("File name:"));

	m_source = static_cast<SourceType>(sourceType);
}

void PartDownloader::buttonClicked(QAbstractButton* button)
{
	if (button == this->button(Close))
	{
		reject();
	}
	else if (button == this->button(Abort))
	{
		m_isAborted = true;

		for (PartDownloadRequest* request : m_requests)
			request->abort();
	}
	else if (button == this->button(Download))
	{
		QString destination = ui.filename->text();
		setPrimaryFile (nullptr);
		m_isAborted = false;

		if (sourceType() == CustomURL)
			destination = Basename(url());

		modifyDestination(destination);

		if (QFile::exists(downloadPath() + DIRSLASH + destination))
		{
			QString message = format(tr("%1 already exists in download directory. Overwrite?"), destination);
			if (not Confirm(tr("Overwrite?"), message))
				return;
		}

		downloadFile (destination, url(), true);
	}
}

void PartDownloader::downloadFile(QString destination, QString url, bool isPrimary)
{
	int row = ui.progressTable->rowCount();

	// Don't download files repeadetly.
	if (not m_filesToDownload.contains(destination))
	{
		print ("Downloading %1 from %2\n", destination, url);
		modifyDestination (destination);
		PartDownloadRequest* request = new PartDownloadRequest {url, destination, isPrimary, this};
		m_filesToDownload << destination;
		m_requests << request;
		ui.progressTable->insertRow(row);
		request->setTableRow(row);
		request->updateToTable();
		m_downloadButton->setEnabled(false);
		ui.progressTable->setEnabled(true);
		ui.filename->setEnabled(false);
		ui.source->setEnabled(false);
		button(Close)->setEnabled(false);
		button(Abort)->setEnabled(true);
		button(Download)->setEnabled(false);
	}
}

void PartDownloader::downloadFromPartsTracker (QString file)
{
	modifyDestination(file);
	downloadFile(file, g_unofficialLibraryURL + file, false);
}

void PartDownloader::checkIfFinished()
{
	bool failed = isAborted();

	// If there is some download still working, we're not finished.
	for (PartDownloadRequest* request : m_requests)
	{
		if (not request->isFinished())
			return;

		failed |= request->failed();
	}

	for (PartDownloadRequest* request : m_requests)
		delete request;

	m_requests.clear();

	if (primaryFile())
		emit primaryFileDownloaded();

	for (LDDocument* file : m_files)
		file->reloadAllSubfiles();

	if (m_config->autoCloseDownloadDialog() and not failed)
	{
		// Close automatically if desired.
		accept();
	}
	else
	{
		// Allow the prompt be closed now.
		button(Abort)->setEnabled(false);
		button(Close)->setEnabled(true);
	}
}

QAbstractButton* PartDownloader::button(PartDownloader::Button which)
{
	switch (which)
	{
	case Download:
		return m_downloadButton;

	case Abort:
		return ui.buttonBox->button(QDialogButtonBox::Abort);

	case Close:
		return ui.buttonBox->button(QDialogButtonBox::Close);

	default:
		return nullptr;
	}
}

void PartDownloader::addFile(LDDocument* file)
{
	m_files.append(file);
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
		path.replace(DIRSLASH, "/");

	return path;
}

QTableWidget* PartDownloader::progressTable() const
{
	return ui.progressTable;
}
