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

#include <QApplication>
#include <QMessageBox>
#include <QAbstractButton>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include "mainwindow.h"
#include "ldDocument.h"
#include "miscallenous.h"
#include "colors.h"
#include "basics.h"
#include "primitives.h"
#include "glRenderer.h"
#include "dialogs.h"
#include "crashCatcher.h"
#include "ldpaths.h"
#include "documentmanager.h"

MainWindow* g_win = nullptr;
ConfigurationValueBag* Config = nullptr;
const Vertex Origin (0.0f, 0.0f, 0.0f);
const Matrix IdentityMatrix ({1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f});

ConfigOption (bool FirstStart = true)

// =============================================================================
//
int main (int argc, char* argv[])
{
	QApplication app (argc, argv);
	app.setOrganizationName (APPNAME);
	app.setApplicationName (APPNAME);

	static ConfigurationValueBag configObject;
	Config = &configObject;

	LDPaths* paths = new LDPaths;
	paths->checkPaths();
	paths->deleteLater();

	initCrashCatcher();
	initColors();
	MainWindow* win = new MainWindow;
	LoadPrimitives();
	win->show();

	// Process the command line
	for (int arg = 1; arg < argc; ++arg)
		win->documents()->openMainModel (QString::fromLocal8Bit (argv[arg]));

	return app.exec();
}
