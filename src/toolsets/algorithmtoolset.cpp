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

#include <limits>
#include <QBoxLayout>
#include <QCheckBox>
#include <QDir>
#include <QInputDialog>
#include <QSpinBox>
#include "../mainwindow.h"
#include "../main.h"
#include "../ldDocument.h"
#include "../miscallenous.h"
#include "../radioGroup.h"
#include "../glRenderer.h"
#include "../dialogs.h"
#include "../colors.h"
#include "../ldObjectMath.h"
#include "../ldobjectiterator.h"
#include "../documentmanager.h"
#include "ui_replcoords.h"
#include "ui_editraw.h"
#include "ui_flip.h"
#include "ui_addhistoryline.h"
#include "algorithmtoolset.h"

ConfigOption (int RoundPositionPrecision = 3)
ConfigOption (int RoundMatrixPrecision = 4)
ConfigOption (int SplitLinesSegments = 5)

AlgorithmToolset::AlgorithmToolset (MainWindow* parent) :
	Toolset (parent)
{
}

void AlgorithmToolset::splitQuads()
{
	int num = 0;

	for (LDObjectIterator<LDQuad> it (selectedObjects()); it.isValid(); ++it)
	{
		// Find the index of this quad
		int index = it->lineNumber();

		if (index == -1)
			return;

		QList<LDTriangle*> triangles = it->splitToTriangles();

		// Replace the quad with the first triangle and add the second triangle
		// after the first one.
		currentDocument()->setObject (index, triangles[0]);
		currentDocument()->insertObj (index + 1, triangles[1]);
		num++;
	}

	print ("%1 quadrilaterals split", num);
}

void AlgorithmToolset::editRaw()
{
	if (selectedObjects().size() != 1)
		return;

	LDObject* obj = selectedObjects()[0];
	QDialog* dlg = new QDialog;
	Ui::EditRawUI ui;

	ui.setupUi (dlg);
	ui.code->setText (obj->asText());

	if (obj->type() == OBJ_Error)
		ui.errorDescription->setText (static_cast<LDError*> (obj)->reason());
	else
	{
		ui.errorDescription->hide();
		ui.errorIcon->hide();
	}

	if (dlg->exec() == QDialog::Rejected)
		return;

	// Reinterpret it from the text of the input field
	LDObject* newobj = ParseLine (ui.code->text());
	obj->replace (newobj);
}

void AlgorithmToolset::makeBorders()
{
	LDObjectList objs = selectedObjects();
	int num = 0;

	for (LDObject* obj : objs)
	{
		const LDObjectType type = obj->type();

		if (type != OBJ_Quad and type != OBJ_Triangle)
			continue;

		LDLine* lines[4];

		if (type == OBJ_Quad)
		{
			LDQuad* quad = static_cast<LDQuad*> (obj);
			lines[0] = LDSpawn<LDLine> (quad->vertex (0), quad->vertex (1));
			lines[1] = LDSpawn<LDLine> (quad->vertex (1), quad->vertex (2));
			lines[2] = LDSpawn<LDLine> (quad->vertex (2), quad->vertex (3));
			lines[3] = LDSpawn<LDLine> (quad->vertex (3), quad->vertex (0));
		}
		else
		{
			LDTriangle* tri = static_cast<LDTriangle*> (obj);
			lines[0] = LDSpawn<LDLine> (tri->vertex (0), tri->vertex (1));
			lines[1] = LDSpawn<LDLine> (tri->vertex (1), tri->vertex (2));
			lines[2] = LDSpawn<LDLine> (tri->vertex (2), tri->vertex (0));
			lines[3] = nullptr;
		}

		for (int i = 0; i < countof (lines); ++i)
		{
			if (lines[i] == nullptr)
				continue;

			long idx = obj->lineNumber() + i + 1;
			currentDocument()->insertObj (idx, lines[i]);
			++num;
		}
	}

	print (tr ("Added %1 border lines"), num);
}

