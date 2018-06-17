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

#include "subfilereferenceeditor.h"
#include "ui_subfilereferenceeditor.h"
#include "../linetypes/modelobject.h"
#include "../primitives.h"
#include "../guiutilities.h"
#include "../dialogs/colorselector.h"

SubfileReferenceEditor::SubfileReferenceEditor(LDSubfileReference* reference, QWidget* parent) :
	QDialog {parent},
	ui {*new Ui::SubfileReferenceEditor},
	reference {reference}
{
	this->ui.setupUi(this);
	this->ui.referenceName->setText(reference->referenceName());
	this->ui.matrixEditor->setMatrix(reference->transformationMatrix());
	this->color = reference->color();
	::setColorButton(this->ui.colorButton, this->color);
	connect(
		this->ui.colorButton,
		&QPushButton::clicked,
		[&]()
		{
			if (ColorSelector::selectColor(this, this->color, this->color))
				::setColorButton(this->ui.colorButton, this->color);
		}
	);
	connect(
		this->ui.primitivesTreeView,
		&QTreeView::clicked,
		[&](const QModelIndex& index)
		{
			QAbstractItemModel* model = this->ui.primitivesTreeView->model();
			QVariant primitiveName = model->data(index, PrimitiveManager::PrimitiveNameRole);

			if (primitiveName.isValid())
				this->ui.referenceName->setText(primitiveName.toString());
		}
	);
}

SubfileReferenceEditor::~SubfileReferenceEditor()
{
	delete &this->ui;
}

void SubfileReferenceEditor::accept()
{
	this->reference->setReferenceName(this->ui.referenceName->text());
	this->reference->setColor(this->color);
	this->reference->setTransformationMatrix(this->ui.matrixEditor->matrix());
	QDialog::accept();
}

void SubfileReferenceEditor::setPrimitivesTree(PrimitiveManager* primitives)
{
	this->ui.primitivesTreeView->setModel(primitives);
}
