/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 Santeri `arezey` Piippo
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

#include "common.h"
#include "gui.h"
#include <qgraphicsscene.h>
#include <qgraphicsview.h>
#include <qicon.h>
#include <qboxlayout.h>
#include <qgraphicsitem.h>
#include "zz_colorSelectDialog.h"
#include "colors.h"

static const short g_dNumColumns = 8;
static const short g_dNumRows = 10;
static const short g_dSquareSize = 32;
static const long g_lWidth = (g_dNumColumns * g_dSquareSize);
static const long g_lHeight = (g_dNumRows * g_dSquareSize);
static const long g_lMaxHeight = ((MAX_COLORS / g_dNumColumns) * g_dSquareSize);

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ColorSelectDialog::ColorSelectDialog (QWidget* parent) {
	qScene = new QGraphicsScene;
	qView = new QGraphicsView (qScene);
	
	// not really an icon but eh
	qScene->setBackgroundBrush (QPixmap ("icons/checkerboard.png"));
	
	qScene->setSceneRect (0, 0, g_lWidth, g_lMaxHeight);
	qView->setSceneRect (0, 0, g_lWidth, g_lMaxHeight);
	
	const double fPenWidth = 1.0f;
	QPen qPen (Qt::black, fPenWidth, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
	
	// Draw the color rectangles.
	for (ulong i = 0; i < MAX_COLORS; ++i) {
		color* meta = g_LDColors[i];
		if (!meta)
			continue;
		
		const double x = (i % g_dNumColumns) * g_dSquareSize;
		const double y = (i / g_dNumColumns) * g_dSquareSize;
		const double w = (g_dSquareSize) - (fPenWidth / 2);
		
		QColor qColor (meta->zColor.chars ());
		qColor.setAlpha (meta->fAlpha * 255.0);
		
		uchar ucLuma = (0.2126f * qColor.red()) +
			(0.7152f * qColor.green()) + (0.0722 * qColor.blue());
		bool bDark = (ucLuma < 80);
		
		qScene->addRect (x, y, w, w, qPen, qColor);
		QGraphicsTextItem* qText = qScene->addText (str::mkfmt ("%lu", i).chars());
		qText->setDefaultTextColor ((bDark) ? Qt::white : Qt::black);
		qText->setPos (x, y);
	}
	
	IMPLEMENT_DIALOG_BUTTONS
	
	// Set the size of the view
	const long lWidth = g_lWidth + 21; // HACK
	qView->setMaximumWidth (lWidth);
	qView->setMinimumWidth (lWidth);
	qView->setMaximumHeight (g_lHeight);
	qView->setMinimumHeight (g_lHeight);
	
	QVBoxLayout* qLayout = new QVBoxLayout;
	qLayout->addWidget (qView);
	qLayout->addWidget (qButtons);
	setLayout (qLayout);
	
	setWindowIcon (QIcon ("icons/palette.png"));
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
bool ColorSelectDialog::staticDialog (short& dValue, QWidget* parent) {
	ColorSelectDialog dlg;
	
	if (dlg.exec ()) {
		dValue = dValue;
		return true;
	}
	
	return false;
}