void AlgorithmToolset::roundCoordinates()
{
	setlocale (LC_ALL, "C");
	int num = 0;

	for (LDObject* obj : selectedObjects())
	{
		LDMatrixObject* mo = dynamic_cast<LDMatrixObject*> (obj);

		if (mo)
		{
			Vertex v = mo->position();
			Matrix t = mo->transform();

			v.apply ([&](Axis, double& a)
			{
				roundToDecimals (a, Config->roundPositionPrecision());
			});

			applyToMatrix (t, [&](int, double& a)
			{
				roundToDecimals (a, Config->roundMatrixPrecision());
			});

			mo->setPosition (v);
			mo->setTransform (t);
			num += 12;
		}
		else
		{
			for (int i = 0; i < obj->numVertices(); ++i)
			{
				Vertex v = obj->vertex (i);
				v.apply ([&](Axis, double& a)
				{
					roundToDecimals (a, Config->roundPositionPrecision());
				});
				obj->setVertex (i, v);
				num += 3;
			}
		}
	}

	print (tr ("Rounded %1 values"), num);
	m_window->refreshObjectList();
}

void AlgorithmToolset::replaceCoordinates()
{
	QDialog* dlg = new QDialog (m_window);
	Ui::ReplaceCoordsUI ui;
	ui.setupUi (dlg);

	if (not dlg->exec())
		return;

	const double search = ui.search->value(),
		replacement = ui.replacement->value();
	const bool any = ui.any->isChecked(),
		rel = ui.relative->isChecked();

	QList<Axis> sel;
	int num = 0;

	if (ui.x->isChecked()) sel << X;
	if (ui.y->isChecked()) sel << Y;
	if (ui.z->isChecked()) sel << Z;

	for (LDObject* obj : selectedObjects())
	{
		for (int i = 0; i < obj->numVertices(); ++i)
		{
			Vertex v = obj->vertex (i);

			v.apply ([&](Axis ax, double& coord)
			{
				if (not sel.contains (ax) or
					(not any and coord != search))
				{
					return;
				}

				if (not rel)
					coord = 0;

				coord += replacement;
				num++;
			});

			obj->setVertex (i, v);
		}
	}

	print (tr ("Altered %1 values"), num);
}

void AlgorithmToolset::flip()
{
	QDialog* dlg = new QDialog;
	Ui::FlipUI ui;
	ui.setupUi (dlg);

	if (not dlg->exec())
		return;

	QList<Axis> sel;

	if (ui.x->isChecked()) sel << X;
	if (ui.y->isChecked()) sel << Y;
	if (ui.z->isChecked()) sel << Z;

	for (LDObject* obj : selectedObjects())
	{
		for (int i = 0; i < obj->numVertices(); ++i)
		{
			Vertex v = obj->vertex (i);

			v.apply ([&](Axis ax, double& a)
			{
				if (sel.contains (ax))
					a = -a;
			});

			obj->setVertex (i, v);
		}
	}
}

void AlgorithmToolset::demote()
{
	int num = 0;

	for (LDObjectIterator<LDCondLine> it (selectedObjects()); it.isValid(); ++it)
	{
		it->toEdgeLine();
		++num;
	}

	print (tr ("Converted %1 conditional lines"), num);
}

bool AlgorithmToolset::isColorUsed (LDColor color)
{
	for (LDObject* obj : currentDocument()->objects())
	{
		if (obj->isColored() and obj->color() == color)
			return true;
	}

	return false;
}

void AlgorithmToolset::autocolor()
{
	LDColor color;

	for (color = 0; color.isLDConfigColor(); ++color)
	{
		if (color.isValid() and not isColorUsed (color))
			break;
	}

	if (not color.isLDConfigColor())
	{
		print (tr ("Cannot auto-color: all colors are in use!"));
		return;
	}

	for (LDObject* obj : selectedObjects())
	{
		if (not obj->isColored())
			continue;

		obj->setColor (color);
	}

	print (tr ("Auto-colored: new color is [%1] %2"), color.index(), color.name());
}

