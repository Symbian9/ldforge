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
#include <QDir>
#include <QInputDialog>
#include <QMessageBox>
#include "../mainwindow.h"
#include "../main.h"
#include "../lddocument.h"
#include "../miscallenous.h"
#include "../glrenderer.h"
#include "../colors.h"
#include "../mathfunctions.h"
#include "../ldobjectiterator.h"
#include "../documentmanager.h"
#include "../linetypes/comment.h"
#include "../linetypes/conditionaledge.h"
#include "../linetypes/edgeline.h"
#include "../linetypes/empty.h"
#include "../linetypes/quadrilateral.h"
#include "../linetypes/triangle.h"
#include "../parser.h"
#include "ui_replacecoordinatesdialog.h"
#include "ui_editrawdialog.h"
#include "ui_flipdialog.h"
#include "ui_addhistorylinedialog.h"
#include "algorithmtoolset.h"

AlgorithmToolset::AlgorithmToolset (MainWindow* parent) :
	Toolset (parent)
{
}

void AlgorithmToolset::splitQuads()
{
	int count = 0;

	for (LDObject* object : selectedObjects())
	{
		QModelIndex index = currentDocument()->indexOf(object);

		if (object->numVertices() != 4)
			continue;

		Vertex v0 = object->vertex(0);
		Vertex v1 = object->vertex(1);
		Vertex v2 = object->vertex(2);
		Vertex v3 = object->vertex(3);
		LDColor color = object->color();

		// Create the two triangles based on this quadrilateral:
		// 0───3       0───3    3
		// │   │  --→  │  ╱    ╱│
		// │   │  --→  │ ╱    ╱ │
		// │   │  --→  │╱    ╱  │
		// 1───2       1    1───2
		int row = index.row();
		currentDocument()->removeAt(index);
		LDTriangle* triangle1 = currentDocument()->emplaceAt<LDTriangle>(row, v0, v1, v3);
		LDTriangle* triangle2 = currentDocument()->emplaceAt<LDTriangle>(row + 1, v1, v2, v3);

		// The triangles also inherit the quad's color
		triangle1->setColor(color);
		triangle2->setColor(color);
		count += 1;
	}

	print ("%1 quadrilaterals split", count);
}

void AlgorithmToolset::editRaw()
{
	if (countof(m_window->selectedIndexes()) != 1)
		return;

	QModelIndex index = *(m_window->selectedIndexes().begin());
	LDObject* object = currentDocument()->lookup(index);
	QDialog dialog;
	Ui::EditRawUI ui;
	ui.setupUi(&dialog);
	ui.code->setText (object->asText());

	if (object->type() == LDObjectType::Error)
	{
		ui.errorDescription->setText(static_cast<LDError*>(object)->reason());
	}
	else
	{
		ui.errorDescription->hide();
		ui.errorIcon->hide();
	}

	if (dialog.exec() == QDialog::Accepted)
	{
		// Reinterpret it from the text of the input field
		int row = index.row();
		currentDocument()->removeAt(row);
		Parser::parseFromString(*currentDocument(), row, ui.code->text());
	}
}

