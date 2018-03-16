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

#include <QFileDialog>
#include <QMessageBox>
#include "../canvas.h"
#include "../lddocument.h"
#include "../mainwindow.h"
#include "../partdownloader.h"
#include "../parser.h"
#include "../primitives.h"
#include "../dialogs/configdialog.h"
#include "../dialogs/ldrawpathdialog.h"
#include "../dialogs/newpartdialog.h"
#include "../dialogs/generateprimitivedialog.h"
#include "../documentmanager.h"
#include "filetoolset.h"
#include "ui_aboutdialog.h"

FileToolset::FileToolset(MainWindow* parent)
    : Toolset(parent) {}

void FileToolset::newPart()
{
	NewPartDialog* dialog = new NewPartDialog {m_window};

	if (dialog->exec() == QDialog::Accepted)
	{
		m_window->createBlankDocument();
		dialog->fillHeader(currentDocument());
		m_window->doFullRefresh();
	}
}

void FileToolset::newFile()
{
	m_window->createBlankDocument();
}

void FileToolset::open()
{
	QString name = QFileDialog::getOpenFileName(m_window, "Open File", "", "LDraw files (*.dat *.ldr)");

	if (not name.isEmpty())
		m_documents->openMainModel (name);
}

void FileToolset::save()
{
	m_window->save(currentDocument(), false);
}

void FileToolset::saveAs()
{
	m_window->save(currentDocument(), true);
}

void FileToolset::saveAll()
{
	for (LDDocument* document : m_documents->allDocuments())
		m_window->save(document, false);
}

void FileToolset::close()
{
	if (currentDocument()->isSafeToClose())
		currentDocument()->close();
}

void FileToolset::closeAll()
{
	if (m_documents->isSafeToCloseAll())
		m_documents->clear();
}

void FileToolset::settings()
{
	(new ConfigDialog {m_window})->exec();
}

void FileToolset::setLDrawPath()
{
	LDrawPathDialog* dialog = new LDrawPathDialog {m_config->lDrawPath(), true};

	if (dialog->exec())
		m_config->setLDrawPath (dialog->path());
}

void FileToolset::exit()
{
	::exit(EXIT_SUCCESS);
}

void FileToolset::insertFrom()
{
	QString filePath = QFileDialog::getOpenFileName();
	int position = m_window->suggestInsertPoint();

	if (not filePath.isEmpty())
	{
		QFile file = {filePath};

		if (file.open(QIODevice::ReadOnly))
		{
			Model model {m_documents};
			Parser parser {file};
			parser.parseBody(model);

			mainWindow()->clearSelection();

			for (LDObject* object : model.objects())
			{
				currentDocument()->insertCopy (position, object);
				mainWindow()->select(currentDocument()->index(position));
				position += 1;
			}

			m_window->refresh();
		}
		else
		{
			QMessageBox::critical(m_window, tr("Error"), format(tr("Couldn't open %1 (%2)"), filePath, file.errorString()));
		}
	}
}

void FileToolset::exportTo()
{
	if (selectedObjects().isEmpty())
		return;

	QString filePath = QFileDialog::getSaveFileName();

	if (filePath.isEmpty())
		return;

	QFile file {filePath};

	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		for (LDObject* obj : selectedObjects())
		{
			QString contents = obj->asText();
			QByteArray data = contents.toUtf8();
			file.write(data, countof(data));
			file.write("\r\n", 2);
		}
	}
	else
	{
		QMessageBox::critical(m_window, tr("Error"), format(tr("Unable to open %1 for writing: %2"), filePath, file.errorString()));
	}
}

void FileToolset::scanPrimitives()
{
	primitives()->startScan();
}

void FileToolset::openSubfiles()
{
	for (LDObject* object : selectedObjects())
	{
		LDSubfileReference* reference = dynamic_cast<LDSubfileReference*>(object);
		LDDocument* referenceDocument = reference ? reference->fileInfo(m_documents) : nullptr;

		if (referenceDocument and referenceDocument->isFrozen())
			m_window->openDocumentForEditing(referenceDocument);
	}
}

void FileToolset::downloadFrom()
{
	PartDownloader* dialog = new PartDownloader {m_window};
	connect(dialog, &PartDownloader::primaryFileDownloaded, [&]()
	{
		m_window->changeDocument (dialog->primaryFile());
		m_window->doFullRefresh();
		m_window->renderer()->resetAngles();
	});
	dialog->exec();
}

void FileToolset::makePrimitive()
{
	GeneratePrimitiveDialog* dialog = new GeneratePrimitiveDialog {m_window};

	if (dialog->exec())
	{
		LDDocument* primitive = primitives()->generatePrimitive(dialog->primitiveModel());
		m_window->openDocumentForEditing(primitive);
		m_window->save(primitive, false);
	}
}

// These are not exactly file tools but I don't want to make another toolset just for 3 very small actions
void FileToolset::help()
{
	// Not yet implemented
}

void FileToolset::about()
{
	QDialog *dialog = new QDialog(m_window);
	Ui::AboutUI ui;
	ui.setupUi(dialog);
	ui.versionInfo->setText(APPNAME " " + QString (fullVersionString()));
	dialog->setWindowTitle(format(tr("About %1"), APPNAME));
	dialog->exec();
}

void FileToolset::aboutQt()
{
	QMessageBox::aboutQt (m_window);
}
