#include "subfilereferenceeditor.h"
#include "ui_subfilereferenceeditor.h"
#include "../linetypes/modelobject.h"
#include "../primitives.h"
#include "../guiutilities.h"
#include "../dialogs/colorselector.h"

SubfileReferenceEditor::SubfileReferenceEditor(LDSubfileReference* reference, QWidget* parent) :
	QDialog {parent},
	reference {reference},
	ui {*new Ui::SubfileReferenceEditor}
{
	this->ui.setupUi(this);
	this->ui.referenceName->setText(reference->referenceName());
	this->color = reference->color();
	::setColorButton(this->ui.colorButton, this->color);
	this->ui.positionX->setValue(reference->position().x());
	this->ui.positionY->setValue(reference->position().y());
	this->ui.positionZ->setValue(reference->position().z());
	connect(
		this->ui.colorButton,
		&QPushButton::clicked,
		[&]()
		{
			if (ColorSelector::selectColor(this, this->color, this->color))
				::setColorButton(this->ui.colorButton, this->color);
		}
	);
	for (int i : {0, 1, 2})
	for (int j : {0, 1, 2})
	{
		QLayoutItem* item = this->ui.matrixLayout->itemAtPosition(i, j);
		QDoubleSpinBox* spinbox = item ? qobject_cast<QDoubleSpinBox*>(item->widget()) : nullptr;
		spinbox->setValue(reference->transformationMatrix()(i, j));
	}
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

void SubfileReferenceEditor::accept()
{
	this->reference->setReferenceName(this->ui.referenceName->text());
	Matrix transformationMatrix;
	for (int i : {0, 1, 2})
	for (int j : {0, 1, 2})
	{
		QLayoutItem* item = this->ui.matrixLayout->itemAtPosition(i, j);
		QDoubleSpinBox* spinbox = item ? qobject_cast<QDoubleSpinBox*>(item->widget()) : nullptr;
		transformationMatrix(i, j) = spinbox->value();
	}
	this->reference->setTransformationMatrix(transformationMatrix);
	this->reference->setPosition({
		this->ui.positionX->value(),
		this->ui.positionY->value(),
		this->ui.positionZ->value()
	});
	this->reference->setColor(this->color);
	QDialog::accept();
}

void SubfileReferenceEditor::setPrimitivesTree(PrimitiveManager* primitives)
{
	this->ui.primitivesTreeView->setModel(primitives);
}

SubfileReferenceEditor::~SubfileReferenceEditor()
{
	delete &this->ui;
}