void AlgorithmToolset::makeBorders()
{
	int count = 0;

	for (LDObject* object : selectedObjects())
	{
		const LDObjectType type = object->type();

		if (type != LDObjectType::Quadrilateral and type != LDObjectType::Triangle)
			continue;

		Model lines {m_documents};

		if (type == LDObjectType::Quadrilateral)
		{
			LDQuadrilateral* quad = static_cast<LDQuadrilateral*>(object);
			lines.emplace<LDEdgeLine>(quad->vertex (0), quad->vertex (1));
			lines.emplace<LDEdgeLine>(quad->vertex (1), quad->vertex (2));
			lines.emplace<LDEdgeLine>(quad->vertex (2), quad->vertex (3));
			lines.emplace<LDEdgeLine>(quad->vertex (3), quad->vertex (0));
		}
		else
		{
			LDTriangle* triangle = static_cast<LDTriangle*>(object);
			lines.emplace<LDEdgeLine>(triangle->vertex (0), triangle->vertex (1));
			lines.emplace<LDEdgeLine>(triangle->vertex (1), triangle->vertex (2));
			lines.emplace<LDEdgeLine>(triangle->vertex (2), triangle->vertex (0));
		}

		count += countof(lines.objects());
		currentDocument()->merge(lines, currentDocument()->indexOf(object).row() + 1);
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
}

void AlgorithmToolset::replaceCoordinates()
{
	QDialog dialog {m_window};
	Ui::ReplaceCoordsUI ui;
	ui.setupUi (&dialog);

	if (not dialog.exec())
		return;

	const double needle = ui.search->value();
	const double replacement = ui.replacement->value();
	const bool replaceAllValues= ui.any->isChecked();
	const bool relative = ui.relative->isChecked();

	QList<Axis> selectedAxes;
	int count = 0;

	if (ui.x->isChecked())
		selectedAxes << X;
	if (ui.y->isChecked())
		selectedAxes << Y;
	if (ui.z->isChecked())
		selectedAxes << Z;

	for (LDObject* obj : selectedObjects())
	{
		for (int i = 0; i < obj->numVertices(); ++i)
		{
			Vertex vertex = obj->vertex(i);

			vertex.apply([&](Axis axis, double& coordinate)
			{
				if (selectedAxes.contains(axis) and (replaceAllValues or isZero(coordinate - needle)))
				{
					if (relative)
						coordinate += replacement;
					else
						coordinate = replacement;
					count += 1;
				}
			});

			obj->setVertex(i, vertex);
		}
	}

	print(tr("Altered %1 values"), count);
}

void AlgorithmToolset::flip()
{
	QDialog dialog {m_window};
	Ui::FlipUI ui;
	ui.setupUi(&dialog);

	if (not dialog.exec())
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
	int count = 0;

	for (int i = 0; i < currentDocument()->size(); ++i)
	{
		LDObject* object = currentDocument()->objects()[i];

		if (object->type() == LDObjectType::ConditionalEdge)
		{
			Vertex v1 = object->vertex(0);
			Vertex v2 = object->vertex(1);
			LDColor color = object->color();
			currentDocument()->removeAt(i);
			LDEdgeLine* edge = currentDocument()->emplaceAt<LDEdgeLine>(i, v1, v2);
			edge->setColor(color);
			count += 1;
		}
	}

	print (tr ("Converted %1 conditional lines"), count);
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

LDObject* AlgorithmToolset::next(LDObject* object)
{
	QModelIndex index = currentDocument()->indexOf(object);

	if (index.isValid())
		return currentDocument()->getObject(index.row() + 1);
	else
		return nullptr;
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

	QDialog dialog {m_window};
	Ui_AddHistoryLine ui;
	ui.setupUi(&dialog);
	ui.m_username->setText (m_config->defaultUser());
	ui.m_date->setDate (QDate::currentDate());
	ui.m_comment->setFocus();

	if (not dialog.exec())
		return;

	// Find a spot to place the new comment
	for (obj = currentDocument()->getObject (0);
		obj and next(obj) and not next(obj)->isScemantic();
		obj = next(obj))
	{
		LDComment* comment = dynamic_cast<LDComment*> (obj);

		if (comment and comment->text().startsWith ("!HISTORY "))
			ishistory = true;

		if (prevIsHistory and not ishistory)
			break; // Last line was history, this isn't, thus insert the new history line here.

		prevIsHistory = ishistory;
	}

	int idx = obj ? currentDocument()->indexOf(obj).row() : 0;

	// Create the comment object based on input
	currentDocument()->emplaceAt<LDComment>(idx++, format("!HISTORY %1 [%2] %3",
	    ui.m_date->date().toString ("yyyy-MM-dd"),
	    ui.m_username->text(),
	    ui.m_comment->text()));

	// If we're adding a history line right before a scemantic object, pad it
	// an empty line
	if (obj and next(obj) and next(obj)->isScemantic())
		currentDocument()->emplaceAt<LDEmpty>(idx);
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
		if (not isOneOf (obj->type(), LDObjectType::EdgeLine, LDObjectType::ConditionalEdge))
			continue;

		Model segments {m_documents};

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

			if (obj->type() == LDObjectType::EdgeLine)
				segments.emplace<LDEdgeLine>(v0, v1);
			else
				segments.emplace<LDConditionalEdge>(v0, v1, obj->vertex (2), obj->vertex (3));
		}

		currentDocument()->replace(obj, segments);
	}

	m_window->refresh();
}