void AlgorithmToolset::addHistoryLine()
{
	LDObject* obj;
	bool ishistory = false;
	bool prevIsHistory = false;

	QDialog* dlg = new QDialog;
	Ui_AddHistoryLine* ui = new Ui_AddHistoryLine;
	ui->setupUi (dlg);
	ui->m_username->setText (Config->defaultUser());
	ui->m_date->setDate (QDate::currentDate());
	ui->m_comment->setFocus();

	if (not dlg->exec())
		return;

	// Create the comment object based on input
	LDComment* comment = new LDComment (format ("!HISTORY %1 [%2] %3",
		ui->m_date->date().toString ("yyyy-MM-dd"),
		ui->m_username->text(),
		ui->m_comment->text()));

	// Find a spot to place the new comment
	for (obj = currentDocument()->getObject (0);
		obj and obj->next() and not obj->next()->isScemantic();
		obj = obj->next())
	{
		LDComment* comment = dynamic_cast<LDComment*> (obj);

		if (comment and comment->text().startsWith ("!HISTORY "))
			ishistory = true;

		if (prevIsHistory and not ishistory)
			break; // Last line was history, this isn't, thus insert the new history line here.

		prevIsHistory = ishistory;
	}

	int idx = obj ? obj->lineNumber() : 0;
	currentDocument()->insertObj (idx++, comment);

	// If we're adding a history line right before a scemantic object, pad it
	// an empty line
	if (obj and obj->next() and obj->next()->isScemantic())
		currentDocument()->insertObj (idx, new LDEmpty);

	m_window->buildObjectList();
	delete ui;
}

void AlgorithmToolset::splitLines()
{
	bool ok;
	int segments = QInputDialog::getInt (m_window, APPNAME, "Amount of segments:",
		Config->splitLinesSegments(), 0, std::numeric_limits<int>::max(), 1, &ok);

	if (not ok)
		return;

	Config->setSplitLinesSegments (segments);

	for (LDObject* obj : selectedObjects())
	{
		if (not isOneOf (obj->type(), OBJ_Line, OBJ_CondLine))
			continue;

		QVector<LDObject*> newsegs;

		for (int i = 0; i < segments; ++i)
		{
			LDObject* segment;
			Vertex v0, v1;

			v0.apply ([&](Axis ax, double& a)
			{
				double len = obj->vertex (1)[ax] - obj->vertex (0)[ax];
				a = (obj->vertex (0)[ax] + ((len * i) / segments));
			});

			v1.apply ([&](Axis ax, double& a)
			{
				double len = obj->vertex (1)[ax] - obj->vertex (0)[ax];
				a = (obj->vertex (0)[ax] + ((len * (i + 1)) / segments));
			});

			if (obj->type() == OBJ_Line)
				segment = LDSpawn<LDLine> (v0, v1);
			else
				segment = LDSpawn<LDCondLine> (v0, v1, obj->vertex (2), obj->vertex (3));

			newsegs << segment;
		}

		int ln = obj->lineNumber();

		for (LDObject* seg : newsegs)
			currentDocument()->insertObj (ln++, seg);

		obj->destroy();
	}

	m_window->buildObjectList();
	m_window->refresh();
}

