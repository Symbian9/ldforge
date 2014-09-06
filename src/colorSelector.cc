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
 *  =====================================================================
 *
 *  colorSelectDialog.cxx: Color selector box.
 */

#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QMouseEvent>
#include <QScrollBar>
#include <QColorDialog>
#include "main.h"
#include "mainWindow.h"
#include "colorSelector.h"
#include "colors.h"
#include "configuration.h"
#include "miscallenous.h"
#include "ui_colorsel.h"

static const int g_numColumns = 16;

EXTERN_CFGENTRY (String, MainColor)
EXTERN_CFGENTRY (Float, MainColorAlpha)

// =============================================================================
//
ColorSelector::ColorSelector (LDColor defaultvalue, QWidget* parent) :
	QDialog (parent)
{
	m_firstResize = true;
	ui = new Ui_ColorSelUI;
	ui->setupUi (this);
	setSelection (defaultvalue);

	QGridLayout* layout = new QGridLayout (this);

	// Spawn color selector buttons
	for (int i = 0; i < CountLDConfigColors(); ++i)
	{
		LDColor ldcolor = LDColor::fromIndex (i);
		QPushButton* button = new QPushButton (this);
		button->setMinimumSize (QSize (32, 32));
		button->setMaximumSize (button->minimumSize());

		if (ldcolor != null)
		{
			QString colorname;
			QColor color (ldcolor.faceColor());

			if (i == MainColorIndex)
			{
				color = QColor (cfg::MainColor);
				color.setAlphaF (cfg::MainColorAlpha);
			}

			QString color2name (Luma (color) < 80 ? "white" : "black");
			button->setAutoFillBackground (true);
			button->setStyleSheet (format ("background-color: rgba(%1, %2, %3, %4); color: %5",
				color.red(), color.green(), color.blue(), color.alpha(), color2name));
			button->setCheckable (true);
			button->setText (QString::number (ldcolor.index()));
			button->setToolTip (format ("%1: %2", ldcolor.index(), ldcolor.name()));
			m_buttons[i] = button;
			m_buttonsReversed[button] = i;
			connect (button, SIGNAL (clicked(bool)), this, SLOT (colorButtonClicked()));

			if (ldcolor == selection())
				button->setChecked (true);
		}
		else
		{
			button->setEnabled (false);
		}

		layout->addWidget (button, i / g_numColumns, i % g_numColumns);
	}

	QWidget* widget = new QWidget();
	widget->setLayout (layout);
	ui->definedColors->setWidget (widget);
	connect (ui->directColor, SIGNAL (clicked (bool)), this, SLOT (chooseDirectColor()));

	ui->definedColors->setMinimumWidth (ui->definedColors->widget()->width() + 16);

#ifdef TRANSPARENT_DIRECT_COLORS
	connect (ui->transparentDirectColor, SIGNAL (clicked (bool)), this, SLOT (transparentCheckboxClicked()));
#else
	ui->transparentDirectColor->hide();
#endif

	drawColorInfo();
}

// =============================================================================
//
ColorSelector::~ColorSelector()
{
	delete ui;
}

// =============================================================================
//
void ColorSelector::colorButtonClicked()
{
	QPushButton* button = qobject_cast<QPushButton*> (sender());
	auto it = m_buttonsReversed.find (button);
	LDColor color;

	if (Q_UNLIKELY (button == null or it == m_buttonsReversed.end()
		or (color = LDColor::fromIndex (*it)) == null))
	{
		print ("colorButtonClicked() called with invalid sender");
		return;
	}

	if (selection() != null)
	{
		auto it2 = m_buttons.find (selection().index());

		if (it2 != m_buttons.end())
			(*it2)->setChecked (false);
	}

	setSelection (color);
	button->setChecked (true);
	drawColorInfo();
}

// =============================================================================
//
void ColorSelector::drawColorInfo()
{
	if (selection() == null)
	{
		ui->colorLabel->setText ("---");
		ui->iconLabel->setPixmap (QPixmap());
		ui->transparentDirectColor->setChecked (false);
		return;
	}

	ui->colorLabel->setText (format ("%1 - %2", selection().indexString(),
		(selection().isDirect() ? "<direct color>" : selection().name())));
	ui->iconLabel->setPixmap (MakeColorIcon (selection(), 16).pixmap (16, 16));

#ifdef TRANSPARENT_DIRECT_COLORS
	ui->transparentDirectColor->setEnabled (selection().isDirect());
	ui->transparentDirectColor->setChecked (selection().isDirect() and selection().faceColor().alphaF() < 1.0);
#else
	ui->transparentDirectColor->setChecked (false);
	ui->transparentDirectColor->setEnabled (false);
#endif
}

// =============================================================================
//
void ColorSelector::selectDirectColor (QColor col)
{
	int32 idx = (ui->transparentDirectColor->isChecked() ? 0x03000000 : 0x02000000);
	idx |= (col.red() << 16) | (col.green() << 8) | (col.blue());
	setSelection (LDColor::fromIndex (idx));
	drawColorInfo();
}

// =============================================================================
//
void ColorSelector::chooseDirectColor()
{
	QColor defcolor = selection() != null ? selection().faceColor() : Qt::white;
	QColor newcolor = QColorDialog::getColor (defcolor);

	if (not newcolor.isValid())
		return; // canceled

	selectDirectColor (newcolor);
}

// =============================================================================
//
void ColorSelector::transparentCheckboxClicked()
{
	if (selection() == null or not selection().isDirect())
		return;

	selectDirectColor (selection().faceColor());
}

// =============================================================================
//
bool ColorSelector::selectColor (LDColor& val, LDColor defval, QWidget* parent)
{
	ColorSelector dlg (defval, parent);

	if (dlg.exec() and dlg.selection() != null)
	{
		val = dlg.selection();
		return true;
	}

	return false;
}
