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

#include <QFileDialog>
#include <QInputDialog>
#include "../mainwindow.h"
#include "../ldDocument.h"
#include "../miscallenous.h"
#include "../glRenderer.h"
#include "../primitives.h"
#include "../colors.h"
#include "../dialogs.h"
#include "../glCompiler.h"
#include "viewtoolset.h"

EXTERN_CFGENTRY (Bool, DrawWireframe)
EXTERN_CFGENTRY (Bool, BFCRedGreenView)
EXTERN_CFGENTRY (Bool, DrawAngles)
EXTERN_CFGENTRY (Bool, RandomColors)
EXTERN_CFGENTRY (Bool, DrawSurfaces)
EXTERN_CFGENTRY (Bool, DrawEdgeLines)
EXTERN_CFGENTRY (Bool, DrawConditionalLines)
EXTERN_CFGENTRY (Bool, DrawAxes)

ViewToolset::ViewToolset (MainWindow *parent) :
	Toolset (parent) {}

void ViewToolset::selectAll()
{
	for (LDObject* obj : CurrentDocument()->objects())
		obj->select();
}

void ViewToolset::selectByColor()
{
	if (Selection().isEmpty())
		return;

	QList<LDColor> colors;

	for (LDObject* obj : Selection())
	{
		if (obj->isColored())
			colors << obj->color();
	}

	removeDuplicates (colors);
	CurrentDocument()->clearSelection();

	for (LDObject* obj : CurrentDocument()->objects())
	{
		if (colors.contains (obj->color()))
			obj->select();
	}
}

void ViewToolset::selectByType()
{
	if (Selection().isEmpty())
		return;

	QList<LDObjectType> types;
	QStringList subfilenames;

	for (LDObject* obj : Selection())
	{
		types << obj->type();

		if (types.last() == OBJ_Subfile)
			subfilenames << static_cast<LDSubfile*> (obj)->fileInfo()->name();
	}

	removeDuplicates (types);
	removeDuplicates (subfilenames);
	CurrentDocument()->clearSelection();

	for (LDObject* obj : CurrentDocument()->objects())
	{
		LDObjectType type = obj->type();

		if (not types.contains (type))
			continue;

		// For subfiles, type check is not enough, we check the name of the document as well.
		if (type == OBJ_Subfile and not subfilenames.contains (static_cast<LDSubfile*> (obj)->fileInfo()->name()))
			continue;

		obj->select();
	}
}

void ViewToolset::resetView()
{
	m_window->R()->resetAngles();
	m_window->R()->update();
}

void ViewToolset::screenshot()
{
	setlocale (LC_ALL, "C");

	int w, h;
	uchar* imgdata = m_window->R()->getScreencap (w, h);
	QImage img = GetImageFromScreencap (imgdata, w, h);

	QString root = Basename (CurrentDocument()->name());

	if (root.right (4) == ".dat")
		root.chop (4);

	QString defaultname = (root.length() > 0) ? format ("%1.png", root) : "";
	QString fname = QFileDialog::getSaveFileName (m_window, "Save Screencap", defaultname,
				"PNG images (*.png);;JPG images (*.jpg);;BMP images (*.bmp);;All Files (*.*)");

	if (not fname.isEmpty() and not img.save (fname))
		Critical (format ("Couldn't open %1 for writing to save screencap: %2", fname, strerror (errno)));

	delete[] imgdata;
}

void ViewToolset::axes()
{
	cfg::DrawAxes = not cfg::DrawAxes;
	m_window->updateActions();
	m_window->R()->update();
}

void ViewToolset::visibilityToggle()
{
	for (LDObject* obj : Selection())
		obj->setHidden (not obj->isHidden());
}

void ViewToolset::visibilityHide()
{
	for (LDObject* obj : Selection())
		obj->setHidden (true);
}

void ViewToolset::visibilityReveal()
{
	for (LDObject* obj : Selection())
		obj->setHidden (false);
}

