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

#include <archive.h>
#include <archive_entry.h>
#include "ldproject.h"

LDProject::LDProject() {}
LDProject::~LDProject() {}

LDProjectPtr LDProject::LoadFromFile (const QString& filename)
{
	FILE* fp = fopen ("log.txt", "w");
	if (!fp)
		return LDProjectPtr();

	archive* arc = archive_read_new();
	archive_read_support_filter_all (arc);
	archive_read_support_format_all (arc);
	// archive_read_support_format_zip (arc);
	archive_entry* arcent;
	int result = archive_read_open_filename (arc, filename.toLocal8Bit().constData(), 0x4000);

	if (result != ARCHIVE_OK)
	{
		fprint (fp, "unable to open argh.pk3 (%1)\n", archive_error_string (arc));
		return LDProjectPtr();
	}

	while (archive_read_next_header(arc, &arcent) == ARCHIVE_OK)
	{
		QString pathname = archive_entry_pathname (arcent);
		char buffer[1024];
		int size = archive_read_data(arc, buffer, sizeof buffer);

		if (size < 0)
			fprint (fp, "Error reading entry %1: %2", pathname, archive_error_string (arc));
		else
			fprint (fp, "%1 contains: %2 (%3 bytes)\n", pathname, static_cast<const char*>(buffer), size);
	}

	if ((result = archive_read_free(arc)) != ARCHIVE_OK)
	{
		fprint (fp, "unable to close argh.pk3\n");
		return LDProjectPtr();
	}

	return LDProjectPtr();
}

LDProjectPtr LDProject::NewProject()
{
	return LDProjectPtr (new LDProject());
}

bool LDProject::save (const QString &filename)
{
	return false;
}

