/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2016 Teemu Piippo
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

#pragma once

#define APPNAME			"LDForge"
#define UNIXNAME		"ldforge"

#define MACRO_TO_STRING(X) MACRO_TO_STRING_2(X)
#define MACRO_TO_STRING_2(X) #X

#define VERSION_MAJOR	0
#define VERSION_MINOR	4
#define VERSION_PATCH	0

#define VERSION_MAJOR_MINOR_STRING MACRO_TO_STRING (VERSION_MAJOR) "." MACRO_TO_STRING (VERSION_MINOR)

#if VERSION_PATCH == 0
# define VERSION_STRING VERSION_MAJOR_MINOR_STRING
#else
# define VERSION_STRING VERSION_MAJOR_MINOR_STRING "." MACRO_TO_STRING (VERSION_PATCH)
#endif

//
// Build ID, this is BUILD_RELEASE for releases
//
#define BUILD_ID		BUILD_INTERNAL
#define BUILD_INTERNAL	0
#define BUILD_RELEASE	1

#ifdef DEBUG
# undef RELEASE
#endif // DEBUG

#ifdef RELEASE
# undef DEBUG
#endif // RELEASE

const char* fullVersionString();
const char* commitTimeString();
