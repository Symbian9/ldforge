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
#include "ldDocument.h"

enum { CurrentBinaryVersion = 0 };

LDProject::LDProject() {}
LDProject::~LDProject() {}

LDProjectPtr LDProject::LoadFromFile (const QString& filename)
{
	QVector<QPair<LDSubfilePtr, QString>> referenceNames;
	std::function<LDProjectPtr (const QString&)> loadProject;
	std::function<void (const QString &name, const QByteArray &data)> loadDocument;

	loadProject = [&](const QString& filename) -> LDProjectPtr
	{
		archive* arc = archive_read_new();
		archive_read_support_filter_all (arc);
		archive_read_support_format_zip (arc);
		int result = archive_read_open_filename (arc, filename.toLocal8Bit().constData(), 0x4000);

		if (result != ARCHIVE_OK)
		{
			fprint (stderr, "unable to open argh.pk3 (%1)\n", archive_error_string (arc));
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
					loadDocument (pathname.right (4), QByteArray (buffer.constData(), buffer.size()));
			}
			else
				fprint (stderr, "Unable to read %1: %2", pathname, archive_error_string (arc));
		}

		if ((result = archive_read_free(arc)) != ARCHIVE_OK)
		{
			fprint (stderr, "unable to close argh.pk3\n");
			return LDProjectPtr();
		}

		return LDProjectPtr();
	};

	loadDocument = [&](const QString &name, const QByteArray &data) -> void
	{
		QDataStream ds (&data, QIODevice::ReadOnly);
		ds.setVersion (QDataStream::Qt_4_8);

		quint16 version;
		ds << version;

		if (version > CurrentBinaryVersion)
			return; // too new

		qint8 header;
		quint32 color;
		LDDocumentPtr doc = LDDocument::createNew();
		doc->setName (name);
		LDObjectPtr obj;
		Vertex vertex;
		Matrix matrix;

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

			case 1:
				{
					LDSubfilePtr ref = LDSpawn<LDSubfile>();
					QString name;
					ds >> color;
					ds >> vertex;

					for (int i = 0; i < 9; ++i)
						ds >> matrix[i];

					ds >> name;
					ref->setColor (LDColor::fromIndex (color));
					ref->setPosition (vertex);
					ref->setTransform (matrix);

					// We leave the fileInfo null for now, references are resolved during
					// post-process. If this object references a document that we'll parse
					// later, trying to find it now would yield null.
					referenceNames.append ({ref, name});
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
				obj->setColor (LDColor::fromIndex (color));

				for (int i = 0; i < obj->numVertices(); ++i)
				{
					ds >> vertex;
					obj->setVertex (i, vertex);
				}

				doc->addObject (obj);
				break;
			}
		}
	};

	return loadProject (filename);
}

struct Xyz
{
	double x, y, z;
	Xyz (Vertex const& a) :
		x (a.x()),
		y (a.y()),
		z (a.z()) {}
	Vertex toVertex() const { return Vertex (x, y, z); }
};

QDataStream& operator>> (QDataStream& ds, Xyz& a)
{
	return ds >> a.x >> a.y >> a.z;
}

QDataStream& operator<< (QDataStream& ds, const Xyz& a)
{
	return ds << a.x << a.y << a.z;
}

struct LDProjectLoader
{

	LDProjectPtr loadProject();
	void loadDocument (const QString &name, const QByteArray &data);

};

struct ArchiveEntry
{
	archive_entry* entry;
	ArchiveEntry() : entry (archive_entry_new()) {}
	ArchiveEntry (const ArchiveEntry&) = delete;
	~ArchiveEntry() { archive_entry_free (entry); }
	void operator= (const ArchiveEntry&) = delete;
	operator archive_entry*() { return entry; }

	void clear() { archive_entry_clear (entry); }
	void setSize (size_t size) { archive_entry_set_size (entry, size); }
	void setPathName (const char* name) { archive_entry_set_pathname (entry, name); }
	void setFileType (unsigned type) { archive_entry_set_filetype (entry, type); }
	void setPermissions (int perms) { archive_entry_set_perm (entry, perms); }
};

void LDProject::saveBinaryDocuments (archive* arc)
{
	ArchiveEntry ent;

	for (LDDocumentPtr doc : m_documents)
	{
		QByteArray buffer;
		QDataStream ds (&buffer, QIODevice::WriteOnly);
		ds << CurrentBinaryVersion;

		for (LDObjectPtr obj : doc->objects())
		{
			int number;

			switch (obj->type())
			{
			case OBJ_Comment:
				ds << 0
				   << obj.staticCast<LDCommentPtr>()->text();
				break;

			case OBJ_Subfile:
				{
					LDSubfilePtr ref = obj.staticCast<LDSubfilePtr>();
					ds << 1
					   << ref->color().index()
					   << ref->position();

					for (int i = 0; i < 9; ++i)
						ds << ref->transform()[i];

					ds << ref->fileInfo()->name();
				}
				break;

			case OBJ_Line:
				number = 2;
				goto polyobj;

			case OBJ_Triangle:
				number = 3;
				goto polyobj;

			case OBJ_Quad:
				number = 4;
				goto polyobj;

			case OBJ_CondLine:
				number = 5;
			polyobj:
				ds << obj->color().index();

				for (int i = 0; i < obj->numVertices(); ++i)
					ds << obj->vertex (i);
				break;
			}
		}

		ent.clear();
		ent.setSize (buffer.size());
		ent.setPathName (QString ("doc/" + doc->name() + ".dat").toLocal8Bit());
		ent.setFileType (AE_IFREG);
		ent.setPermissions (0644);
		archive_write_header (arc, ent);
		archive_write_data (arc, buffer.constData(), buffer.size());
	}
}

LDProjectPtr LDProject::NewProject()
{
	return LDProjectPtr (new LDProject());
}

bool LDProject::save (const QString& filename)
{
	QString tempname = filename;

	if (tempname.endsWith (".ldforge"))
		tempname.chop (strlen (".ldforge"));

	if (not tempname.endsWith (".zip"))
		tempname += ".zip";

	archive* arc = archive_write_new();
	archive_write_open_filename (arc, filename);
	saveBinaryDocuments (arc);
	archive_write_close (arc);
	m_lastErrorString = archive_error_string (arc);
	archive_write_free (arc);
	return true;
}
