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

#include <QApplication>
#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include "../addObjectDialog.h"
#include "../glRenderer.h"
#include "../ldDocument.h"
#include "../ldObject.h"
#include "../ldobjectiterator.h"
#include "../mainwindow.h"
#include "../dialogs/colorselector.h"
#include "basictoolset.h"

BasicToolset::BasicToolset (MainWindow *parent) :
	Toolset (parent) {}

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

void BasicToolset::cut()
{
	int num = CopyToClipboard();
	m_window->deleteSelection();
	print (tr ("%1 objects cut"), num);
}

void BasicToolset::copy()
{
	int num = CopyToClipboard();
	print (tr ("%1 objects copied"), num);
}

void BasicToolset::paste()
{
	const QString clipboardText = qApp->clipboard()->text();
	int idx = m_window->getInsertionPoint();
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
	m_window->refresh();
	m_window->scrollToSelection();
}

void BasicToolset::remove()
{
	int num = m_window->deleteSelection();
	print (tr ("%1 objects deleted"), num);
}

void BasicToolset::doInline (bool deep)
{
	for (LDObjectIterator<LDSubfile> it (Selection()); it.isValid(); ++it)
	{
		// Get the index of the subfile so we know where to insert the
		// inlined contents.
		int idx = it->lineNumber();

		if (idx != -1)
		{
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
	CurrentDocument()->undo();
}

void BasicToolset::redo()
{
	CurrentDocument()->redo();
}

void BasicToolset::uncolor()
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
}

void BasicToolset::insertRaw()
{
	int idx = m_window->getInsertionPoint();

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

	CurrentDocument()->clearSelection();

	for (QString line : QString (inputbox->toPlainText()).split ("\n"))
	{
		LDObject* obj = ParseLine (line);

		CurrentDocument()->insertObj (idx, obj);
		obj->select();
		idx++;
	}

	m_window->refresh();
	m_window->scrollToSelection();
}

void BasicToolset::setColor()
{
	if (Selection().isEmpty())
		return;

	LDObjectList objs = Selection();

	// If all selected objects have the same color, said color is our default
	// value to the color selection dialog.
	LDColor color;
	LDColor defaultcol = m_window->getSelectedColor();

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
	for (LDObject* obj : Selection())
		obj->invert();
}

void BasicToolset::newSubfile()
{
	AddObjectDialog::staticDialog (OBJ_Subfile, nullptr);
}

void BasicToolset::newLine()
{
	AddObjectDialog::staticDialog (OBJ_Line, nullptr);
}

void BasicToolset::newTriangle()
{
	AddObjectDialog::staticDialog (OBJ_Triangle, nullptr);
}

void BasicToolset::newQuadrilateral()
{
	AddObjectDialog::staticDialog (OBJ_Quad, nullptr);
}

void BasicToolset::newConditionalLine()
{
	AddObjectDialog::staticDialog (OBJ_CondLine, nullptr);
}

void BasicToolset::newComment()
{
	AddObjectDialog::staticDialog (OBJ_Comment, nullptr);
}

void BasicToolset::newBFC()
{
	AddObjectDialog::staticDialog (OBJ_BFC, nullptr);
}

void BasicToolset::edit()
{
	if (Selection().size() != 1)
		return;

	LDObject* obj = Selection().first();
	AddObjectDialog::staticDialog (obj->type(), obj);
}

void BasicToolset::modeSelect()
{
	m_window->R()->setEditMode (EditModeType::Select);
}

void BasicToolset::modeDraw()
{
	m_window->R()->setEditMode (EditModeType::Draw);
}

void BasicToolset::modeRectangle()
{
	m_window->R()->setEditMode (EditModeType::Rectangle);
}

void BasicToolset::modeCircle()
{
	m_window->R()->setEditMode (EditModeType::Circle);
}

void BasicToolset::modeMagicWand()
{
 	m_window->R()->setEditMode (EditModeType::MagicWand);
}

void BasicToolset::modeLinePath()
{
	m_window->R()->setEditMode (EditModeType::LinePath);
}