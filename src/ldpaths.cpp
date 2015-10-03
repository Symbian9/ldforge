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

#include <QDebug>
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
