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

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include "../mainwindow.h"
#include "../lddocument.h"
#include "../canvas.h"
#include "../primitives.h"
#include "../colors.h"
#include "../glcompiler.h"
#include "../documentmanager.h"
#include "viewtoolset.h"

ViewToolset::ViewToolset (MainWindow *parent) :
	Toolset (parent) {}

void ViewToolset::selectAll()
{
	if (currentDocument()->size() >= 1)
	{
		QModelIndex top = currentDocument()->index(0);
		QModelIndex bottom = currentDocument()->index(currentDocument()->size() - 1);
		QItemSelection selection {top, bottom};
		mainWindow()->replaceSelection(selection);
	}
}

void ViewToolset::selectByColor()
{
	if (selectedObjects().isEmpty())
		return;

	QSet<LDColor> colors;

	for (LDObject* obj : selectedObjects())
	{
		if (obj->isColored())
			colors << obj->color();
	}

	QItemSelection selection;

	for (QModelIndex index : currentDocument()->indices())
	{
		if (colors.contains(currentDocument()->lookup(index)->color()))
			selection.select(index, index);
	}

	mainWindow()->replaceSelection(selection);
}

void ViewToolset::selectByType()
{
	if (selectedObjects().isEmpty())
		return;

	QSet<LDObjectType> types;
	QSet<QString> subfilenames;

	for (LDObject* obj : selectedObjects())
	{
		types << obj->type();

		if (obj->type() == LDObjectType::SubfileReference)
			subfilenames << static_cast<LDSubfileReference*>(obj)->fileInfo(m_documents)->name();
	}

	QItemSelection selection;

	for (QModelIndex index : currentDocument()->indices())
	{
		LDObject* obj = currentDocument()->lookup(index);
		LDObjectType type = obj->type();

		if (not types.contains (type))
			continue;

		// For subfiles, type check is not enough, we check the name of the document as well.
		if (type == LDObjectType::SubfileReference
			and not subfilenames.contains(static_cast<LDSubfileReference*>(obj)->fileInfo(m_documents)->name()))
		{
			continue;
		}

		selection.select(index, index);
	}

	mainWindow()->replaceSelection(selection);
}

void ViewToolset::resetView()
{
	m_window->renderer()->resetAngles();
	m_window->renderer()->update();
}

void ViewToolset::screenshot()
{
	const char* imageFormats = "PNG images (*.png);;JPG images (*.jpg);;BMP images (*.bmp);;"
	    "PPM images (*.ppm);;X11 Bitmaps (*.xbm);;X11 Pixmaps (*.xpm);;All Files (*.*)";
	QImage image = m_window->renderer()->screenCapture();
	QString root = QFileInfo {currentDocument()->name()}.fileName();

	if (root.right (4) == ".dat")
		root.chop (4);

	QString defaultname = (not root.isEmpty()) ? format ("%1.png", root) : "";
	QString filename = QFileDialog::getSaveFileName (m_window, "Save Screencap", defaultname, imageFormats);

	if (not filename.isEmpty() and not image.save (filename))
	{
		QString message = format(tr("Couldn't open %1 for writing to save screen capture: %2"), filename, strerror(errno));
		QMessageBox::critical(m_window, tr("Error"), message);
	}
}

void ViewToolset::axes()
{
	config::toggleDrawAxes();
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
	config::toggleDrawWireframe();
	m_window->renderer()->update();
}

void ViewToolset::drawAngles()
{
	config::toggleDrawAngles();
	m_window->renderer()->update();
}

void ViewToolset::setDrawDepth()
{
	if (m_window->renderer()->camera() == Camera::Free)
		return;

	bool ok;
	double depth = QInputDialog::getDouble(
		m_window,
		tr("Set draw depth"),
		format(
			tr("Depth value for %1:"),
			m_window->renderer()->currentCamera().name()
		),
		m_window->renderer()->getDepthValue(),
		-10000.0f,
		10000.0f,
		4,
		&ok
	);

	if (ok)
		m_window->renderer()->setDepthValue(depth);
}

void ViewToolset::setCullDepth()
{
	if (m_window->renderer()->camera() == Camera::Free)
		return;

	bool ok;
	double depth = QInputDialog::getDouble(
		m_window,
		tr("Set cull value"),
		format(
			tr("Cull depth for %1:\nPolygons closer than at this depth are not shown."),
			m_window->renderer()->currentCamera().name()
		),
		m_window->renderer()->currentCullValue(),
		-GLRenderer::far,
		GLRenderer::far,
		4,
		&ok
	);

	if (ok)
		m_window->renderer()->setCullValue(depth);
}

void ViewToolset::clearCullDepth()
{
	m_window->renderer()->clearCurrentCullValue();
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
	config::toggleBfcRedGreenView();

	if (config::bfcRedGreenView())
		config::setRandomColors (false);

	m_window->updateActions();
	m_window->renderer()->update();
}

void ViewToolset::jumpTo()
{
	bool ok;
	int defaultValue = 0;

	if (countof(m_window->selectedIndexes()) == 1)
		defaultValue = (*m_window->selectedIndexes().begin()).row();

	int row = QInputDialog::getInt(
		nullptr, /* parent */
		tr("Go to line"), /* title */
		tr("Go to line:"), /* caption */
		defaultValue, /* default value */
		1, /* minimum value */
		currentDocument()->size(), /* maximum value */
		1, /* step value */
		&ok /* success pointer */
	);

	if (ok)
	{
		QModelIndex object = currentDocument()->index(row - 1);

		if (object.isValid() and object.row() < currentDocument()->size())
		{
			mainWindow()->clearSelection();
			mainWindow()->select(object);
		}
	}
}

void ViewToolset::randomColors()
{
	config::toggleRandomColors();

	if (config::randomColors())
		config::setBfcRedGreenView (false);

	m_window->updateActions();
	m_window->renderer()->update();
}

void ViewToolset::drawSurfaces()
{
	config::toggleDrawSurfaces();
	m_window->updateActions();
}

void ViewToolset::drawEdgeLines()
{
	config::toggleDrawEdgeLines();
	m_window->updateActions();
}

void ViewToolset::drawConditionalLines()
{
	config::toggleDrawConditionalLines();
	m_window->updateActions();
}

void ViewToolset::lighting()
{
	config::toggleLighting();
	m_window->updateActions();
}