void AlgorithmToolset::subfileSelection()
{
	if (selectedObjects().size() == 0)
		return;

	QString			parentpath (currentDocument()->fullPath());

	// BFC type of the new subfile - it shall inherit the BFC type of the parent document
	BfcStatement	bfctype (BfcStatement::NoCertify);

	// Dirname of the new subfile
	QString			subdirname (Dirname (parentpath));

	// Title of the new subfile
	QString			subtitle;

	// Comment containing the title of the parent document
	LDComment*	titleobj = dynamic_cast<LDComment*> (currentDocument()->getObject (0));

	// License text for the subfile
	QString			license (PreferredLicenseText());

	// LDraw code body of the new subfile (i.e. code of the selection)
	QStringList		code;

	// Full path of the subfile to be
	QString			fullsubname;

	// Where to insert the subfile reference?
	int				refidx (selectedObjects()[0]->lineNumber());

	// Determine title of subfile
	if (titleobj)
		subtitle = "~" + titleobj->text();
	else
		subtitle = "~subfile";

	// Remove duplicate tildes
	while (subtitle.startsWith ("~~"))
		subtitle.remove (0, 1);

	// If this the parent document isn't already in s/, we need to stuff it into
	// a subdirectory named s/. Ensure it exists!
	QString topdirname = Basename (Dirname (currentDocument()->fullPath()));

	if (topdirname != "s")
	{
		QString desiredPath = subdirname + "/s";
		QString title = tr ("Create subfile directory?");
		QString text = format (tr ("The directory <b>%1</b> is suggested for "
			"subfiles. This directory does not exist, create it?"), desiredPath);

		if (QDir (desiredPath).exists() or Confirm (title, text))
		{
			subdirname = desiredPath;
			QDir().mkpath (subdirname);
		}
		else
			return;
	}

	// Determine the body of the name of the subfile
	if (not parentpath.isEmpty())
	{
		// Chop existing '.dat' suffix
		if (parentpath.endsWith (".dat"))
			parentpath.chop (4);

		// Remove the s?? suffix if it's there, otherwise we'll get filenames
		// like s01s01.dat when subfiling subfiles.
		QRegExp subfilesuffix ("s[0-9][0-9]$");
		if (subfilesuffix.indexIn (parentpath) != -1)
			parentpath.chop (subfilesuffix.matchedLength());

		int subidx = 1;
		QString digits;

		// Now find the appropriate filename. Increase the number of the subfile
		// until we find a name which isn't already taken.
		do
		{
			digits.setNum (subidx++);

			// pad it with a zero
			if (digits.length() == 1)
				digits.prepend ("0");

			fullsubname = subdirname + "/" + Basename (parentpath) + "s" + digits + ".dat";
		} while (m_documents->findDocumentByName ("s\\" + Basename (fullsubname)) or QFile (fullsubname).exists());
	}

	// Determine the BFC winding type used in the main document - it is to
	// be carried over to the subfile.
	for (LDObjectIterator<LDBfc> it (currentDocument()); it.isValid(); ++it)
	{
		if (isOneOf (it->statement(), BfcStatement::CertifyCCW, BfcStatement::CertifyCW, BfcStatement::NoCertify))
		{
			bfctype = it->statement();
			break;
		}
	}

	// Get the body of the document in LDraw code
	for (LDObject* obj : selectedObjects())
		code << obj->asText();

	// Create the new subfile document
	LDDocument* doc = m_window->newDocument();
	doc->openForEditing();
	doc->setFullPath (fullsubname);
	doc->setName (LDDocument::shortenName (fullsubname));

	LDObjectList objs;
	objs << LDSpawn<LDComment> (subtitle);
	objs << LDSpawn<LDComment> ("Name: "); // This gets filled in when the subfile is saved
	objs << LDSpawn<LDComment> (format ("Author: %1 [%2]", Config->defaultName(), Config->defaultUser()));
	objs << LDSpawn<LDComment> ("!LDRAW_ORG Unofficial_Subpart");

	if (not license.isEmpty())
		objs << LDSpawn<LDComment> (license);

	objs << LDSpawn<LDEmpty>();
	objs << LDSpawn<LDBfc> (bfctype);
	objs << LDSpawn<LDEmpty>();

	doc->addObjects (objs);

	// Add the actual subfile code to the new document
	for (QString line : code)
	{
		LDObject* obj = ParseLine (line);
		doc->addObject (obj);
	}

	// Try save it
	if (m_window->save (doc, true))
	{
		// Save was successful. Delete the original selection now from the
		// main document.
		for (LDObject* obj : selectedObjects())
			obj->destroy();

		// Add a reference to the new subfile to where the selection was
		LDSubfileReference* ref = LDSpawn<LDSubfileReference>();
		ref->setColor (MainColor);
		ref->setFileInfo (doc);
		ref->setPosition (Origin);
		ref->setTransform (IdentityMatrix);
		currentDocument()->insertObj (refidx, ref);

		// Refresh stuff
		m_window->updateDocumentList();
		m_window->doFullRefresh();
	}
	else
	{
		// Failed to save.
		doc->close();
	}
}