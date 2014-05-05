/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Santeri Piippo
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

#ifdef __unix__

#include <QString>
#include <QProcess>
#include <QTemporaryFile>
#include <QMessageBox>
#include <unistd.h>
#include <signal.h>

#ifdef Q_OS_LINUX
# include <sys/prctl.h>
#endif

#include "crashCatcher.h"
#include "basics.h"
#include "dialogs.h"
#include "mainWindow.h"

// Is the crash catcher active now?
static bool g_crashCatcherActive = false;

// If an assertion failed, what was it?
static String g_assertionFailure;

// List of signals to catch and crash on
static QList<int> g_signalsToCatch ({
	SIGSEGV, // segmentation fault
	SIGABRT, // abort() calls
	SIGFPE, // floating point exceptions (e.g. division by zero)
	SIGILL, // illegal instructions
});

// =============================================================================
//
static void handleCrash (int sig)
{
	printf ("%s: crashed with signal %d, launching gdb\n", __func__, sig);

	if (g_crashCatcherActive)
	{
		printf ("caught signal while crash catcher is active!\n");
		exit (149);
	}

	if (g_win != null)
		g_win->hide();

	const pid_t pid = getpid();
	QProcess proc;
	QTemporaryFile commandsFile;

	g_crashCatcherActive = true;

	if (commandsFile.open())
	{
		commandsFile.write (format ("attach %1\n", pid).toLocal8Bit());
		commandsFile.write (String ("backtrace full\n").toLocal8Bit());
		commandsFile.write (String ("detach\n").toLocal8Bit());
		commandsFile.write (String ("quit").toLocal8Bit());
		commandsFile.flush();
		commandsFile.close();
	}

	QStringList args ({"-x", commandsFile.fileName()});

	proc.start ("gdb", args);

	// Linux doesn't allow ptrace to be used on anything but direct child processes
	// so we need to use prctl to register an exception to this to allow GDB attach to us.
	// We need to do this now and no earlier because only now we actually know GDB's PID.
#ifdef Q_OS_LINUX
	prctl (PR_SET_PTRACER, proc.pid(), 0, 0, 0);
#endif

	proc.waitForFinished (1000);
	String output = String (proc.readAllStandardOutput());
	String err = String (proc.readAllStandardError());
	QFile f ("ldforge-crash.log");

	if (f.open (QIODevice::WriteOnly))
	{
		fprint (f, format ("=== Program crashed with signal %1 ===\n\n%2"
			"GDB stdout:\n%3\n"
			"GDB stderr:\n%4\n",
			sig, (not g_assertionFailure.isEmpty()) ? g_assertionFailure + "\n\n" : "", output, err));
		f.close();
	}

	bombBox (format ("<h3>Program crashed with signal %1</h3>\n\n"
		"%2"
		"<p><b>GDB <tt>stdout</tt>:</b></p><pre>%3</pre>\n"
		"<p><b>GDB <tt>stderr</tt>:</b></p><pre>%4</pre>",
		sig, g_assertionFailure, output, err));
}

// =============================================================================
//
void initCrashCatcher()
{
	struct sigaction sighandler;
	sighandler.sa_handler = &handleCrash;
	sighandler.sa_flags = 0;
	sigemptyset (&sighandler.sa_mask);

	for (int sig : g_signalsToCatch)
		sigaction (sig, &sighandler, null);

	print ("%1: crash catcher hooked to signals: %2\n", __func__, g_signalsToCatch);
}
#endif // #ifdef __unix__

// =============================================================================
//
// This function must be readily available in both Windows and Linux. We display
// the bomb box straight in Windows while in Linux we let abort() trigger the
// signal handler, which will cause the usual bomb box with GDB diagnostics.
// Said prompt will embed the assertion failure information.
//
void assertionFailure (const char* file, int line, const char* funcname, const char* expr)
{
	String errmsg = format (
		"<p><b>File</b>: <tt>%1</tt><br />"
		"<b>Line</b>: <tt>%2</tt><br />"
		"<b>Function:</b> <tt>%3</tt></p>"
		"<p>Assertion <b><tt>`%4'</tt></b> failed.</p>",
		file, line, funcname, expr);

	g_assertionFailure = errmsg;

#ifndef __unix__
	bombBox (errmsg);
#endif

	abort();
}