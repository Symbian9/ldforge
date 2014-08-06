/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Teemu Piippo
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
#include <QDir>
#include "mainWindow.h"
#include "ldDocument.h"
#include "miscallenous.h"
#include "configuration.h"
#include "colors.h"
#include "basics.h"
#include "primitives.h"
#include "glRenderer.h"
#include "configDialog.h"
#include "dialogs.h"
#include "crashCatcher.h"

MainWindow* g_win = null;
static QString g_versionString, g_fullVersionString;
static bool g_IsExiting (false);

const Vertex Origin (0.0f, 0.0f, 0.0f);
const Matrix IdentityMatrix ({1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f});

CFGENTRY (Bool, FirstStart, true)

// =============================================================================
//
int main (int argc, char* argv[])
{
	QApplication app (argc, argv);
	app.setOrganizationName (APPNAME);
	app.setApplicationName (APPNAME);
	InitCrashCatcher();
	Config::Initialize();

	// Load or create the configuration
	if (not Config::Load())
	{
		print ("Creating configuration file...\n");

		if (Config::Save())
			print ("Configuration file successfully created.\n");
		else
			CriticalError ("Failed to create configuration file!\n");
	}

	LDPaths::initPaths();
	InitColors();
	LoadPrimitives();
	MainWindow* win = new MainWindow;
	newFile();
	win->show();

	// If this is the first start, get the user to configuration. Especially point
	// them to the profile tab, it's the most important form to fill in.
	if (cfg::FirstStart)
	{
		(new ConfigDialog (ConfigDialog::ProfileTab))->exec();
		cfg::FirstStart = false;
		Config::Save();
	}

	int result = app.exec();
	g_IsExiting = true;
	return result;
}

bool IsExiting()
{
	return g_IsExiting;
}

void Exit()
{
	g_IsExiting = true;
	exit (EXIT_SUCCESS);
}
