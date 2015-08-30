/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Teemu Piippo
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
#include <QSpinBox>
#include <QCheckBox>
#include <QBoxLayout>
#include <QClipboard>
#include <QInputDialog>
#include "mainWindow.h"
#include "main.h"
#include "ldDocument.h"
#include "dialogs/colorselector.h"
#include "miscallenous.h"
#include "radioGroup.h"
#include "glRenderer.h"
#include "dialogs.h"
#include "colors.h"
#include "ldObjectMath.h"
#include "ui_replcoords.h"
#include "ui_editraw.h"
#include "ui_flip.h"
#include "ui_addhistoryline.h"
#include "ldobjectiterator.h"

EXTERN_CFGENTRY (String, DefaultUser)

CFGENTRY (Int, RoundPosition,		3)
CFGENTRY (Int, RoundMatrix,		4)
CFGENTRY (Int, SplitLinesSegments, 5)

// =============================================================================
//
static int CopyToClipboard()
{
	LDObjectList objs = Selection();
	int num = 0;

	// Clear the clipboard first.
	qApp->clipboard()->clear();

	// Now, copy the contents into the clipboard.
	QString data;

	for (LDObject* obj : objs)
	{
		if (not data.isEmpty())
			data += "\n";

		data += obj->asText();
		++num;
	}

	qApp->clipboard()->setText (data);
	return num;
}

// =============================================================================
//
void MainWindow::slot_actionCut()
{
	int num = CopyToClipboard();
	deleteSelection();
	print (tr ("%1 objects cut"), num);
}

// =============================================================================
//
void MainWindow::slot_actionCopy()
{
	int num = CopyToClipboard();
	print (tr ("%1 objects copied"), num);
}

// =============================================================================
//
void MainWindow::slot_actionPaste()
{
	const QString clipboardText = qApp->clipboard()->text();
	int idx = getInsertionPoint();
	CurrentDocument()->clearSelection();
	int num = 0;

	for (QString line : clipboardText.split ("\n"))
	{
		LDObject* pasted = ParseLine (line);
		CurrentDocument()->insertObj (idx++, pasted);
		pasted->select();
		++num;
	}

	print (tr ("%1 objects pasted"), num);
	refresh();
	scrollToSelection();
}

// =============================================================================
//
void MainWindow::slot_actionDelete()
{
	int num = deleteSelection();
	print (tr ("%1 objects deleted"), num);
}

// =============================================================================
//
static void DoInline (bool deep)
{
	LDObjectList sel = Selection();

	for (LDObjectIterator<LDSubfile> it (Selection()); it.isValid(); ++it)
	{
		// Get the index of the subfile so we know where to insert the
		// inlined contents.
		long idx = it->lineNumber();

		assert (idx != -1);
		LDObjectList objs = it->inlineContents (deep, false);

		// Merge in the inlined objects
		for (LDObject* inlineobj : objs)
		{
			QString line = inlineobj->asText();
			inlineobj->destroy();
			LDObject* newobj = ParseLine (line);
			CurrentDocument()->insertObj (idx++, newobj);
			newobj->select();
		}

		// Delete the subfile now as it's been inlined.
		it->destroy();
	}
}

void MainWindow::slot_actionInline()
{
	DoInline (false);
	refresh();
}

void MainWindow::slot_actionInlineDeep()
{
	DoInline (true);
	refresh();
}

// =============================================================================
//
void MainWindow::slot_actionSplitQuads()
{
	int num = 0;

	for (LDObjectIterator<LDQuad> it (Selection()); it.isValid(); ++it)
	{
		// Find the index of this quad
		long index = it->lineNumber();

		if (index == -1)
			return;

		QList<LDTriangle*> triangles = it->splitToTriangles();

		// Replace the quad with the first triangle and add the second triangle
		// after the first one.
		CurrentDocument()->setObject (index, triangles[0]);
		CurrentDocument()->insertObj (index + 1, triangles[1]);
		num++;
	}

	print ("%1 quadrilaterals split", num);
	refresh();
}

// =============================================================================
//
void MainWindow::slot_actionEditRaw()
{
	if (Selection().size() != 1)
		return;

	LDObject* obj = Selection()[0];
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
	refresh();
}

