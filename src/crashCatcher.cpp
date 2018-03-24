/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2018 Teemu Piippo
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
# include <signal.h>
# include <unistd.h>
# include "crashCatcher.h"

# ifdef Q_OS_LINUX
#  include <sys/prctl.h>
# endif

/*
 * finalAbort
 *
 * Removes the signal handler from SIGABRT and then aborts.
 * This really aborts instead of falling back into the crash handler.
 */
//
static void finalAbort()
{
	struct sigaction sighandler;
	sighandler.sa_handler = SIG_DFL;
	sighandler.sa_flags = 0;
	sigaction(SIGABRT, &sighandler, 0);
	abort();
}

/*
 * handleCrash
 *
 * Handles a crash signal. Launches gdb and gets debug info for a post-mortem.
 */
static void handleCrash(int signal)
{
	static bool isActive = false;
	printf("!! Caught signal %d, launching gdb\n", signal);

	if (isActive)
	{
		printf("Caught signal while crash catcher is active! Execution cannot continue.\n");
		finalAbort();
	}

	QTemporaryFile commandsFile;
	isActive = true;

	if (commandsFile.open())
	{
		commandsFile.write(format("attach %1\n", getpid()).toLocal8Bit());
		commandsFile.write("backtrace full\n");
		commandsFile.write("detach\n");
		commandsFile.write("quit");
		commandsFile.close();

		QProcess process;
		process.start("gdb", {"-x", commandsFile.fileName()});

		// Linux doesn't allow ptrace to be used on anything but direct child processes, so we need to use prctl to register an exception
		// to this to allow GDB attach to us.
#ifdef Q_OS_LINUX
		prctl(PR_SET_PTRACER, process.pid(), 0, 0, 0);
#endif

		process.waitForFinished(1000);
		QString output = process.readAllStandardOutput();
		QString errorOutput = process.readAllStandardError();
		QFile file {UNIXNAME "-crash.log"};

		if (file.open(QIODevice::WriteOnly))
		{
			fprint(file, format("=== Program crashed with signal %1 ===\n\n"
			                    "GDB stdout:\n%3\nGDB stderr:\n%4\n", signal, output, errorOutput));
			file.close();
			printf("Backtrace written to " UNIXNAME "-crash.log. Aborting.\n");
		}
		else
		{
			printf("Unable to write a crashlog: %s\n", file.errorString().toUtf8().constData());
		}
	}
	else
	{
		printf("Unable to write commands to temporary file: %s", commandsFile.errorString().toUtf8().constData());
	}

	finalAbort();
}

/*
 * initializeCrashHandler
 *
 * Creates handlers for crash signals, so that we can create backtraces for them.
 */
void initializeCrashHandler()
{
	// List of signals to catch and crash on
	QVector<int> signalsToCatch = {
		SIGSEGV, // segmentation fault
		SIGABRT, // abort() calls
		SIGFPE, // floating point exceptions (e.g. division by zero)
		SIGILL, // illegal instructions
	};

	struct sigaction sighandler;
	sighandler.sa_handler = &handleCrash;
	sighandler.sa_flags = 0;
	sigemptyset (&sighandler.sa_mask);

	for (int signal : signalsToCatch)
	{
		if (sigaction(signal, &sighandler, nullptr) == -1)
			fprint(stderr, "Couldn't set signal handler %1: %2", signal, strerror(errno));
	}
}

#endif // Q_OS_UNIX
