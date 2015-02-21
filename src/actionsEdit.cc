﻿/*
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
#include <QSpinBox>
#include <QCheckBox>
#include <QBoxLayout>
#include <QClipboard>
#include <QInputDialog>
#include "mainWindow.h"
#include "main.h"
#include "ldDocument.h"
#include "colorSelector.h"
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

	for (LDObjectPtr obj : objs)
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
		LDObjectPtr pasted = ParseLine (line);
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

	LDIterate<LDSubfile> (Selection(), [&](LDSubfilePtr const& ref)
	{
		// Get the index of the subfile so we know where to insert the
		// inlined contents.
		long idx = ref->lineNumber();

		assert (idx != -1);
		LDObjectList objs = ref->inlineContents (deep, false);

		// Merge in the inlined objects
		for (LDObjectPtr inlineobj : objs)
		{
			QString line = inlineobj->asText();
			inlineobj->destroy();
			LDObjectPtr newobj = ParseLine (line);
			CurrentDocument()->insertObj (idx++, newobj);
			newobj->select();
		}

		// Delete the subfile now as it's been inlined.
		ref->destroy();
	});

	g_win->refresh();
}

void MainWindow::slot_actionInline()
{
	DoInline (false);
}

void MainWindow::slot_actionInlineDeep()
{
	DoInline (true);
}

// =============================================================================
//
void MainWindow::slot_actionSplitQuads()
{
	int num = 0;

	LDIterate<LDQuad> (Selection(), [&](LDQuadPtr const& quad)
	{
		// Find the index of this quad
		long index = quad->lineNumber();

		if (index == -1)
			return;

		QList<LDTrianglePtr> triangles = quad->splitToTriangles();

		// Replace the quad with the first triangle and add the second triangle
		// after the first one.
		CurrentDocument()->setObject (index, triangles[0]);
		CurrentDocument()->insertObj (index + 1, triangles[1]);
		num++;
	});

	print ("%1 quadrilaterals split", num);
	refresh();
}

// =============================================================================
//
void MainWindow::slot_actionEditRaw()
{
	if (Selection().size() != 1)
		return;

	LDObjectPtr obj = Selection()[0];
	QDialog* dlg = new QDialog;
	Ui::EditRawUI ui;

	ui.setupUi (dlg);
	ui.code->setText (obj->asText());

	if (obj->type() == OBJ_Error)
		ui.errorDescription->setText (obj.staticCast<LDError>()->reason());
	else
	{
		ui.errorDescription->hide();
		ui.errorIcon->hide();
	}

	if (dlg->exec() == QDialog::Rejected)
		return;

	// Reinterpret it from the text of the input field
	LDObjectPtr newobj = ParseLine (ui.code->text());
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
	if (ColorSelector::selectColor (color, defaultcol, g_win))
	{
		for (LDObjectPtr obj : objs)
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

	for (LDObjectPtr obj : objs)
	{
		const LDObjectType type = obj->type();

		if (type != OBJ_Quad and type != OBJ_Triangle)
			continue;

		LDLinePtr lines[4];

		if (type == OBJ_Quad)
		{
			LDQuadPtr quad = obj.staticCast<LDQuad>();
			lines[0] = LDSpawn<LDLine> (quad->vertex (0), quad->vertex (1));
			lines[1] = LDSpawn<LDLine> (quad->vertex (1), quad->vertex (2));
			lines[2] = LDSpawn<LDLine> (quad->vertex (2), quad->vertex (3));
			lines[3] = LDSpawn<LDLine> (quad->vertex (3), quad->vertex (0));
		}
		else
		{
			LDTrianglePtr tri = obj.staticCast<LDTriangle>();
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
static void MoveSelection (const bool up)
{
	LDObjectList objs = Selection();
	LDObject::moveObjects (objs, up);
	g_win->buildObjList();
}

// =============================================================================
//
void MainWindow::slot_actionMoveUp()
{
	MoveSelection (true);
}

void MainWindow::slot_actionMoveDown()
{
	MoveSelection (false);
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

	for (LDObjectPtr obj : Selection())
		obj->move (vect);

	g_win->refresh();
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
	for (LDObjectPtr obj : Selection())
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

	for (LDObjectPtr obj : Selection())
	{
		LDMatrixObjectPtr mo = obj.dynamicCast<LDMatrixObject>();

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

	for (LDObjectPtr obj : Selection())
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
	QDialog* dlg = new QDialog (g_win);
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

	for (LDObjectPtr obj : Selection())
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

	for (LDObjectPtr obj : Selection())
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

	LDIterate<LDCondLine> (Selection(), [&](LDCondLinePtr const& cnd)
	{
		cnd->toEdgeLine();
		++num;
	});

	print (tr ("Demoted %1 conditional lines"), num);
	refresh();
}

// =============================================================================
//
static bool IsColorUsed (LDColor color)
{
	for (LDObjectPtr obj : CurrentDocument()->objects())
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
	int colnum = 0;
	LDColor color;

	for (colnum = 0;
		 colnum < CountLDConfigColors() and
			((color = LDColor::fromIndex (colnum)) == null or
			IsColorUsed (color));
		colnum++) {}

	if (colnum >= CountLDConfigColors())
	{
		print (tr ("Cannot auto-color: all colors are in use!"));
		return;
	}

	for (LDObjectPtr obj : Selection())
	{
		if (not obj->isColored())
			continue;

		obj->setColor (color);
	}

	print (tr ("Auto-colored: new color is [%1] %2"), colnum, color.name());
	refresh();
}

// =============================================================================
//
void MainWindow::slot_actionAddHistoryLine()
{
	LDObjectPtr obj;
	bool ishistory = false,
		 prevIsHistory = false;

	QDialog* dlg = new QDialog;
	Ui_AddHistoryLine* ui = new Ui_AddHistoryLine;
	ui->setupUi (dlg);
	ui->m_username->setText (cfg::DefaultUser);
	ui->m_date->setDate (QDate::currentDate());
	ui->m_comment->setFocus();

	if (not dlg->exec())
		return;

	// Create the comment object based on input
	QString commentText = format ("!HISTORY %1 [%2] %3",
		ui->m_date->date().toString ("yyyy-MM-dd"),
		ui->m_username->text(),
		ui->m_comment->text());

	LDCommentPtr comm (LDSpawn<LDComment> (commentText));

	// Find a spot to place the new comment
	for (obj = CurrentDocument()->getObject (0);
		obj != null and obj->next() != null and not obj->next()->isScemantic();
		obj = obj->next())
	{
		LDCommentPtr comm = obj.dynamicCast<LDComment>();

		if (comm != null and comm->text().startsWith ("!HISTORY "))
			ishistory = true;

		if (prevIsHistory and not ishistory)
		{
			// Last line was history, this isn't, thus insert the new history
			// line here.
			break;
		}

		prevIsHistory = ishistory;
	}

	int idx = obj ? obj->lineNumber() : 0;
	CurrentDocument()->insertObj (idx++, comm);

	// If we're adding a history line right before a scemantic object, pad it
	// an empty line
	if (obj and obj->next() and obj->next()->isScemantic())
		CurrentDocument()->insertObj (idx, LDEmptyPtr (LDSpawn<LDEmpty>()));

	buildObjList();
	delete ui;
}

void MainWindow::slot_actionSplitLines()
{
	bool ok;
	int segments = QInputDialog::getInt (g_win, APPNAME, "Amount of segments:", cfg::SplitLinesSegments, 0,
		std::numeric_limits<int>::max(), 1, &ok);

	if (not ok)
		return;

	cfg::SplitLinesSegments = segments;

	for (LDObjectPtr obj : Selection())
	{
		if (not Eq (obj->type(), OBJ_Line, OBJ_CondLine))
			continue;

		QVector<LDObjectPtr> newsegs;

		for (int i = 0; i < segments; ++i)
		{
			LDObjectPtr segment;
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

		for (LDObjectPtr seg : newsegs)
			CurrentDocument()->insertObj (ln++, seg);

		obj->destroy();
	}

	buildObjList();
	g_win->refresh();
}