// =============================================================================
//
void MainWindow::slot_actionSetColor()
{
	if (Selection().isEmpty())
		return;

	LDObjectList objs = Selection();

	// If all selected objects have the same color, said color is our default
	// value to the color selection dialog.
	LDColor color;
	LDColor defaultcol = getSelectedColor();

	// Show the dialog to the user now and ask for a color.
	if (ColorSelector::selectColor (color, defaultcol, this))
	{
		for (LDObject* obj : objs)
		{
			if (obj->isColored())
				obj->setColor (color);
		}

		refresh();
	}
}

// =============================================================================
//
void MainWindow::slot_actionBorders()
{
	LDObjectList objs = Selection();
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
		}

		for (int i = 0; i < countof (lines); ++i)
		{
			if (lines[i] == null)
				continue;

			long idx = obj->lineNumber() + i + 1;
			CurrentDocument()->insertObj (idx, lines[i]);
		}

		num += countof (lines);
	}

	print (tr ("Added %1 border lines"), num);
	refresh();
}

// =============================================================================
//
void MainWindow::slot_actionCornerVerts()
{
	int num = 0;

	for (LDObject* obj : Selection())
	{
		if (obj->numVertices() < 2)
			continue;

		int ln = obj->lineNumber();

		for (int i = 0; i < obj->numVertices(); ++i)
		{
			LDVertex* vertex = new LDVertex();
			vertex->pos = obj->vertex (i);
			vertex->setColor (obj->color());
			CurrentDocument()->insertObj (++ln, vertex);
			++num;
		}
	}

	print (tr ("Added %1 vertices"), num);
	refresh();
}

// =============================================================================
//
static void MoveSelection (MainWindow* win, bool up)
{
	LDObjectList objs = Selection();
	LDObject::moveObjects (objs, up);
	win->buildObjList();
}

// =============================================================================
//
void MainWindow::slot_actionMoveUp()
{
	MoveSelection (this, true);
}

void MainWindow::slot_actionMoveDown()
{
	MoveSelection (this, false);
}

// =============================================================================
//
void MainWindow::slot_actionUndo()
{
	CurrentDocument()->undo();
}

void MainWindow::slot_actionRedo()
{
	CurrentDocument()->redo();
}

// =============================================================================
//
static void MoveObjects (Vertex vect)
{
	// Apply the grid values
	vect *= *CurrentGrid().coordinateSnap;

	for (LDObject* obj : Selection())
		obj->move (vect);
}

// =============================================================================
//
void MainWindow::slot_actionMoveXNeg()
{
	MoveObjects ({-1, 0, 0});
}

void MainWindow::slot_actionMoveYNeg()
{
	MoveObjects ({0, -1, 0});
}

void MainWindow::slot_actionMoveZNeg()
{
	MoveObjects ({0, 0, -1});
}

void MainWindow::slot_actionMoveXPos()
{
	MoveObjects ({1, 0, 0});
}

void MainWindow::slot_actionMoveYPos()
{
	MoveObjects ({0, 1, 0});
}

void MainWindow::slot_actionMoveZPos()
{
	MoveObjects ({0, 0, 1});
}

// =============================================================================
//
void MainWindow::slot_actionInvert()
{
	for (LDObject* obj : Selection())
		obj->invert();

	refresh();
}

// =============================================================================
//
static double GetRotateActionAngle()
{
	return (Pi * *CurrentGrid().angleSnap) / 180;
}

void MainWindow::slot_actionRotateXPos()
{
	RotateObjects (1, 0, 0, GetRotateActionAngle(), Selection());
}
void MainWindow::slot_actionRotateYPos()
{
	RotateObjects (0, 1, 0, GetRotateActionAngle(), Selection());
}
void MainWindow::slot_actionRotateZPos()
{
	RotateObjects (0, 0, 1, GetRotateActionAngle(), Selection());
}
void MainWindow::slot_actionRotateXNeg()
{
	RotateObjects (-1, 0, 0, GetRotateActionAngle(), Selection());
}
void MainWindow::slot_actionRotateYNeg()
{
	RotateObjects (0, -1, 0, GetRotateActionAngle(), Selection());
}
void MainWindow::slot_actionRotateZNeg()
{
	RotateObjects (0, 0, -1, GetRotateActionAngle(), Selection());
}

void MainWindow::slot_actionRotationPoint()
{
	ConfigureRotationPoint();
}

