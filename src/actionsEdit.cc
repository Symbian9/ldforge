/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Santeri Piippo
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
#include "ui_replcoords.h"
#include "ui_editraw.h"
#include "ui_flip.h"
#include "ui_addhistoryline.h"

EXTERN_CFGENTRY (String, defaultUser)

CFGENTRY (Int,	roundPosition,		3)
CFGENTRY (Int,	roundMatrix,		4)
CFGENTRY (Int,	splitLinesSegments, 5)

// =============================================================================
//
static int copyToClipboard()
{
	LDObjectList objs = selection();
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
DEFINE_ACTION (Cut, CTRL (X))
{
	int num = copyToClipboard();
	deleteSelection();
	print (tr ("%1 objects cut"), num);
}

// =============================================================================
//
DEFINE_ACTION (Copy, CTRL (C))
{
	int num = copyToClipboard();
	print (tr ("%1 objects copied"), num);
}

// =============================================================================
//
DEFINE_ACTION (Paste, CTRL (V))
{
	const QString clipboardText = qApp->clipboard()->text();
	int idx = getInsertionPoint();
	getCurrentDocument()->clearSelection();
	int num = 0;

	for (QString line : clipboardText.split ("\n"))
	{
		LDObjectPtr pasted = parseLine (line);
		getCurrentDocument()->insertObj (idx++, pasted);
		pasted->select();
		++num;
	}

	print (tr ("%1 objects pasted"), num);
	refresh();
	scrollToSelection();
}

// =============================================================================
//
DEFINE_ACTION (Delete, KEY (Delete))
{
	int num = deleteSelection();
	print (tr ("%1 objects deleted"), num);
}

// =============================================================================
//
static void doInline (bool deep)
{
	LDObjectList sel = selection();

	for (LDObjectPtr obj : sel)
	{
		// Get the index of the subfile so we know where to insert the
		// inlined contents.
		long idx = obj->lineNumber();

		if (idx == -1 || obj->type() != OBJ_Subfile)
			continue;

		LDObjectList objs = obj.staticCast<LDSubfile>()->inlineContents (deep, false);

		// Merge in the inlined objects
		for (LDObjectPtr inlineobj : objs)
		{
			QString line = inlineobj->asText();
			inlineobj->destroy();
			LDObjectPtr newobj = parseLine (line);
			getCurrentDocument()->insertObj (idx++, newobj);
			newobj->select();
		}

		// Delete the subfile now as it's been inlined.
		obj->destroy();
	}

	g_win->refresh();
}

DEFINE_ACTION (Inline, CTRL (I))
{
	doInline (false);
}

DEFINE_ACTION (InlineDeep, CTRL_SHIFT (I))
{
	doInline (true);
}

// =============================================================================
//
DEFINE_ACTION (SplitQuads, 0)
{
	int num = 0;

	for (LDObjectPtr obj : selection())
	{
		if (obj->type() != OBJ_Quad)
			continue;

		// Find the index of this quad
		long index = obj->lineNumber();

		if (index == -1)
			return;

		QList<LDTrianglePtr> triangles = obj.staticCast<LDQuad>()->splitToTriangles();

		// Replace the quad with the first triangle and add the second triangle
		// after the first one.
		getCurrentDocument()->setObject (index, triangles[0]);
		getCurrentDocument()->insertObj (index + 1, triangles[1]);
		num++;
	}

	print ("%1 quadrilaterals split", num);
	refresh();
}

// =============================================================================
//
DEFINE_ACTION (EditRaw, KEY (F9))
{
	if (selection().size() != 1)
		return;

	LDObjectPtr obj = selection()[0];
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
	LDObjectPtr newobj = parseLine (ui.code->text());
	obj->replace (newobj);
	refresh();
}

// =============================================================================
//
DEFINE_ACTION (SetColor, KEY (C))
{
	if (selection().isEmpty())
		return;

	LDObjectList objs = selection();

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
DEFINE_ACTION (Borders, CTRL_SHIFT (B))
{
	LDObjectList objs = selection();
	int num = 0;

	for (LDObjectPtr obj : objs)
	{
		const LDObjectType type = obj->type();

		if (type != OBJ_Quad && type != OBJ_Triangle)
			continue;

		LDLinePtr lines[4];

		if (type == OBJ_Quad)
		{
			LDQuadPtr quad = obj.staticCast<LDQuad>();
			lines[0] = spawn<LDLine> (quad->vertex (0), quad->vertex (1));
			lines[1] = spawn<LDLine> (quad->vertex (1), quad->vertex (2));
			lines[2] = spawn<LDLine> (quad->vertex (2), quad->vertex (3));
			lines[3] = spawn<LDLine> (quad->vertex (3), quad->vertex (0));
		}
		else
		{
			LDTrianglePtr tri = obj.staticCast<LDTriangle>();
			lines[0] = spawn<LDLine> (tri->vertex (0), tri->vertex (1));
			lines[1] = spawn<LDLine> (tri->vertex (1), tri->vertex (2));
			lines[2] = spawn<LDLine> (tri->vertex (2), tri->vertex (0));
		}

		for (int i = 0; i < countof (lines); ++i)
		{
			if (lines[i] == null)
				continue;

			long idx = obj->lineNumber() + i + 1;
			getCurrentDocument()->insertObj (idx, lines[i]);
		}

		num += countof (lines);
	}

	print (tr ("Added %1 border lines"), num);
	refresh();
}

// =============================================================================
//
DEFINE_ACTION (CornerVerts, 0)
{
	int num = 0;

	for (LDObjectPtr obj : selection())
	{
		if (obj->numVertices() < 2)
			continue;

		int ln = obj->lineNumber();

		for (int i = 0; i < obj->numVertices(); ++i)
		{
			QSharedPointer<LDVertex> vert (spawn<LDVertex>());
			vert->pos = obj->vertex (i);
			vert->setColor (obj->color());
			getCurrentDocument()->insertObj (++ln, vert);
			++num;
		}
	}

	print (tr ("Added %1 vertices"), num);
	refresh();
}

// =============================================================================
//
static void doMoveSelection (const bool up)
{
	LDObjectList objs = selection();
	LDObject::moveObjects (objs, up);
	g_win->buildObjList();
}

// =============================================================================
//
DEFINE_ACTION (MoveUp, KEY (PageUp))
{
	doMoveSelection (true);
}

DEFINE_ACTION (MoveDown, KEY (PageDown))
{
	doMoveSelection (false);
}

// =============================================================================
//
DEFINE_ACTION (Undo, CTRL (Z))
{
	getCurrentDocument()->undo();
}

DEFINE_ACTION (Redo, CTRL_SHIFT (Z))
{
	getCurrentDocument()->redo();
}

// =============================================================================
//
void doMoveObjects (Vertex vect)
{
	// Apply the grid values
	vect *= *currentGrid().coordsnap;

	for (LDObjectPtr obj : selection())
		obj->move (vect);

	g_win->refresh();
}

// =============================================================================
//
DEFINE_ACTION (MoveXNeg, KEY (Left))
{
	doMoveObjects ({-1, 0, 0});
}

DEFINE_ACTION (MoveYNeg, KEY (Home))
{
	doMoveObjects ({0, -1, 0});
}

DEFINE_ACTION (MoveZNeg, KEY (Down))
{
	doMoveObjects ({0, 0, -1});
}

DEFINE_ACTION (MoveXPos, KEY (Right))
{
	doMoveObjects ({1, 0, 0});
}

DEFINE_ACTION (MoveYPos, KEY (End))
{
	doMoveObjects ({0, 1, 0});
}

DEFINE_ACTION (MoveZPos, KEY (Up))
{
	doMoveObjects ({0, 0, 1});
}

// =============================================================================
//
DEFINE_ACTION (Invert, CTRL_SHIFT (W))
{
	for (LDObjectPtr obj : selection())
		obj->invert();

	refresh();
}

// =============================================================================
//
static void rotateVertex (Vertex& v, const Vertex& rotpoint, const Matrix& transformer)
{
	v -= rotpoint;
	v.transform (transformer, g_origin);
	v += rotpoint;
}

// =============================================================================
//
static void doRotate (const int l, const int m, const int n)
{
	LDObjectList sel = selection();
	QList<Vertex*> queue;
	const Vertex rotpoint = rotPoint (sel);
	const double angle = (pi * *currentGrid().anglesnap) / 180,
				 cosangle = cos (angle),
				 sinangle = sin (angle);

	// ref: http://en.wikipedia.org/wiki/Transformation_matrix#Rotation_2
	Matrix transform (
	{
		(l* l * (1 - cosangle)) + cosangle,
		(m* l * (1 - cosangle)) - (n* sinangle),
		(n* l * (1 - cosangle)) + (m* sinangle),

		(l* m * (1 - cosangle)) + (n* sinangle),
		(m* m * (1 - cosangle)) + cosangle,
		(n* m * (1 - cosangle)) - (l* sinangle),

		(l* n * (1 - cosangle)) - (m* sinangle),
		(m* n * (1 - cosangle)) + (l* sinangle),
		(n* n * (1 - cosangle)) + cosangle
	});

	// Apply the above matrix to everything
	for (LDObjectPtr obj : sel)
	{
		if (obj->numVertices())
		{
			for (int i = 0; i < obj->numVertices(); ++i)
			{
				Vertex v = obj->vertex (i);
				rotateVertex (v, rotpoint, transform);
				obj->setVertex (i, v);
			}
		}
		elif (obj->hasMatrix())
		{
			LDMatrixObjectPtr mo = obj.dynamicCast<LDMatrixObject>();

			// Transform the position
			Vertex v = mo->position();
			rotateVertex (v, rotpoint, transform);
			mo->setPosition (v);

			// Transform the matrix
			mo->setTransform (transform * mo->transform());
		}
		elif (obj->type() == OBJ_Vertex)
		{
			LDVertexPtr vert = obj.staticCast<LDVertex>();
			Vertex v = vert->pos;
			rotateVertex (v, rotpoint, transform);
			vert->pos = v;
		}
	}

	g_win->refresh();
}

// =============================================================================
//
DEFINE_ACTION (RotateXPos, CTRL (Right))
{
	doRotate (1, 0, 0);
}
DEFINE_ACTION (RotateYPos, CTRL (End))
{
	doRotate (0, 1, 0);
}
DEFINE_ACTION (RotateZPos, CTRL (Up))
{
	doRotate (0, 0, 1);
}
DEFINE_ACTION (RotateXNeg, CTRL (Left))
{
	doRotate (-1, 0, 0);
}
DEFINE_ACTION (RotateYNeg, CTRL (Home))
{
	doRotate (0, -1, 0);
}
DEFINE_ACTION (RotateZNeg, CTRL (Down))
{
	doRotate (0, 0, -1);
}

DEFINE_ACTION (RotationPoint, (0))
{
	configRotationPoint();
}

// =============================================================================
//
DEFINE_ACTION (RoundCoordinates, 0)
{
	setlocale (LC_ALL, "C");
	int num = 0;

	for (LDObjectPtr obj : selection())
	{
		LDMatrixObjectPtr mo = obj.dynamicCast<LDMatrixObject>();

		if (mo != null)
		{
			Vertex v = mo->position();
			Matrix t = mo->transform();

			// Note: matrix values are to be rounded to 4 decimals.
			v.apply ([](Axis, double& a) { roundToDecimals (a, cfg::roundPosition); });
			applyToMatrix (t, [](int, double& a) { roundToDecimals (a, cfg::roundMatrix); });

			mo->setPosition (v);
			mo->setTransform (t);
			num += 12;
		}
		else
		{
			for (int i = 0; i < obj->numVertices(); ++i)
			{
				Vertex v = obj->vertex (i);
				v.apply ([](Axis, double& a) { roundToDecimals (a, cfg::roundPosition); });
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
DEFINE_ACTION (Uncolor, 0)
{
	int num = 0;

	for (LDObjectPtr obj : selection())
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
DEFINE_ACTION (ReplaceCoords, CTRL (R))
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

	for (LDObjectPtr obj : selection())
	{
		for (int i = 0; i < obj->numVertices(); ++i)
		{
			Vertex v = obj->vertex (i);

			v.apply ([&](Axis ax, double& coord)
			{
				if (not sel.contains (ax) ||
					(not any && coord != search))
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
DEFINE_ACTION (Flip, CTRL_SHIFT (F))
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

	for (LDObjectPtr obj : selection())
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
DEFINE_ACTION (Demote, 0)
{
	LDObjectList sel = selection();
	int num = 0;

	for (LDObjectPtr obj : sel)
	{
		if (obj->type() != OBJ_CondLine)
			continue;

		obj.staticCast<LDCondLine>()->toEdgeLine();
		++num;
	}

	print (tr ("Demoted %1 conditional lines"), num);
	refresh();
}

// =============================================================================
//
static bool isColorUsed (LDColor color)
{
	for (LDObjectPtr obj : getCurrentDocument()->objects())
	{
		if (obj->isColored() && obj->color() == color)
			return true;
	}

	return false;
}

// =============================================================================
//
DEFINE_ACTION (Autocolor, 0)
{
	int colnum = 0;
	LDColor color;

	for (colnum = 0; colnum < numLDConfigColors() && ((color = LDColor::fromIndex (colnum)) == null || isColorUsed (color)); colnum++)
		;

	if (colnum >= numLDConfigColors())
	{
		print (tr ("Cannot auto-color: all colors are in use!"));
		return;
	}

	for (LDObjectPtr obj : selection())
	{
		if (not obj->isColored())
			continue;

		obj->setColor (color);
	}

	print (tr ("Auto-colored: new color is [%1] %2"), colnum, color->name());
	refresh();
}

// =============================================================================
//
DEFINE_ACTION (AddHistoryLine, 0)
{
	LDObjectPtr obj;
	bool ishistory = false,
		 prevIsHistory = false;

	QDialog* dlg = new QDialog;
	Ui_AddHistoryLine* ui = new Ui_AddHistoryLine;
	ui->setupUi (dlg);
	ui->m_username->setText (cfg::defaultUser);
	ui->m_date->setDate (QDate::currentDate());
	ui->m_comment->setFocus();

	if (not dlg->exec())
		return;

	// Create the comment object based on input
	QString commentText = format ("!HISTORY %1 [%2] %3",
		ui->m_date->date().toString ("yyyy-MM-dd"),
		ui->m_username->text(),
		ui->m_comment->text());

	LDCommentPtr comm (spawn<LDComment> (commentText));

	// Find a spot to place the new comment
	for (obj = getCurrentDocument()->getObject (0);
		obj != null && obj->next() != null && not obj->next()->isScemantic();
		obj = obj->next())
	{
		LDCommentPtr comm = obj.dynamicCast<LDComment>();

		if (comm != null && comm->text().startsWith ("!HISTORY "))
			ishistory = true;

		if (prevIsHistory && not ishistory)
		{
			// Last line was history, this isn't, thus insert the new history
			// line here.
			break;
		}

		prevIsHistory = ishistory;
	}

	int idx = obj ? obj->lineNumber() : 0;
	getCurrentDocument()->insertObj (idx++, comm);

	// If we're adding a history line right before a scemantic object, pad it
	// an empty line
	if (obj && obj->next() && obj->next()->isScemantic())
		getCurrentDocument()->insertObj (idx, LDEmptyPtr (spawn<LDEmpty>()));

	buildObjList();
	delete ui;
}

DEFINE_ACTION (SplitLines, 0)
{
	bool ok;
	int segments = QInputDialog::getInt (g_win, APPNAME, "Amount of segments:", cfg::splitLinesSegments, 0,
		std::numeric_limits<int>::max(), 1, &ok);

	if (not ok)
		return;

	cfg::splitLinesSegments = segments;

	for (LDObjectPtr obj : selection())
	{
		if (obj->type() != OBJ_Line && obj->type() != OBJ_CondLine)
			continue;

		QVector<LDObjectPtr> newsegs;

		for (int i = 0; i < segments; ++i)
		{
			LDObjectPtr segment;
			Vertex v0, v1;
			v0.apply ([&](Axis ax, double& a) { a = (obj->vertex (0)[ax] + (((obj->vertex (1)[ax] - obj->vertex (0)[ax]) * i) / segments)); });
			v1.apply ([&](Axis ax, double& a) { a = (obj->vertex (0)[ax] + (((obj->vertex (1)[ax] - obj->vertex (0)[ax]) * (i + 1)) / segments)); });

			if (obj->type() == OBJ_Line)
				segment = spawn<LDLine> (v0, v1);
			else
				segment = spawn<LDCondLine> (v0, v1, obj->vertex (2), obj->vertex (3));

			newsegs << segment;
		}

		int ln = obj->lineNumber();

		for (LDObjectPtr seg : newsegs)
			getCurrentDocument()->insertObj (ln++, seg);

		obj->destroy();
	}

	buildObjList();
	g_win->refresh();
}
