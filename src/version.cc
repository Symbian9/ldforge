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

// Note: only using stock C stuff in this file, keeping the STL/Qt stuff out
// makes this simple file very fast to compile, which is nice since this file
// must be compiled every time I commit something.

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "version.h"
#include "hginfo.h"

// =============================================================================
//
const char* VersionString()
{
	static char result[64] = {'\0'};

	if (result[0] == '\0')
	{
#if VERSION_PATCH == 0
		sprintf (result, "%d.%d", VERSION_MAJOR, VERSION_MINOR);
#else
		sprintf (g_versionString, "%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
#endif // VERSION_PATCH
	}

	return result;
}

// =============================================================================
//
const char* FullVersionString()
{
	static char result[256] = {'\0'};

	if (result[0] == '\0')
	{
#if BUILD_ID != BUILD_RELEASE and defined (HG_NODE)
		sprintf (result, "%s-" HG_NODE, VersionString());
#else
		sprintf (g_fullVersionString, "%s", VersionString());
#endif
	}

	return result;
}

// =============================================================================
//
const char* CommitTimeString()
{
	static char result[256] = {'\0'};

#ifdef HG_DATE_TIME
	if (result[0] == '\0')
	{
		time_t timestamp = HG_DATE_TIME;
		strftime (result, sizeof result, "%d %b %Y", localtime (&timestamp));
	}
#endif

	return result;
}
