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
	archive_read_support_format_zip (arc);
	int result = archive_read_open_filename (arc, filename.toLocal8Bit().constData(), 0x4000);

	if (result != ARCHIVE_OK)
	{
		fprint (fp, "unable to open argh.pk3 (%1)\n", archive_error_string (arc));
		return LDProjectPtr();
	}

	for (archive_entry* arcent; archive_read_next_header(arc, &arcent) == ARCHIVE_OK;)
	{
		QString pathname = archive_entry_pathname (arcent);
		QVector<char> buffer;
		buffer.resize (archive_entry_size (arcent));
		int size = archive_read_data(arc, buffer.data(), buffer.size());

		if (size >= 0)
		{
			if (pathname.startsWith ("dat/"))
				loadBinaryDocument (pathname.right (4), QByteArray (buffer.constData(), buffer.size()));
		}
		else
			fprint (fp, "Unable to read %1: %2", pathname, archive_error_string (arc));
	}

	if ((result = archive_read_free(arc)) != ARCHIVE_OK)
	{
		fprint (fp, "unable to close argh.pk3\n");
		return LDProjectPtr();
	}

	return LDProjectPtr();
}

#include "ldDocument.h"
void LDProject::loadBinaryDocument(const QString &name, const QByteArray &data)
{
	QDataStream ds (&data, QIODevice::ReadOnly);
	ds.setVersion (QDataStream::Qt_4_8);
	enum { CurrentVersion = 0 };

	quint16 version;
	ds << version;

	if (version > CurrentVersion)
		return; // too new

	qint8 header;
	quint32 color;
	LDDocumentPtr doc = LDDocument::createNew();
	LDObjectPtr obj;
	struct XYZ { double x, y, z; Vertex toVertex() const { return Vertex (x,y,z); }};
	XYZ verts[4];

	while ((ds << header) != -1)
	{
		switch (header)
		{
		case 0:
			{
				QString message;
				ds >> message;
				doc->addObject (LDSpawn<LDComment> (message));
			}
			break;

		case 2:
			obj = LDSpawn<LDLine>();
			goto polyobject;

		case 3:
			obj = LDSpawn<LDTriangle>();
			goto polyobject;

		case 4:
			obj = LDSpawn<LDQuad>();
			goto polyobject;

		case 5:
			obj = LDSpawn<LDCondLine>();
		polyobject:
			ds >> color;
			for (int i = 0; i < obj->numVertices(); ++i)
			{
				XYZ v;
				ds >> v.x >> v.y >> v.z;
				obj->setVertex (i, Vertex (v.x, v.y, v.z));
			}

			doc->addObject (obj);
			break;
		}
	}

}

LDProjectPtr LDProject::NewProject()
{
	return LDProjectPtr (new LDProject());
}

bool LDProject::save (const QString &filename)
{
	return false;
}