void AlgorithmToolset::subfileSelection()
{
	if (selectedObjects().isEmpty())
		return;

	// Determine the title of the new subfile
	QString subfileTitle;
	LDComment* titleObject = dynamic_cast<LDComment*>(currentDocument()->getObject(0));

	if (titleObject)
		subfileTitle = "~" + titleObject->text();
	else
		subfileTitle = "~subfile";

	// Remove duplicate tildes
	while (subfileTitle.startsWith("~~"))
		subfileTitle.remove(0, 1);

	// If this the parent document isn't already in s/, we need to stuff it into
	// a subdirectory named s/. Ensure it exists!
	QString topDirectoryName = Basename(Dirname(currentDocument()->fullPath()));
	QString parentDocumentPath = currentDocument()->fullPath();
	QString subfileDirectory = Dirname(parentDocumentPath);

	if (topDirectoryName != "s")
	{
		QString desiredPath = subfileDirectory + "/s";
		QString title = tr ("Create subfile directory?");
		QString text = format(tr("The directory <b>%1</b> is suggested for subfiles. "
		                         "This directory does not exist, do you want to create it?"), desiredPath);

		if (QDir(desiredPath).exists()
		    or QMessageBox::question(m_window, title, text, (QMessageBox::Yes | QMessageBox::No), QMessageBox::No) == QMessageBox::Yes)
		{
			subfileDirectory = desiredPath;
			QDir().mkpath(subfileDirectory);
		}
		else
			return;
	}

	// Determine the body of the name of the subfile
	QString fullSubfileName;

	if (not parentDocumentPath.isEmpty())
	{
		// Chop existing '.dat' suffix
		if (parentDocumentPath.endsWith (".dat"))
			parentDocumentPath.chop (4);

		// Remove the s?? suffix if it's there, otherwise we'll get filenames
		// like s01s01.dat when subfiling subfiles.
		QRegExp subfilesuffix {"s[0-9][0-9]$"};
		if (subfilesuffix.indexIn(parentDocumentPath) != -1)
			parentDocumentPath.chop(subfilesuffix.matchedLength());

		int subfileIndex = 1;
		QString digits;

		// Now find the appropriate filename. Increase the number of the subfile until we find a name which isn't already taken.
		do
		{
			digits.setNum(subfileIndex++);

			// Pad it with a zero
			if (countof(digits) == 1)
				digits.prepend("0");

			fullSubfileName = subfileDirectory + "/" + Basename(parentDocumentPath) + "s" + digits + ".dat";
		} while (m_documents->findDocumentByName("s\\" + Basename(fullSubfileName)) != nullptr or QFile {fullSubfileName}.exists());
	}

	// Determine the BFC winding type used in the main document. It will be carried over to the subfile.
	BfcStatement winding = BfcStatement::NoCertify;
	for (LDObjectIterator<LDBfc> it (currentDocument()); it.isValid(); ++it)
	{
		if (isOneOf(it->statement(), BfcStatement::CertifyCCW, BfcStatement::CertifyCW, BfcStatement::NoCertify))
		{
			winding = it->statement();
			break;
		}
	}

	// Create the new subfile document
	LDDocument* subfile = m_window->newDocument();
	subfile->setFullPath(fullSubfileName);
	subfile->setName(LDDocument::shortenName(fullSubfileName));

	Model header {m_documents};
	header.emplace<LDComment>(subfileTitle);
	header.emplace<LDComment>("Name: "); // This gets filled in when the subfile is saved
	header.emplace<LDComment>(format("Author: %1 [%2]", m_config->defaultName(), m_config->defaultUser()));
	header.emplace<LDComment>("!LDRAW_ORG Unofficial_Subpart");

	QString license = preferredLicenseText();
	if (not license.isEmpty())
		header.emplace<LDComment>(license);

	header.emplace<LDEmpty>();
	header.emplace<LDBfc>(winding);
	header.emplace<LDEmpty>();
	subfile->merge(header);

	// Copy the body over to the new document
	for (LDObject* object : selectedObjects())
		Parser::parseFromString(*subfile, Parser::EndOfModel, object->asText());

	// Try save it
	if (m_window->save(subfile, true))
	{
		// Where to insert the subfile reference?
		// TODO: the selection really should be sorted by position...
		int referencePosition = m_window->selectedIndexes().begin()->row();

		// Save was successful. Delete the original selection now from the
		// main document.
		for (LDObject* object : selectedObjects().toList())
			currentDocument()->remove(object);

		// Add a reference to the new subfile to where the selection was
		currentDocument()->emplaceAt<LDSubfileReference>(referencePosition, subfile->name(), Matrix::identity, Vertex {0, 0, 0});

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
