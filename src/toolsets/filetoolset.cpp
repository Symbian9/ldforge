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

#include <QFileDialog>
#include <QMessageBox>
#include "../dialogs.h"
#include "../glRenderer.h"
#include "../ldDocument.h"
#include "../mainwindow.h"
#include "../partDownloader.h"
#include "../primitives.h"
#include "../dialogs/configdialog.h"
#include "../dialogs/ldrawpathdialog.h"
#include "../dialogs/newpartdialog.h"
#include "filetoolset.h"
#include "ui_makeprim.h"

FileToolset::FileToolset (MainWindow* parent) :
	Toolset (parent) {}

void FileToolset::newPart()
{
	NewPartDialog* dlg = new NewPartDialog (m_window);

	if (dlg->exec() == QDialog::Accepted)
	{
		m_window->createBlankDocument();
		dlg->fillHeader (currentDocument());
		m_window->doFullRefresh();
	}
}

void FileToolset::newFile()
{
	m_window->createBlankDocument();
}

void FileToolset::open()
{
	QString name = QFileDialog::getOpenFileName (m_window, "Open File", "", "LDraw files (*.dat *.ldr)");

	if (name.isEmpty())
		return;

	OpenMainModel (name);
}

void FileToolset::save()
{
	m_window->save (currentDocument(), false);
}

void FileToolset::saveAs()
{
	m_window->save (currentDocument(), true);
}

void FileToolset::saveAll()
{
	for (LDDocument* file : m_window->allDocuments())
		m_window->save (file, false);
}

void FileToolset::close()
{
	if (not currentDocument()->isSafeToClose())
		return;

	currentDocument()->close();
}

void FileToolset::closeAll()
{
	if (not IsSafeToCloseAll())
		return;

	CloseAllDocuments();
}

void FileToolset::settings()
{
	(new ConfigDialog (m_window))->exec();
}

void FileToolset::setLDrawPath()
{
	LDrawPathDialog* dialog = new LDrawPathDialog (m_config->lDrawPath(), true);

	if (dialog->exec())
		m_config->setLDrawPath (dialog->path());
}

void FileToolset::exit()
{
	Exit();
}

void FileToolset::insertFrom()
{
	QString fname = QFileDialog::getOpenFileName();
	int idx = m_window->suggestInsertPoint();

	if (not fname.length())
		return;

	QFile f (fname);

	if (not f.open (QIODevice::ReadOnly))
	{
		Critical (format ("Couldn't open %1 (%2)", fname, f.errorString()));
		return;
	}

	LDObjectList objs = LoadFileContents (&f, nullptr);

	currentDocument()->clearSelection();

	for (LDObject* obj : objs)
	{
		currentDocument()->insertObj (idx, obj);
		obj->select();
		m_window->renderer()->compileObject (obj);

		idx++;
	}

	m_window->refresh();
	m_window->scrollToSelection();
}

void FileToolset::exportTo()
{
	if (selectedObjects().isEmpty())
		return;

	QString fname = QFileDialog::getSaveFileName();

	if (fname.length() == 0)
		return;

	QFile file (fname);

	if (not file.open (QIODevice::WriteOnly | QIODevice::Text))
	{
		Critical (format ("Unable to open %1 for writing (%2)", fname, file.errorString()));
		return;
	}

	for (LDObject* obj : selectedObjects())
	{
		QString contents = obj->asText();
		QByteArray data = contents.toUtf8();
		file.write (data, data.size());
		file.write ("\r\n", 2);
	}
}

void FileToolset::scanPrimitives()
{
	PrimitiveScanner::start();
}

void FileToolset::openSubfiles()
{
	for (LDObject* obj : selectedObjects())
	{
		LDSubfile* ref = dynamic_cast<LDSubfile*> (obj);

		if (ref and ref->fileInfo()->isCache())
			ref->fileInfo()->openForEditing();
	}
}

void FileToolset::downloadFrom()
{
	PartDownloader::staticBegin();
}

void FileToolset::makePrimitive()
{
	PrimitivePrompt* dlg = new PrimitivePrompt (g_win);

	if (not dlg->exec())
		return;

	int segs = dlg->ui->sb_segs->value();
	int divs = dlg->ui->cb_hires->isChecked() ? HighResolution : LowResolution;
	int num = dlg->ui->sb_ringnum->value();
	PrimitiveType type =
		dlg->ui->rb_circle->isChecked()   ? Circle :
		dlg->ui->rb_cylinder->isChecked() ? Cylinder :
		dlg->ui->rb_disc->isChecked()     ? Disc :
		dlg->ui->rb_ndisc->isChecked()    ? DiscNeg :
		dlg->ui->rb_ring->isChecked()     ? Ring : Cone;

	LDDocument* f = GeneratePrimitive (type, segs, divs, num);
	f->openForEditing();
	m_window->save (f, false);
}

// These are not exactly file tools but I don't want to make another toolset just for 3 very small actions
void FileToolset::help()
{
	// Not yet implemented
}

void FileToolset::about()
{
	AboutDialog().exec();
}

void FileToolset::aboutQt()
{
	QMessageBox::aboutQt (m_window);
}