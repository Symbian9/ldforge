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

#include <QApplication>
#include "crashCatcher.h"
#include "ldpaths.h"
#include "documentmanager.h"
#include "mainwindow.h"

MainWindow* g_win = nullptr;
const Vertex Origin (0.0f, 0.0f, 0.0f);

// =============================================================================
//
int main (int argc, char* argv[])
{
	QApplication app (argc, argv);
	app.setOrganizationName (APPNAME);
	app.setApplicationName (APPNAME);

	static Configuration configObject;
	LDPaths* paths = new LDPaths(&configObject);
	paths->checkPaths();
	paths->deleteLater();

	initializeCrashHandler();
	LDColor::initColors();
	MainWindow* win = new MainWindow(configObject);
	win->show();

	// Process the command line
	for (int arg = 1; arg < argc; ++arg)
		win->documents()->openMainModel (QString::fromLocal8Bit (argv[arg]));

	return app.exec();
}
