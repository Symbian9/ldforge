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

#include "documentloader.h"
#include "lddocument.h"
#include "linetypes/modelobject.h"
#include "mainwindow.h"
#include "dialogs/openprogressdialog.h"

DocumentLoader::DocumentLoader(
	Model* model,
	LDHeader& header,
	bool onForeground,
	QObject *parent,
) :
	QObject {parent},
	_model {model},
	_header {header},
	m_isOnForeground {onForeground} {}

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
			m_lines << QString::fromUtf8(fp->readLine()).simplified();
	}
}

void DocumentLoader::start()
{
	m_isDone = false;
	m_progress = 0;
	m_hasAborted = false;

	if (isOnForeground())
	{
		// Show a progress dialog if we're loading the main lddocument.here so we can show progress updates and keep the
		// WM posted that we're still here.
		m_progressDialog = new OpenProgressDialog(qobject_cast<QWidget*>(parent()));
		m_progressDialog->setNumLines (countof(m_lines));
		m_progressDialog->setModal (true);
		m_progressDialog->show();
		connect (this, SIGNAL (workDone()), m_progressDialog, SLOT (accept()));
		connect (m_progressDialog, SIGNAL (rejected()), this, SLOT (abort()));
	}
	else
	{
		m_progressDialog = nullptr;
	}

	// Parse the header
	while (m_progress < m_lines.size())
	{
		const QString& line = m_lines[m_progress];

		if (not line.isEmpty())
		{
			if (line.startsWith("0"))
			{
				if (m_progress == 0)
				{
					_header.description = line.mid(1).simplified();
				}
				else if (line.startsWith("0 !LDRAW_ORG"))
				{
					QStringList tokens = line.mid(strlen("0 !LDRAW_ORG"));
				}
				else if (line.startsWith("0 BFC"))
				{
					...;
				}
				else
				{
					_model->addFromString(line);
				}
				m_progress += 1;
			}
			else
			{
				break;
			}
		}
	}

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

	bool invertNext = false;

	for (; i < max and i < (int) countof(m_lines); ++i)
	{
		const QString& line = m_lines[i];

		if (line == "0 BFC INVERTNEXT")
		{
			invertNext = true;
			continue;
		}

		LDObject* obj = _model->addFromString(line);

		// Check for parse errors and warn about them
		if (obj->type() == LDObjectType::Error)
		{
			emit parseErrorMessage(format(tr("Couldn't parse line #%1: %2"), progress() + 1, static_cast<LDError*> (obj)->reason()));
			++m_warningCount;
		}

		if (invertNext and obj->type() == LDObjectType::SubfileReference)
			obj->setInverted(true);

		invertNext = false;
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
