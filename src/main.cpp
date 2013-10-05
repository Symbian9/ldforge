/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 Santeri Piippo
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

#include <QApplication>
#include <QMessageBox>
#include <QAbstractButton>
#include <QFile>
#include <QTextStream>
#include "gui.h"
#include "file.h"
#include "misc.h"
#include "config.h"
#include "colors.h"
#include "types.h"
#include "primitives.h"
#include "gldraw.h"
#include "configDialog.h"

List<LDFile*> g_loadedFiles;
ForgeWindow* g_win = null;
const QApplication* g_app = null;
File g_file_stdout (stdout, File::Write);
File g_file_stderr (stderr, File::Write);
static str g_versionString, g_fullVersionString;

const vertex g_origin (0.0f, 0.0f, 0.0f);
const matrix g_identity ( {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f});

cfg (Bool, firststart, true);

// =============================================================================
// -----------------------------------------------------------------------------
int main (int argc, char* argv[])
{	QApplication app (argc, argv);
	app.setOrganizationName (APPNAME);
	app.setApplicationName (APPNAME);

	g_app = &app;
	LDFile::setCurrent (null);

	// Load or create the configuration
	if (!Config::load())
	{	print ("Creating configuration file...\n");

		if (Config::save())
			print ("Configuration file successfully created.\n");
		else
			print ("failed to create configuration file!\n");
	}

	LDPaths::initPaths();
	initColors();
	loadLogoedStuds();

	ForgeWindow* win = new ForgeWindow;
	newFile();
	win->show();

	// If this is the first start, get the user to configuration. Especially point
	// them to the profile tab, it's the most important form to fill in.
	if (firststart)
	{	(new ConfigDialog (ConfigDialog::ProfileTab))->exec();
		firststart = false;
		Config::save();
	}

	loadPrimitives();
	return app.exec();
}

// =============================================================================
// -----------------------------------------------------------------------------
void doPrint (File& f, initlist<StringFormatArg> args)
{	str msg = DoFormat (args);
	f.write (msg.toUtf8());
	f.flush();
}

// =============================================================================
// -----------------------------------------------------------------------------
void doPrint (FILE* fp, initlist<StringFormatArg> args)
{	str msg = DoFormat (args);
	fwrite (msg.toStdString().c_str(), 1, msg.length(), fp);
	fflush (fp);
}

// =============================================================================
// -----------------------------------------------------------------------------
QString versionString()
{	if (g_versionString.length() == 0)
	{
#if VERSION_PATCH == 0
		g_versionString = fmt ("%1.%2", VERSION_MAJOR, VERSION_MINOR);
#else
		g_versionString = fmt ("%1.%2.%3", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
#endif // VERSION_PATCH
	}

	return g_versionString;
}

// =============================================================================
// -----------------------------------------------------------------------------
QString versionMoniker()
{
#if BUILD_ID == BUILD_INTERNAL
	return "Internal";
#elif BUILD_ID == BUILD_ALPHA
	return "Alpha";
#elif BUILD_ID == BUILD_BETA
	return "Beta";
#elif BUILD_ID == BUILD_RC
	return fmt ("RC %1", RC_NUMBER);
#else
	return "";
#endif // BUILD_ID
}

// =============================================================================
// -----------------------------------------------------------------------------
QString fullVersionString()
{	return fmt ("v%1 %2", versionString(), versionMoniker());
}

// =============================================================================
// -----------------------------------------------------------------------------
void assertionFailure (const char* file, const ulong line, const char* funcname, const char* expr)
{	str errmsg = fmt ("File: %1\nLine: %2:\nFunction %3:\n\nAssertion `%4' failed",
					  file, line, funcname, expr);

#ifndef RELEASE
	errmsg += ", aborting.";
#else
	errmsg += ".";
#endif

	printf ("%s\n", errmsg.toStdString().c_str());

#ifndef RELEASE
	if (g_win)
		g_win->deleteLater();

	errmsg.replace ("\n", "<br />");
	
	QMessageBox box (null);
	const QMessageBox::StandardButton btn = QMessageBox::Close;
	box.setWindowTitle ("Fatal Error");
	box.setIconPixmap (getIcon ("bomb"));
	box.setWindowIcon (getIcon ("ldforge"));
	box.setText (errmsg);
	box.addButton (btn);
	box.button (btn)->setText ("Damn it");
	box.setDefaultButton (btn);
	box.exec();
	abort();
#endif
}