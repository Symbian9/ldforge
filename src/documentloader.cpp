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

#include <QFile>
#include "documentloader.h"
#include "ldDocument.h"
#include "ldObject.h"
#include "mainwindow.h"
#include "dialogs/openprogressdialog.h"

DocumentLoader::DocumentLoader (Model* model, bool onForeground, QObject *parent) :
	QObject (parent),
    _model(model),
	m_warningCount (0),
	m_isDone (false),
	m_hasAborted (false),
	m_isOnForeground (onForeground) {}

bool DocumentLoader::hasAborted()
{
	return m_hasAborted;
}

bool DocumentLoader::isDone() const
{
	return m_isDone;
}

int DocumentLoader::progress() const
{
	return m_progress;
}

int DocumentLoader::warningCount() const
{
	return m_warningCount;
}

bool DocumentLoader::isOnForeground() const
{
	return m_isOnForeground;
}

const QVector<LDObject*>& DocumentLoader::objects() const
{
	return _model->objects();
}

void DocumentLoader::read (QIODevice* fp)
{
	if (fp and fp->isOpen())
	{
		while (not fp->atEnd())
			m_lines << QString::fromUtf8 (fp->readLine());
	}
}

void DocumentLoader::start()
{
	m_isDone = false;
	m_progress = 0;
	m_hasAborted = false;

	if (isOnForeground())
	{
		// Show a progress dialog if we're loading the main ldDocument.here so we can show progress updates and keep the
		// WM posted that we're still here.
		m_progressDialog = new OpenProgressDialog(qobject_cast<QWidget*>(parent()));
		m_progressDialog->setNumLines (countof(m_lines));
		m_progressDialog->setModal (true);
		m_progressDialog->show();
		connect (this, SIGNAL (workDone()), m_progressDialog, SLOT (accept()));
		connect (m_progressDialog, SIGNAL (rejected()), this, SLOT (abort()));
	}
	else
		m_progressDialog = nullptr;

	// Begin working
	work (0);
}

void DocumentLoader::work (int i)
{
	// User wishes to abort, so stop here now.
	if (hasAborted())
	{
		m_isDone = true;
		return;
	}

	// Parse up to 200 lines per iteration
	int max = i + 200;

	for (; i < max and i < (int) countof(m_lines); ++i)
	{
		QString line = m_lines[i];

		// Trim the trailing newline
		while (line.endsWith ("\n") or line.endsWith ("\r"))
			line.chop (1);

		LDObject* obj = ParseLine (line);

		// Check for parse errors and warn about them
		if (obj->type() == OBJ_Error)
		{
			print ("Couldn't parse line #%1: %2", progress() + 1, static_cast<LDError*> (obj)->reason());
			++m_warningCount;
		}

		_model->addObject(obj);
	}

	m_progress = i;

	if (m_progressDialog)
		m_progressDialog->setProgress (i);

	if (i >= countof(m_lines) - 1)
	{
		emit workDone();
		m_isDone = true;
	}
	else
	{
		// If we have a dialog to show progress output to, we cannot just call work() again immediately as the dialog
		// needs to be updated as well. Thus, we take a detour through the event loop by using the meta-object system.
		//
		// This terminates the loop here and control goes back to the function which called the file loader. It will
		// keep processing the event loop until we're ready (see loadFileContents), thus the event loop will eventually
		// catch the invokation we throw here and send us back.
		if (isOnForeground())
			QMetaObject::invokeMethod (this, "work", Qt::QueuedConnection, Q_ARG (int, i));
		else
			work (i);
	}
}

void DocumentLoader::abort()
{
	m_hasAborted = true;
}