void ViewToolset::wireframe()
{
	cfg::DrawWireframe = not cfg::DrawWireframe;
	m_window->R()->refresh();
}

void ViewToolset::setOverlay()
{
	OverlayDialog dlg;

	if (not dlg.exec())
		return;

	m_window->R()->setupOverlay ((ECamera) dlg.camera(), dlg.fpath(), dlg.ofsx(),
		dlg.ofsy(), dlg.lwidth(), dlg.lheight());
}

void ViewToolset::clearOverlay()
{
	m_window->R()->clearOverlay();
}

void ViewToolset::drawAngles()
{
	cfg::DrawAngles = not cfg::DrawAngles;
	m_window->R()->refresh();
}

void ViewToolset::setDrawDepth()
{
	if (m_window->R()->camera() == EFreeCamera)
		return;

	bool ok;
	double depth = QInputDialog::getDouble (m_window, "Set Draw Depth",
		format ("Depth value for %1 Camera:", m_window->R()->getCameraName()),
		m_window->R()->getDepthValue(), -10000.0f, 10000.0f, 3, &ok);

	if (ok)
		m_window->R()->setDepthValue (depth);
}

#if 0
// This is a test to draw a dummy axle. Meant to be used as a primitive gallery,
// but I can't figure how to generate these pictures properly. Multi-threading
// these is an immense pain.
void ViewToolset::testpic()
{
	LDDocument* file = getFile ("axle.dat");
	setlocale (LC_ALL, "C");

	if (not file)
	{
		critical ("couldn't load axle.dat");
		return;
	}

	int w, h;

	GLRenderer* rend = new GLRenderer;
	rend->resize (64, 64);
	rend->setAttribute (Qt::WA_DontShowOnScreen);
	rend->show();
	rend->setFile (file);
	rend->setDrawOnly (true);
	rend->compileAllObjects();
	rend->initGLData();
	rend->drawGLScene();

	uchar* imgdata = rend->screencap (w, h);
	QImage img = imageFromScreencap (imgdata, w, h);

	if (img.isNull())
	{
		critical ("Failed to create the image!\n");
	}
	else
	{
		QLabel* label = new QLabel;
		QDialog* dlg = new QDialog;
		label->setPixmap (QPixmap::fromImage (img));
		QVBoxLayout* layout = new QVBoxLayout (dlg);
		layout->addWidget (label);
		dlg->exec();
	}

	delete[] imgdata;
	rend->deleteLater();
}
#endif

void ViewToolset::bfcView()
{
	cfg::BFCRedGreenView = not cfg::BFCRedGreenView;

	if (cfg::BFCRedGreenView)
		cfg::RandomColors = false;

	m_window->updateActions();
	m_window->R()->refresh();
}

void ViewToolset::jumpTo()
{
	bool ok;
	int defval = 0;
	LDObject* obj;

	if (Selection().size() == 1)
		defval = Selection()[0]->lineNumber();

	int idx = QInputDialog::getInt (null, "Go to line", "Go to line:", defval,
		1, CurrentDocument()->getObjectCount(), 1, &ok);

	if (not ok or (obj = CurrentDocument()->getObject (idx - 1)) == null)
		return;

	CurrentDocument()->clearSelection();
	obj->select();
	m_window->updateSelection();
}

void ViewToolset::randomColors()
{
	cfg::RandomColors = not cfg::RandomColors;

	if (cfg::RandomColors)
		cfg::BFCRedGreenView = false;

	m_window->updateActions();
	m_window->R()->refresh();
}

void ViewToolset::drawSurfaces()
{
	cfg::DrawSurfaces = not cfg::DrawSurfaces;
	m_window->updateActions();
}

void ViewToolset::drawEdgeLines()
{
	cfg::DrawEdgeLines = not cfg::DrawEdgeLines;
	m_window->updateActions();
}

void ViewToolset::drawConditionalLines()
{
	cfg::DrawConditionalLines = not cfg::DrawConditionalLines;
	m_window->updateActions();
}