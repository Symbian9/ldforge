/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2016 Teemu Piippo
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

#include <QGridLayout>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QLabel>
#include <QListWidget>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include "mainwindow.h"
#include "addObjectDialog.h"
#include "ldDocument.h"
#include "colors.h"
#include "dialogs/colorselector.h"
#include "editHistory.h"
#include "radioGroup.h"
#include "miscallenous.h"
#include "primitives.h"

// =============================================================================
//
AddObjectDialog::AddObjectDialog (const LDObjectType type, LDObject* obj, QWidget* parent) :
	QDialog (parent)
{
	setlocale (LC_ALL, "C");

	int coordCount = 0;
	QString typeName = LDObject::typeName (type);

	switch (type)
	{
		case OBJ_Comment:
		{
			le_comment = new QLineEdit;

			if (obj)
				le_comment->setText (static_cast<LDComment*> (obj)->text());

			le_comment->setMinimumWidth (384);
		} break;

		case OBJ_Line:
		{
			coordCount = 6;
		} break;

		case OBJ_Triangle:
		{
			coordCount = 9;
		} break;

		case OBJ_Quad:
		case OBJ_CondLine:
		{
			coordCount = 12;
		} break;

		case OBJ_Bfc:
		{
			rb_bfcType = new RadioGroup ("Statement", {}, 0, Qt::Vertical);

			for (BfcStatement statement : iterateEnum<BfcStatement>())
			{
				// Separate these in two columns
				if (int(statement) == EnumLimits<BfcStatement>::Count / 2)
					rb_bfcType->rowBreak();

				rb_bfcType->addButton (LDBfc::statementToString(statement));
			}

			if (obj)
				rb_bfcType->setValue ((int) static_cast<LDBfc*> (obj)->statement());
		} break;

		case OBJ_SubfileReference:
		{
			coordCount = 3;
			tw_subfileList = new QTreeWidget();
			tw_subfileList->setHeaderLabel (tr ("Primitives"));

			QString defaultname;

			if (obj)
				defaultname = static_cast<LDSubfileReference*> (obj)->fileInfo()->name();

			g_win->primitives()->populateTreeWidget(tw_subfileList, defaultname);
			connect (tw_subfileList, SIGNAL (itemSelectionChanged()), this, SLOT (slot_subfileTypeChanged()));
			lb_subfileName = new QLabel ("File:");
			le_subfileName = new QLineEdit;
			le_subfileName->setFocus();

			if (obj)
			{
				LDSubfileReference* ref = static_cast<LDSubfileReference*> (obj);
				le_subfileName->setText (ref->fileInfo()->name());
			}
		} break;

		default:
		{
			Critical (format ("Unhandled LDObject type %1 (%2) in AddObjectDialog", (int) type, typeName));
		} return;
	}

	QPixmap icon = GetIcon (format ("add-%1", typeName));
	LDObject* defaults = LDObject::getDefault (type);

	lb_typeIcon = new QLabel;
	lb_typeIcon->setPixmap (icon);

	// Show a color edit dialog for the types that actually use the color
	if (defaults->isColored())
	{
		if (obj)
			m_color = obj->color();
		else
			m_color = (type == OBJ_CondLine or type == OBJ_Line) ? EdgeColor : MainColor;

		pb_color = new QPushButton;
		setButtonBackground (pb_color, m_color);
		connect (pb_color, SIGNAL (clicked()), this, SLOT (slot_colorButtonClicked()));
	}

	for (int i = 0; i < coordCount; ++i)
	{
		dsb_coords[i] = new QDoubleSpinBox;
		dsb_coords[i]->setDecimals (5);
		dsb_coords[i]->setMinimum (-10000.0);
		dsb_coords[i]->setMaximum (10000.0);
	}

	QGridLayout* const layout = new QGridLayout;
	layout->addWidget (lb_typeIcon, 0, 0);

	switch (type)
	{
		case OBJ_Line:
		case OBJ_CondLine:
		case OBJ_Triangle:
		case OBJ_Quad:
			// Apply coordinates
			if (obj)
			{
				for (int i = 0; i < coordCount / 3; ++i)
				{
					obj->vertex (i).apply ([&](Axis ax, double value)
					{
						dsb_coords[(i * 3) + ax]->setValue (value);
					});
				}
			}
			break;

		case OBJ_Comment:
			layout->addWidget (le_comment, 0, 1);
			break;

		case OBJ_Bfc:
			layout->addWidget (rb_bfcType, 0, 1);
			break;

		case OBJ_SubfileReference:
			layout->addWidget (tw_subfileList, 1, 1, 1, 2);
			layout->addWidget (lb_subfileName, 2, 1);
			layout->addWidget (le_subfileName, 2, 2);
			break;

		default:
			break;
	}

	if (defaults->hasMatrix())
	{
		LDMatrixObject* mo = dynamic_cast<LDMatrixObject*> (obj);
		QLabel* lb_matrix = new QLabel ("Matrix:");
		le_matrix = new QLineEdit;
		// le_matrix->setValidator (new QDoubleValidator);
		Matrix defaultMatrix = Matrix::identity;

		if (mo)
		{
			mo->position().apply ([&](Axis ax, double value)
			{
				dsb_coords[ax]->setValue (value);
			});

			defaultMatrix = mo->transformationMatrix();
		}

		le_matrix->setText (defaultMatrix.toString());
		layout->addWidget (lb_matrix, 4, 1);
		layout->addWidget (le_matrix, 4, 2, 1, 3);
	}

	if (defaults->isColored())
		layout->addWidget (pb_color, 1, 0);

	if (coordCount > 0)
	{
		QGridLayout* const qCoordLayout = new QGridLayout;

		for (int i = 0; i < coordCount; ++i)
			qCoordLayout->addWidget (dsb_coords[i], (i / 3), (i % 3));

		layout->addLayout (qCoordLayout, 0, 1, (coordCount / 3), 3);
	}

	QDialogButtonBox* bbx_buttons = new QDialogButtonBox (QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	QWidget::connect (bbx_buttons, SIGNAL (accepted()), this, SLOT (accept()));
	QWidget::connect (bbx_buttons, SIGNAL (rejected()), this, SLOT (reject()));
	layout->addWidget (bbx_buttons, 5, 0, 1, 4);
	setLayout (layout);
	setWindowTitle (format (tr ("Edit %1"), typeName));
	setWindowIcon (icon);
}

// =============================================================================
// =============================================================================
void AddObjectDialog::setButtonBackground (QPushButton* button, LDColor color)
{
	button->setIcon (GetIcon ("palette"));
	button->setAutoFillBackground (true);

	if (color.isValid())
		button->setStyleSheet (format ("background-color: %1", color.hexcode()));
}

// =============================================================================
// =============================================================================
QString AddObjectDialog::currentSubfileName()
{
	PrimitiveTreeItem* item = static_cast<PrimitiveTreeItem*> (tw_subfileList->currentItem());

	if (item->primitive() == nullptr)
		return ""; // selected a heading

	return item->primitive()->name;
}

// =============================================================================
// =============================================================================
void AddObjectDialog::slot_colorButtonClicked()
{
	ColorSelector::selectColor (this, m_color, m_color);
	setButtonBackground (pb_color, m_color);
}

// =============================================================================
// =============================================================================
void AddObjectDialog::slot_subfileTypeChanged()
{
	QString name = currentSubfileName();

	if (not name.isEmpty())
		le_subfileName->setText (name);
}

// =============================================================================
// =============================================================================
template<typename T>
static T* InitObject (LDObject*& obj)
{
	if (obj == nullptr)
		obj = new T;

	return static_cast<T*> (obj);
}

// =============================================================================
// =============================================================================
#include "documentmanager.h"
void AddObjectDialog::staticDialog (const LDObjectType type, LDObject* obj)
{
	setlocale (LC_ALL, "C");

	// FIXME: Redirect to Edit Raw
	if (obj and obj->type() == OBJ_Error)
		return;

	if (type == OBJ_Empty)
		return; // Nothing to edit with empties

	const bool newObject = (obj == nullptr);
	Matrix transform = Matrix::identity;
	AddObjectDialog dlg (type, obj);

	if (obj and obj->type() != type)
		return;

	if (dlg.exec() == QDialog::Rejected)
		return;

	if (type == OBJ_SubfileReference)
	{
		QStringList stringValues = dlg.le_matrix->text().split (" ", QString::SkipEmptyParts);

		if (countof(stringValues) == 9)
		{
			int i = 0;

			for (QString stringValue : stringValues)
				transform.value(i++) = stringValue.toFloat();
		}
	}

	switch (type)
	{
		case OBJ_Comment:
		{
			LDComment* comm = InitObject<LDComment> (obj);
			comm->setText (dlg.le_comment->text());
		}
		break;

		case OBJ_Line:
		case OBJ_Triangle:
		case OBJ_Quad:
		case OBJ_CondLine:
		{
			if (not obj)
				obj = LDObject::getDefault (type);

			for (int i = 0; i < obj->numVertices(); ++i)
			{
				Vertex v;

				v.apply ([&](Axis ax, double& value)
				{
					value = dlg.dsb_coords[(i * 3) + ax]->value();
				});

				obj->setVertex (i, v);
			}
		} break;

		case OBJ_Bfc:
		{
			LDBfc* bfc = InitObject<LDBfc> (obj);
			int value = dlg.rb_bfcType->value();
			if (valueInEnum<BfcStatement>(value))
				bfc->setStatement(BfcStatement(value));
		} break;

		case OBJ_SubfileReference:
		{
			QString name = dlg.le_subfileName->text();

			if (name.isEmpty())
				return; // no subfile filename

			LDDocument* document = g_win->documents()->getDocumentByName (name);

			if (not document)
			{
				Critical (format ("Couldn't open `%1': %2", name, strerror (errno)));
				return;
			}

			LDSubfileReference* ref = InitObject<LDSubfileReference> (obj);

			for_axes (ax)
				ref->setCoordinate (ax, dlg.dsb_coords[ax]->value());

			ref->setTransformationMatrix (transform);
			ref->setFileInfo (document);
		} break;

		default:
			break;
	}

	if (obj->isColored())
		obj->setColor (dlg.m_color);

	if (newObject)
	{
		int idx = g_win->suggestInsertPoint();
		g_win->currentDocument()->insertObject (idx, obj);
	}

	g_win->refresh();
}
