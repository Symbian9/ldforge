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

#include <QProcess>
#include <QTemporaryFile>
#include <unistd.h>
#include <signal.h>
#include "crashCatcher.h"
#include "dialogs.h"
 
#ifdef __unix__
# ifdef Q_OS_LINUX
#  include <sys/prctl.h>
# endif

// Is the crash catcher active now?
static bool IsCrashCatcherActive = false;

// If an assertion failed, what was it?
static QString AssertionFailureText;

// List of signals to catch and crash on
static QList<int> SignalsToCatch ({
	SIGSEGV, // segmentation fault
	SIGABRT, // abort() calls
	SIGFPE, // floating point exceptions (e.g. division by zero)
	SIGILL, // illegal instructions
});

// -------------------------------------------------------------------------------------------------
//
//	Removes the signal handler from SIGABRT and then aborts.
//
static void FinalAbort()
{
	struct sigaction sighandler;
	sighandler.sa_handler = SIG_DFL;
	sighandler.sa_flags = 0;
	sigaction (SIGABRT, &sighandler, 0);
	abort();
}

// -------------------------------------------------------------------------------------------------
//
static void HandleCrash (int sig)
{
	printf ("!! Caught signal %d, launching gdb\n", sig);

	if (IsCrashCatcherActive)
	{
		printf ("Caught signal while crash catcher is active! Execution cannot continue.\n");
		FinalAbort();
	}

	pid_t const pid (getpid());
	QProcess proc;
	QTemporaryFile commandsFile;

	IsCrashCatcherActive = true;

	if (commandsFile.open())
	{
		commandsFile.write (format ("attach %1\n", pid).toLocal8Bit());
		commandsFile.write (QString ("backtrace full\n").toLocal8Bit());
		commandsFile.write (QString ("detach\n").toLocal8Bit());
		commandsFile.write (QString ("quit").toLocal8Bit());
		commandsFile.close();
	}

	proc.start ("gdb", {"-x", commandsFile.fileName()});

	// Linux doesn't allow ptrace to be used on anything but direct child processes
	// so we need to use prctl to register an exception to this to allow GDB attach to us.
	// We need to do this now and no earlier because only now we actually know GDB's PID.
#ifdef Q_OS_LINUX
	prctl (PR_SET_PTRACER, proc.pid(), 0, 0, 0);
#endif

	proc.waitForFinished (1000);
	QString output (proc.readAllStandardOutput());
	QString err (proc.readAllStandardError());
	QFile f (UNIXNAME "-crash.log");

	if (f.open (QIODevice::WriteOnly))
	{
		fprint (f, format ("=== Program crashed with signal %1 ===\n\n%2"
			"GDB stdout:\n%3\nGDB stderr:\n%4\n", sig,
			(not AssertionFailureText.isEmpty()) ? AssertionFailureText + "\n\n" : "",
			output, err));
		f.close();
	}

	if (not AssertionFailureText.isEmpty())
		printf ("Assertion failed: \"%s\".\n", qPrintable (AssertionFailureText));

	printf ("Backtrace written to " UNIXNAME "-crash.log. Aborting.\n");
	FinalAbort();
}

// -------------------------------------------------------------------------------------------------
//
//	Initializes the crash catcher.
//
void InitCrashCatcher()
{
	struct sigaction sighandler;
	sighandler.sa_handler = &HandleCrash;
	sighandler.sa_flags = 0;
	sigemptyset (&sighandler.sa_mask);

	for (int sig : SignalsToCatch)
	{
		if (sigaction (sig, &sighandler, null) == -1)
		{
			fprint (stderr, "Couldn't set signal handler %1: %2", sig, strerror (errno));
			SignalsToCatch.removeOne (sig);
		}
	}

	print ("Crash catcher hooked to signals: %1\n", SignalsToCatch);
}

#endif // #ifdef __unix__

// -------------------------------------------------------------------------------------------------
//
// This function catches an assertion failure. It must be readily available in both Windows and
// Linux. We display the bomb box straight in Windows while in Linux we let abort() trigger
// the signal handler, which will cause the usual bomb box with GDB diagnostics. Said prompt will
// embed the assertion failure information.
//
void HandleAssertFailure (const char* file, int line, const char* funcname, const char* expr)
{
#ifdef __unix__
	AssertionFailureText = format ("%1:%2: %3: %4", file, line, funcname, expr);
#else
	bombBox (format (
		"<p><b>File</b>: <tt>%1</tt><br />"
		"<b>Line</b>: <tt>%2</tt><br />"
		"<b>Function:</b> <tt>%3</tt></p>"
		"<p>Assertion <b><tt>`%4'</tt></b> failed.</p>",
		file, line, funcname, expr));
#endif

	abort();
}