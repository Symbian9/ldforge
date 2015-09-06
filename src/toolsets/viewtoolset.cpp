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

ViewToolset::ViewToolset (MainWindow *parent) :
	Toolset (parent) {}

void ViewToolset::selectAll()
{
	for (LDObject* obj : currentDocument()->objects())
		obj->select();
}

void ViewToolset::selectByColor()
{
	if (selectedObjects().isEmpty())
		return;

	QList<LDColor> colors;

	for (LDObject* obj : selectedObjects())
	{
		if (obj->isColored())
			colors << obj->color();
	}

	removeDuplicates (colors);
	currentDocument()->clearSelection();

	for (LDObject* obj : currentDocument()->objects())
	{
		if (colors.contains (obj->color()))
			obj->select();
	}
}

void ViewToolset::selectByType()
{
	if (selectedObjects().isEmpty())
		return;

	QList<LDObjectType> types;
	QStringList subfilenames;

	for (LDObject* obj : selectedObjects())
	{
		types << obj->type();

		if (types.last() == OBJ_Subfile)
			subfilenames << static_cast<LDSubfile*> (obj)->fileInfo()->name();
	}

	removeDuplicates (types);
	removeDuplicates (subfilenames);
	currentDocument()->clearSelection();

	for (LDObject* obj : currentDocument()->objects())
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
	m_window->renderer()->resetAngles();
	m_window->renderer()->update();
}

void ViewToolset::screenshot()
{
	setlocale (LC_ALL, "C");

	int w, h;
	uchar* imgdata = m_window->renderer()->getScreencap (w, h);
	QImage img = GetImageFromScreencap (imgdata, w, h);

	QString root = Basename (currentDocument()->name());

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
	m_config->setDrawAxes (not m_config->drawAxes());
	m_window->updateActions();
	m_window->renderer()->update();
}

void ViewToolset::visibilityToggle()
{
	for (LDObject* obj : selectedObjects())
		obj->setHidden (not obj->isHidden());
}

void ViewToolset::visibilityHide()
{
	for (LDObject* obj : selectedObjects())
		obj->setHidden (true);
}

void ViewToolset::visibilityReveal()
{
	for (LDObject* obj : selectedObjects())
		obj->setHidden (false);
}

void ViewToolset::wireframe()
{
	m_config->setDrawWireframe (not m_config->drawWireframe());
	m_window->renderer()->refresh();
}

void ViewToolset::setOverlay()
{
	OverlayDialog dlg;

	if (not dlg.exec())
		return;

	m_window->renderer()->setupOverlay ((ECamera) dlg.camera(), dlg.fpath(), dlg.ofsx(),
		dlg.ofsy(), dlg.lwidth(), dlg.lheight());
}

void ViewToolset::clearOverlay()
{
	m_window->renderer()->clearOverlay();
}

void ViewToolset::drawAngles()
{
	m_config->setDrawAngles (not m_config->drawAngles());
	m_window->renderer()->refresh();
}

void ViewToolset::setDrawDepth()
{
	if (m_window->renderer()->camera() == EFreeCamera)
		return;

	bool ok;
	double depth = QInputDialog::getDouble (m_window, "Set Draw Depth",
		format ("Depth value for %1 Camera:", m_window->renderer()->getCameraName()),
		m_window->renderer()->getDepthValue(), -10000.0f, 10000.0f, 3, &ok);

	if (ok)
		m_window->renderer()->setDepthValue (depth);
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
	m_config->setBfcRedGreenView (not m_config->bfcRedGreenView());

	if (m_config->bfcRedGreenView())
		m_config->setRandomColors (false);

	m_window->updateActions();
	m_window->renderer()->refresh();
}

void ViewToolset::jumpTo()
{
	bool ok;
	int defval = 0;
	LDObject* obj;

	if (selectedObjects().size() == 1)
		defval = selectedObjects()[0]->lineNumber();

	int idx = QInputDialog::getInt (nullptr, "Go to line", "Go to line:", defval,
		1, currentDocument()->getObjectCount(), 1, &ok);

	if (not ok or (obj = currentDocument()->getObject (idx - 1)) == nullptr)
		return;

	currentDocument()->clearSelection();
	obj->select();
	m_window->updateSelection();
}

void ViewToolset::randomColors()
{
	m_config->setRandomColors (not m_config->randomColors());

	if (m_config->randomColors())
		m_config->setBfcRedGreenView (false);

	m_window->updateActions();
	m_window->renderer()->refresh();
}

void ViewToolset::drawSurfaces()
{
	m_config->setDrawSurfaces (not m_config->drawSurfaces());
	m_window->updateActions();
}

void ViewToolset::drawEdgeLines()
{
	m_config->setDrawEdgeLines (not m_config->drawEdgeLines());
	m_window->updateActions();
}

void ViewToolset::drawConditionalLines()
{
	m_config->setDrawConditionalLines (not m_config->drawConditionalLines());
	m_window->updateActions();
}