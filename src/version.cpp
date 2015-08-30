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

// This file only uses stock C stuff. Keeping the STL/Qt stuff out  makes this simple file very fast to compile, which
// is nice since this file must be compiled every time I commit something.

#include <time.h>
#include "version.h"
#include "hginfo.h"

const char* fullVersionString()
{
#if BUILD_ID != BUILD_RELEASE and defined (HG_DATE_VERSION)
	return VERSION_STRING "-" HG_DATE_VERSION;
#else
	return HG_DATE_VERSION;
#endif
}

const char* commitTimeString()
{
	static char buffer[256];

#ifdef HG_COMMIT_TIME
	if (buffer[0] == '\0')
	{
		time_t timestamp = HG_COMMIT_TIME;
		strftime (buffer, sizeof buffer, "%d %b %Y", localtime (&timestamp));
	}
#endif

	return buffer;
}
