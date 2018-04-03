/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2018 Teemu Piippo
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

#include <QApplication>
#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include "../canvas.h"
#include "../lddocument.h"
#include "../linetypes/comment.h"
#include "../linetypes/modelobject.h"
#include "../linetypes/triangle.h"
#include "../linetypes/quadrilateral.h"
#include "../linetypes/edgeline.h"
#include "../linetypes/conditionaledge.h"
#include "../ldobjectiterator.h"
#include "../mainwindow.h"
#include "../dialogs/colorselector.h"
#include "../dialogs/subfilereferenceeditor.h"
#include "../grid.h"
#include "../parser.h"
#include "../widgets/vertexobjecteditor.h"
#include "../algorithms/invert.h"
#include "basictoolset.h"
#include "guiutilities.h"

BasicToolset::BasicToolset (MainWindow *parent) :
	Toolset (parent) {}

int BasicToolset::copyToClipboard()
{
	const QSet<LDObject*>& objects = selectedObjects();
	int count = 0;
	qApp->clipboard()->clear();
	QString data;

	for (LDObject* obj : objects)
	{
		if (not data.isEmpty())
			data += "\n";

		data += obj->asText();
		++count;
	}

	qApp->clipboard()->setText (data);
	return count;
}

void BasicToolset::cut()
{
	int num = copyToClipboard();
	m_window->deleteSelection();
	print (tr ("%1 objects cut"), num);
}

void BasicToolset::copy()
{
	int num = copyToClipboard();
	print (tr ("%1 objects copied"), num);
}

void BasicToolset::paste()
{
	const QString clipboardText = qApp->clipboard()->text();
	int row = m_window->suggestInsertPoint();
	mainWindow()->clearSelection();
	int count = 0;

	for (QString line : clipboardText.split("\n"))
	{
		Parser::parseFromString(*currentDocument(), row, line);
		mainWindow()->select(currentDocument()->index(row));
		row += 1;
		count += 1;
	}

	print(tr("%1 objects pasted"), count);
	m_window->refresh();
}

void BasicToolset::remove()
{
	int num = m_window->deleteSelection();
	print (tr ("%1 objects deleted"), num);
}

void BasicToolset::doInline (bool deep)
{
	for (LDSubfileReference* reference : filterByType<LDSubfileReference> (selectedObjects()))
	{
		// Get the index of the subfile so we know where to insert the
		// inlined contents.
		QPersistentModelIndex referenceIndex = currentDocument()->indexOf(reference);
		int row = referenceIndex.row();

		if (referenceIndex.isValid())
		{
			Model inlined {m_documents};
			reference->inlineContents(
				m_documents,
				currentDocument()->winding(),
				inlined,
				deep,
				false
			);

			// Merge in the inlined objects
			for (LDObject* inlinedObject : inlined.objects())
			{
				currentDocument()->insertCopy(row, inlinedObject);
				mainWindow()->select(currentDocument()->index(row));
				row += 1;
			}

			// Delete the subfile now as it's been inlined.
			currentDocument()->removeRow(referenceIndex.row());
		}
	}

	for (LDBezierCurve* curve : filterByType<LDBezierCurve> (selectedObjects()))
	{
		Model curveModel {m_documents};
		curve->rasterize(curveModel, grid()->bezierCurveSegments());
		currentDocument()->replace(curve, curveModel);
	}
}

void BasicToolset::inlineShallow()
{
	doInline (false);
}

void BasicToolset::inlineDeep()
{
	doInline (true);
}

void BasicToolset::undo()
{
	currentDocument()->undo();
}

void BasicToolset::redo()
{
	currentDocument()->redo();
}

void BasicToolset::uncolor()
{
	int num = 0;

	for (LDObject* obj : selectedObjects())
	{
		if (not obj->isColored())
			continue;

		obj->setColor (obj->defaultColor());
		num++;
	}

	print (tr ("%1 objects uncolored"), num);
}

void BasicToolset::insertRaw()
{
	int row = m_window->suggestInsertPoint();

	QDialog* const dlg = new QDialog;
	QVBoxLayout* const layout = new QVBoxLayout;
	QTextEdit* const inputbox = new QTextEdit;
	QDialogButtonBox* const buttons = new QDialogButtonBox (QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

	layout->addWidget (inputbox);
	layout->addWidget (buttons);
	dlg->setLayout (layout);
	dlg->setWindowTitle (APPNAME " - Insert Raw");
	dlg->connect (buttons, SIGNAL (accepted()), dlg, SLOT (accept()));
	dlg->connect (buttons, SIGNAL (rejected()), dlg, SLOT (reject()));

	if (dlg->exec() == QDialog::Rejected)
		return;

	mainWindow()->clearSelection();

	for (QString line : QString (inputbox->toPlainText()).split ("\n"))
	{
		Parser::parseFromString(*currentDocument(), row, line);
		mainWindow()->select(currentDocument()->index(row));
		row += 1;
	}

	m_window->refresh();
}

void BasicToolset::setColor()
{
	if (selectedObjects().isEmpty())
		return;

	QSet<LDObject*> objs = selectedObjects();

	// If all selected objects have the same color, said color is our default
	// value to the color selection dialog.
	LDColor color;
	LDColor defaultcol = m_window->getUniformSelectedColor();

	// Show the dialog to the user now and ask for a color.
	if (ColorSelector::selectColor (m_window, color, defaultcol))
	{
		for (LDObject* obj : objs)
		{
			if (obj->isColored())
				obj->setColor (color);
		}
	}
}

void BasicToolset::invert()
{
	for (LDObject* object : selectedObjects())
		::invert(object, m_documents);
}

template<typename T>
static void createObject(MainWindow* window)
{
	LDObject* object = window->currentDocument()->emplaceAt<T>(window->suggestInsertPoint());
	::editObject(window, object);
}

void BasicToolset::newSubfile()
{
	createObject<LDSubfileReference>(this->m_window);
}

void BasicToolset::newLine()
{
	createObject<LDEdgeLine>(this->m_window);
}

void BasicToolset::newTriangle()
{
	createObject<LDTriangle>(this->m_window);
}

void BasicToolset::newQuadrilateral()
{
	createObject<LDQuadrilateral>(this->m_window);
}

void BasicToolset::newConditionalLine()
{
	createObject<LDConditionalEdge>(this->m_window);
}

void BasicToolset::newComment()
{
	createObject<LDComment>(this->m_window);
}

void BasicToolset::edit()
{
	if (countof(selectedObjects()) == 1)
		::editObject(this->m_window, *selectedObjects().begin());
}

void BasicToolset::modeSelect()
{
	m_window->renderer()->setEditMode (EditModeType::Select);
}

void BasicToolset::modeCurve()
{
	m_window->renderer()->setEditMode (EditModeType::Curve);
}

void BasicToolset::modeDraw()
{
	m_window->renderer()->setEditMode (EditModeType::Draw);
}

void BasicToolset::modeRectangle()
{
	m_window->renderer()->setEditMode (EditModeType::Rectangle);
}

void BasicToolset::modeCircle()
{
	m_window->renderer()->setEditMode (EditModeType::Circle);
}

void BasicToolset::modeMagicWand()
{
 	m_window->renderer()->setEditMode (EditModeType::MagicWand);
}

void BasicToolset::modeLinePath()
{
	m_window->renderer()->setEditMode (EditModeType::LinePath);
}
