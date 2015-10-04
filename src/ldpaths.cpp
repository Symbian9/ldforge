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

#include <QDir>
#include "ldpaths.h"
#include "mainwindow.h"
#include "dialogs/ldrawpathdialog.h"

ConfigOption (QString LDrawPath)

LDPaths::LDPaths (QObject* parent) :
	QObject (parent),
	HierarchyElement (parent),
	m_dialog (nullptr) {}

void LDPaths::checkPaths()
{
	QString pathconfig = m_config->lDrawPath();

	if (not configurePaths (pathconfig))
	{
		m_dialog = new LDrawPathDialog (pathconfig, false);
		connect (m_dialog, SIGNAL (pathChanged(QString)), this, SLOT (configurePaths (QString)));

		if (not m_dialog->exec())
			exit (1);
		else
			m_config->setLDrawPath (m_dialog->path());
	}
}

bool LDPaths::isValid (const QDir& dir) const
{
	if (dir.exists())
	{
		if (dir.isReadable())
		{
			QStringList mustHave = { "LDConfig.ldr", "parts", "p" };
			QStringList contents = dir.entryList (mustHave);
	
			if (contents.size() == mustHave.size())
				m_error = "";
			else
				m_error = "That is not an LDraw directory! It must<br />have LDConfig.ldr, parts/ and p/.";
		}
		else
			m_error = "That directory cannot be read.";
	}
	else
		m_error = "That directory does not exist.";
	
	return m_error.isEmpty();
}

bool LDPaths::configurePaths (QString path)
{
	QDir dir (path);
	bool ok = isValid (dir);

	if (ok)
	{
		baseDir() = dir;
		ldConfigPath() = format ("%1" DIRSLASH "LDConfig.ldr", path);
		partsDir() = QDir (path + DIRSLASH "parts");
		primitivesDir() = QDir (path + DIRSLASH "p");
	}

	if (m_dialog)
		m_dialog->setStatusText (m_error.isEmpty() ? "OK" : m_error, ok);

	return ok;
}

QString& LDPaths::ldConfigPath()
{
	static QString value;
	return value;
}

QDir& LDPaths::primitivesDir()
{
	static QDir value;
	return value;
}

QDir& LDPaths::partsDir()
{
	static QDir value;
	return value;
}

QDir& LDPaths::baseDir()
{
	static QDir value;
	return value;
}