// =============================================================================
//
void MainWindow::slot_actionRoundCoordinates()
{
	setlocale (LC_ALL, "C");
	int num = 0;

	for (LDObject* obj : Selection())
	{
		LDMatrixObject* mo = dynamic_cast<LDMatrixObject*> (obj);

		if (mo != null)
		{
			Vertex v = mo->position();
			Matrix t = mo->transform();

			// Note: matrix values are to be rounded to 4 decimals.
			v.apply ([](Axis, double& a) { RoundToDecimals (a, cfg::RoundPosition); });
			ApplyToMatrix (t, [](int, double& a) { RoundToDecimals (a, cfg::RoundMatrix); });

			mo->setPosition (v);
			mo->setTransform (t);
			num += 12;
		}
		else
		{
			for (int i = 0; i < obj->numVertices(); ++i)
			{
				Vertex v = obj->vertex (i);
				v.apply ([](Axis, double& a) { RoundToDecimals (a, cfg::RoundPosition); });
				obj->setVertex (i, v);
				num += 3;
			}
		}
	}

	print (tr ("Rounded %1 values"), num);
	refreshObjectList();
	refresh();
}

// =============================================================================
//
void MainWindow::slot_actionUncolor()
{
	int num = 0;

	for (LDObject* obj : Selection())
	{
		if (not obj->isColored())
			continue;

		obj->setColor (obj->defaultColor());
		num++;
	}

	print (tr ("%1 objects uncolored"), num);
	refresh();
}

// =============================================================================
//
void MainWindow::slot_actionReplaceCoords()
{
	QDialog* dlg = new QDialog (this);
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

	for (LDObject* obj : Selection())
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
	refresh();
}

// =============================================================================
//
void MainWindow::slot_actionFlip()
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

	for (LDObject* obj : Selection())
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

	refresh();
}

// =============================================================================
//
void MainWindow::slot_actionDemote()
{
	int num = 0;

	for (LDObjectIterator<LDCondLine> it (Selection()); it.isValid(); ++it)
	{
		it->toEdgeLine();
		++num;
	}

	print (tr ("Converted %1 conditional lines"), num);
	refresh();
}

// =============================================================================
//
static bool IsColorUsed (LDColor color)
{
	for (LDObject* obj : CurrentDocument()->objects())
	{
		if (obj->isColored() and obj->color() == color)
			return true;
	}

	return false;
}

// =============================================================================
//
void MainWindow::slot_actionAutocolor()
{
	LDColor color;

	for (color = 0; color.isLDConfigColor(); ++color)
	{
		if (color.isValid() and not IsColorUsed (color))
			break;
	}

	if (not color.isLDConfigColor())
	{
		print (tr ("Cannot auto-color: all colors are in use!"));
		return;
	}

	for (LDObject* obj : Selection())
	{
		if (not obj->isColored())
			continue;

		obj->setColor (color);
	}

	print (tr ("Auto-colored: new color is [%1] %2"), color.index(), color.name());
	refresh();
}

// =============================================================================
//
void MainWindow::slot_actionAddHistoryLine()
{
	LDObject* obj;
	bool ishistory = false;
	bool prevIsHistory = false;

	QDialog* dlg = new QDialog;
	Ui_AddHistoryLine* ui = new Ui_AddHistoryLine;
	ui->setupUi (dlg);
	ui->m_username->setText (cfg::DefaultUser);
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
	for (obj = CurrentDocument()->getObject (0);
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
	CurrentDocument()->insertObj (idx++, comment);

	// If we're adding a history line right before a scemantic object, pad it
	// an empty line
	if (obj and obj->next() and obj->next()->isScemantic())
		CurrentDocument()->insertObj (idx, new LDEmpty);

	buildObjList();
	delete ui;
}

void MainWindow::slot_actionSplitLines()
{
	bool ok;
	int segments = QInputDialog::getInt (this, APPNAME, "Amount of segments:", cfg::SplitLinesSegments, 0,
		std::numeric_limits<int>::max(), 1, &ok);

	if (not ok)
		return;

	cfg::SplitLinesSegments = segments;

	for (LDObject* obj : Selection())
	{
		if (not Eq (obj->type(), OBJ_Line, OBJ_CondLine))
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
			CurrentDocument()->insertObj (ln++, seg);

		obj->destroy();
	}

	buildObjList();
	refresh();
}
