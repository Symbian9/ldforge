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

#include <QtGlobal>

#ifdef Q_OS_UNIX
# include <QProcess>
# include <QTemporaryFile>
# include <unistd.h>
# include <signal.h>
# include "crashCatcher.h"
# include "dialogs.h"

# ifdef Q_OS_LINUX
#  include <sys/prctl.h>
# endif

// Removes the signal handler from SIGABRT and then aborts.
static void finalAbort()
{
	struct sigaction sighandler;
	sighandler.sa_handler = SIG_DFL;
	sighandler.sa_flags = 0;
	sigaction (SIGABRT, &sighandler, 0);
	abort();
}

static void HandleCrash (int sig)
{
	static bool isActive = false;
	printf ("!! Caught signal %d, launching gdb\n", sig);

	if (isActive)
	{
		printf ("Caught signal while crash catcher is active! Execution cannot continue.\n");
		finalAbort();
	}

	pid_t const pid (getpid());
	QProcess proc;
	QTemporaryFile commandsFile;
	isActive = true;

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
		fprint (f, format ("=== Program crashed with signal %1 ===\n\n"
			"GDB stdout:\n%3\nGDB stderr:\n%4\n", sig,
			output, err));
		f.close();
	}

	printf ("Backtrace written to " UNIXNAME "-crash.log. Aborting.\n");
	finalAbort();
}

void initCrashCatcher()
{
	// List of signals to catch and crash on
	static const int signalsToCatch[] = {
		SIGSEGV, // segmentation fault
		SIGABRT, // abort() calls
		SIGFPE, // floating point exceptions (e.g. division by zero)
		SIGILL, // illegal instructions
	};

	struct sigaction sighandler;
	sighandler.sa_handler = &HandleCrash;
	sighandler.sa_flags = 0;
	sigemptyset (&sighandler.sa_mask);

	for (int sig : signalsToCatch)
	{
		if (sigaction (sig, &sighandler, null) == -1)
			fprint (stderr, "Couldn't set signal handler %1: %2", sig, strerror (errno));
	}

	print ("Crash catcher hooked to signals: %1\n", signalsToCatch);
}

#endif // Q_OS_UNIX