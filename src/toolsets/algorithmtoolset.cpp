/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2017 Teemu Piippo
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
#include "../mathfunctions.h"
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
	int count = 0;

	for (LDObject* object : selectedObjects().toList())
	{
		if (object->numVertices() != 4)
			continue;

		Vertex v0 = object->vertex(0);
		Vertex v1 = object->vertex(1);
		Vertex v2 = object->vertex(2);
		Vertex v3 = object->vertex(3);
		LDColor color = object->color();

		// Find the index of this quad
		int index = object->lineNumber();
		if (index == -1)
			continue;

		// Create the two triangles based on this quadrilateral:
		// 0───3       0───3    3
		// │   │  --→  │  ╱    ╱│
		// │   │  --→  │ ╱    ╱ │
		// │   │  --→  │╱    ╱  │
		// 1───2       1    1───2
		LDTriangle* triangle1 = currentDocument()->emplaceReplacementAt<LDTriangle>(index, v0, v1, v3);
		LDTriangle* triangle2 = currentDocument()->emplaceAt<LDTriangle>(index + 1, v1, v2, v3);

		// The triangles also inherit the quad's color
		triangle1->setColor(color);
		triangle2->setColor(color);
		count += 1;
	}

	print ("%1 quadrilaterals split", count);
}

void AlgorithmToolset::editRaw()
{
	if (countof(selectedObjects()) != 1)
		return;

	LDObject* obj = *(selectedObjects().begin());
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
	int count = 0;

	for (LDObject* object : selectedObjects())
	{
		const LDObjectType type = object->type();

		if (type != OBJ_Quad and type != OBJ_Triangle)
			continue;

		Model lines;

		if (type == OBJ_Quad)
		{
			LDQuad* quad = static_cast<LDQuad*>(object);
			lines.emplace<LDLine>(quad->vertex (0), quad->vertex (1));
			lines.emplace<LDLine>(quad->vertex (1), quad->vertex (2));
			lines.emplace<LDLine>(quad->vertex (2), quad->vertex (3));
			lines.emplace<LDLine>(quad->vertex (3), quad->vertex (0));
		}
		else
		{
			LDTriangle* triangle = static_cast<LDTriangle*>(object);
			lines.emplace<LDLine>(triangle->vertex (0), triangle->vertex (1));
			lines.emplace<LDLine>(triangle->vertex (1), triangle->vertex (2));
			lines.emplace<LDLine>(triangle->vertex (2), triangle->vertex (0));
		}

		count += countof(lines.objects());
		currentDocument()->merge(lines, object->lineNumber() + 1);
	}

	print(tr("Added %1 border lines"), count);
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
			Matrix t = mo->transformationMatrix();

			v.apply ([&](Axis, double& a)
			{
				roundToDecimals (a, m_config->roundPositionPrecision());
			});

			applyToMatrix (t, [&](int, double& a)
			{
				roundToDecimals (a, m_config->roundMatrixPrecision());
			});

			mo->setPosition (v);
			mo->setTransformationMatrix (t);
			num += 12;
		}
		else
		{
			for (int i = 0; i < obj->numVertices(); ++i)
			{
				Vertex v = obj->vertex (i);
				v.apply ([&](Axis, double& a)
				{
					roundToDecimals (a, m_config->roundPositionPrecision());
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

	for (LDCondLine* line : filterByType<LDCondLine>(selectedObjects()))
	{
		line->becomeEdgeLine();
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
	ui->m_username->setText (m_config->defaultUser());
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
	currentDocument()->insertObject (idx++, comment);

	// If we're adding a history line right before a scemantic object, pad it
	// an empty line
	if (obj and obj->next() and obj->next()->isScemantic())
		currentDocument()->insertObject (idx, new LDEmpty);

	m_window->buildObjectList();
	delete ui;
}

void AlgorithmToolset::splitLines()
{
	bool ok;
	int numSegments = QInputDialog::getInt (m_window, APPNAME, "Amount of segments:",
		m_config->splitLinesSegments(), 0, std::numeric_limits<int>::max(), 1, &ok);

	if (not ok)
		return;

	m_config->setSplitLinesSegments (numSegments);

	for (LDObject* obj : selectedObjects())
	{
		if (not isOneOf (obj->type(), OBJ_Line, OBJ_CondLine))
			continue;

		Model segments;

		for (int i = 0; i < numSegments; ++i)
		{
			Vertex v0;
			Vertex v1;

			v0.apply ([&](Axis ax, double& a)
			{
				double len = obj->vertex (1)[ax] - obj->vertex (0)[ax];
				a = (obj->vertex (0)[ax] + ((len * i) / numSegments));
			});

			v1.apply ([&](Axis ax, double& a)
			{
				double len = obj->vertex (1)[ax] - obj->vertex (0)[ax];
				a = (obj->vertex (0)[ax] + ((len * (i + 1)) / numSegments));
			});

			if (obj->type() == OBJ_Line)
				segments.emplace<LDLine>(v0, v1);
			else
				segments.emplace<LDCondLine>(v0, v1, obj->vertex (2), obj->vertex (3));
		}

		currentDocument()->replace(obj, segments);
	}

	m_window->buildObjectList();
	m_window->refresh();
}

void AlgorithmToolset::subfileSelection()
{
	if (selectedObjects().isEmpty())
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
	QString			license = preferredLicenseText();

	// LDraw code body of the new subfile (i.e. code of the selection)
	QStringList		code;

	// Full path of the subfile to be
	QString			fullsubname;

	// Where to insert the subfile reference?
	// TODO: the selection really should be sorted by position...
	int				referencePosition = (*selectedObjects().begin())->lineNumber();

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
			if (countof(digits) == 1)
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

	// Create the new subfile document
	LDDocument* subfile = m_window->newDocument();
	subfile->openForEditing();
	subfile->setFullPath(fullsubname);
	subfile->setName(LDDocument::shortenName(fullsubname));

	Model header;
	header.emplace<LDComment>(subtitle);
	header.emplace<LDComment>("Name: "); // This gets filled in when the subfile is saved
	header.emplace<LDComment>(format("Author: %1 [%2]", m_config->defaultName(), m_config->defaultUser()));
	header.emplace<LDComment>("!LDRAW_ORG Unofficial_Subpart");

	if (not license.isEmpty())
		header.emplace<LDComment>(license);

	header.emplace<LDEmpty>();
	header.emplace<LDBfc>(bfctype);
	header.emplace<LDEmpty>();
	subfile->merge(header);

	// Copy the body over to the new document
	for (LDObject* object : selectedObjects())
		subfile->addObject(object->createCopy());

	// Try save it
	if (m_window->save(subfile, true))
	{
		// Save was successful. Delete the original selection now from the
		// main document.
		for (LDObject* object : selectedObjects().toList())
			currentDocument()->remove(object);

		// Add a reference to the new subfile to where the selection was
		currentDocument()->emplaceAt<LDSubfileReference>(referencePosition, subfile, Matrix::identity, Origin);

		// Refresh stuff
		m_window->updateDocumentList();
		m_window->doFullRefresh();
	}
	else
	{
		// Failed to save.
		subfile->close();
	}
}