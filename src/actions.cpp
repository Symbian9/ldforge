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
#include <QMessageBox>
#include <QTextEdit>
#include <QBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QInputDialog>

#include "mainWindow.h"
#include "ldDocument.h"
#include "editHistory.h"
#include "configDialog.h"
#include "addObjectDialog.h"
#include "miscallenous.h"
#include "glRenderer.h"
#include "dialogs.h"
#include "primitives.h"
#include "radioGroup.h"
#include "colors.h"
#include "glCompiler.h"
#include "ui_newpart.h"
#include "editmodes/abstractEditMode.h"

EXTERN_CFGENTRY (Bool, DrawWireframe)
EXTERN_CFGENTRY (Bool, BFCRedGreenView)
EXTERN_CFGENTRY (String, DefaultName)
EXTERN_CFGENTRY (String, DefaultUser)
EXTERN_CFGENTRY (Bool, UseCALicense)
EXTERN_CFGENTRY (Bool, DrawAngles)
EXTERN_CFGENTRY (Bool, RandomColors)
EXTERN_CFGENTRY (Bool, DrawSurfaces)
EXTERN_CFGENTRY (Bool, DrawEdgeLines)
EXTERN_CFGENTRY (Bool, DrawConditionalLines)
EXTERN_CFGENTRY (Bool, DrawAxes)

// =============================================================================
//
void MainWindow::actionNew()
{
	QDialog* dlg = new QDialog (g_win);
	Ui::NewPartUI ui;
	ui.setupUi (dlg);

	QString authortext = cfg::DefaultName;

	if (not cfg::DefaultUser.isEmpty())
		authortext.append (format (" [%1]", cfg::DefaultUser));

	ui.le_author->setText (authortext);
	ui.caLicense->setChecked (cfg::UseCALicense);

	if (dlg->exec() == QDialog::Rejected)
		return;

	newFile();

	BFCStatement const bfctype = ui.rb_bfc_ccw->isChecked() ? BFCStatement::CertifyCCW
							   : ui.rb_bfc_cw->isChecked()  ? BFCStatement::CertifyCW
							   : BFCStatement::NoCertify;
	QString const license = ui.caLicense->isChecked() ? CALicenseText : "";

	LDObjectList objs;
	objs << LDSpawn<LDComment> (ui.le_title->text());
	objs << LDSpawn<LDComment> ("Name: <untitled>.dat");
	objs << LDSpawn<LDComment> (format ("Author: %1", ui.le_author->text()));
	objs << LDSpawn<LDComment> (format ("!LDRAW_ORG Unofficial_Part"));

	if (not license.isEmpty())
		objs << LDSpawn<LDComment> (license);

	objs << LDSpawn<LDEmpty>();
	objs << LDSpawn<LDBFC> (bfctype);
	objs << LDSpawn<LDEmpty>();
	CurrentDocument()->addObjects (objs);
	doFullRefresh();
}

// =============================================================================
//
void MainWindow::actionNewFile()
{
	newFile();
}

// =============================================================================
//
void MainWindow::actionOpen()
{
	QString name = QFileDialog::getOpenFileName (g_win, "Open File", "", "LDraw files (*.dat *.ldr)");

	if (name.isEmpty())
		return;

	OpenMainModel (name);
}

// =============================================================================
//
void MainWindow::actionSave()
{
	save (CurrentDocument(), false);
}

// =============================================================================
//
void MainWindow::actionSaveAs()
{
	save (CurrentDocument(), true);
}

// =============================================================================
//
void MainWindow::actionSaveAll()
{
	for (LDDocumentPtr file : LDDocument::explicitDocuments())
		save (file, false);
}

// =============================================================================
//
void MainWindow::actionClose()
{
	if (not CurrentDocument()->isSafeToClose())
		return;

	CurrentDocument()->dismiss();
}

// =============================================================================
//
void MainWindow::actionCloseAll()
{
	if (not IsSafeToCloseAll())
		return;

	CloseAllDocuments();
}

// =============================================================================
//
void MainWindow::actionSettings()
{
	(new ConfigDialog)->exec();
}

// =============================================================================
//
void MainWindow::actionSetLDrawPath()
{
	(new LDrawPathDialog (true))->exec();
}

// =============================================================================
//
void MainWindow::actionExit()
{
	Exit();
}

// =============================================================================
//
void MainWindow::actionNewSubfile()
{
	AddObjectDialog::staticDialog (OBJ_Subfile, LDObjectPtr());
}

// =============================================================================
//
void MainWindow::actionNewLine()
{
	AddObjectDialog::staticDialog (OBJ_Line, LDObjectPtr());
}

// =============================================================================
//
void MainWindow::actionNewTriangle()
{
	AddObjectDialog::staticDialog (OBJ_Triangle, LDObjectPtr());
}

// =============================================================================
//
void MainWindow::actionNewQuad()
{
	AddObjectDialog::staticDialog (OBJ_Quad, LDObjectPtr());
}

// =============================================================================
//
void MainWindow::actionNewCLine()
{
	AddObjectDialog::staticDialog (OBJ_CondLine, LDObjectPtr());
}

// =============================================================================
//
void MainWindow::actionNewComment()
{
	AddObjectDialog::staticDialog (OBJ_Comment, LDObjectPtr());
}

// =============================================================================
//
void MainWindow::actionNewBFC()
{
	AddObjectDialog::staticDialog (OBJ_BFC, LDObjectPtr());
}

// =============================================================================
//
void MainWindow::actionEdit()
{
	if (Selection().size() != 1)
		return;

	LDObjectPtr obj = Selection() [0];
	AddObjectDialog::staticDialog (obj->type(), obj);
}

// =============================================================================
//
void MainWindow::actionHelp()
{
}

// =============================================================================
//
void MainWindow::actionAbout()
{
	AboutDialog().exec();
}

// =============================================================================
//
void MainWindow::actionAboutQt()
{
	QMessageBox::aboutQt (g_win);
}

// =============================================================================
//
void MainWindow::actionSelectAll()
{
	for (LDObjectPtr obj : CurrentDocument()->objects())
		obj->select();
}

// =============================================================================
//
void MainWindow::actionSelectByColor()
{
	if (Selection().isEmpty())
		return;

	QList<LDColor> colors;

	for (LDObjectPtr obj : Selection())
	{
		if (obj->isColored())
			colors << obj->color();
	}

	RemoveDuplicates (colors);
	CurrentDocument()->clearSelection();

	for (LDObjectPtr obj : CurrentDocument()->objects())
	{
		if (colors.contains (obj->color()))
			obj->select();
	}
}

// =============================================================================
//
void MainWindow::actionSelectByType()
{
	if (Selection().isEmpty())
		return;

	QList<LDObjectType> types;
	QStringList subfilenames;

	for (LDObjectPtr obj : Selection())
	{
		types << obj->type();

		if (types.last() == OBJ_Subfile)
			subfilenames << obj.staticCast<LDSubfile>()->fileInfo()->name();
	}

	RemoveDuplicates (types);
	RemoveDuplicates (subfilenames);
	CurrentDocument()->clearSelection();

	for (LDObjectPtr obj : CurrentDocument()->objects())
	{
		LDObjectType type = obj->type();

		if (not types.contains (type))
			continue;

		// For subfiles, type check is not enough, we check the name of the document as well.
		if (type == OBJ_Subfile and not subfilenames.contains (obj.staticCast<LDSubfile>()->fileInfo()->name()))
			continue;

		obj->select();
	}
}

// =============================================================================
//
void MainWindow::actionGridCoarse()
{
	cfg::Grid = Grid::Coarse;
	updateGridToolBar();
}

void MainWindow::actionGridMedium()
{
	cfg::Grid = Grid::Medium;
	updateGridToolBar();
}

void MainWindow::actionGridFine()
{
	cfg::Grid = Grid::Fine;
	updateGridToolBar();
}

// =============================================================================
//
void MainWindow::actionResetView()
{
	R()->resetAngles();
	R()->update();
}

// =============================================================================
//
void MainWindow::actionInsertFrom()
{
	QString fname = QFileDialog::getOpenFileName();
	int idx = getInsertionPoint();

	if (not fname.length())
		return;

	QFile f (fname);

	if (not f.open (QIODevice::ReadOnly))
	{
		Critical (format ("Couldn't open %1 (%2)", fname, f.errorString()));
		return;
	}

	LDObjectList objs = LoadFileContents (&f, null);

	CurrentDocument()->clearSelection();

	for (LDObjectPtr obj : objs)
	{
		CurrentDocument()->insertObj (idx, obj);
		obj->select();
		R()->compileObject (obj);

		idx++;
	}

	refresh();
	scrollToSelection();
}

// =============================================================================
//
void MainWindow::actionExportTo()
{
	if (Selection().isEmpty())
		return;

	QString fname = QFileDialog::getSaveFileName();

	if (fname.length() == 0)
		return;

	QFile file (fname);

	if (not file.open (QIODevice::WriteOnly | QIODevice::Text))
	{
		Critical (format ("Unable to open %1 for writing (%2)", fname, file.errorString()));
		return;
	}

	for (LDObjectPtr obj : Selection())
	{
		QString contents = obj->asText();
		QByteArray data = contents.toUtf8();
		file.write (data, data.size());
		file.write ("\r\n", 2);
	}
}

// =============================================================================
//
void MainWindow::actionInsertRaw()
{
	int idx = getInsertionPoint();

	QDialog* const dlg = new QDialog;
	QVBoxLayout* const layout = new QVBoxLayout;
	QTextEdit* const te_edit = new QTextEdit;
	QDialogButtonBox* const bbx_buttons = new QDialogButtonBox (QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

	layout->addWidget (te_edit);
	layout->addWidget (bbx_buttons);
	dlg->setLayout (layout);
	dlg->setWindowTitle (APPNAME " - Insert Raw");
	dlg->connect (bbx_buttons, SIGNAL (accepted()), dlg, SLOT (accept()));
	dlg->connect (bbx_buttons, SIGNAL (rejected()), dlg, SLOT (reject()));

	if (dlg->exec() == QDialog::Rejected)
		return;

	CurrentDocument()->clearSelection();

	for (QString line : QString (te_edit->toPlainText()).split ("\n"))
	{
		LDObjectPtr obj = ParseLine (line);

		CurrentDocument()->insertObj (idx, obj);
		obj->select();
		idx++;
	}

	refresh();
	scrollToSelection();
}

// =============================================================================
//
void MainWindow::actionScreenshot()
{
	setlocale (LC_ALL, "C");

	int w, h;
	uchar* imgdata = R()->getScreencap (w, h);
	QImage img = GetImageFromScreencap (imgdata, w, h);

	QString root = Basename (CurrentDocument()->name());

	if (root.right (4) == ".dat")
		root.chop (4);

	QString defaultname = (root.length() > 0) ? format ("%1.png", root) : "";
	QString fname = QFileDialog::getSaveFileName (g_win, "Save Screencap", defaultname,
				"PNG images (*.png);;JPG images (*.jpg);;BMP images (*.bmp);;All Files (*.*)");

	if (not fname.isEmpty() and not img.save (fname))
		Critical (format ("Couldn't open %1 for writing to save screencap: %2", fname, strerror (errno)));

	delete[] imgdata;
}

// =============================================================================
//
void MainWindow::actionAxes()
{
	cfg::DrawAxes = not cfg::DrawAxes;
	updateActions();
	R()->update();
}

// =============================================================================
//
void MainWindow::actionVisibilityToggle()
{
	for (LDObjectPtr obj : Selection())
		obj->setHidden (not obj->isHidden());

	refresh();
}

// =============================================================================
//
void MainWindow::actionVisibilityHide()
{
	for (LDObjectPtr obj : Selection())
		obj->setHidden (true);

	refresh();
}

// =============================================================================
//
void MainWindow::actionVisibilityReveal()
{
	for (LDObjectPtr obj : Selection())
	obj->setHidden (false);
	refresh();
}

// =============================================================================
//
void MainWindow::actionWireframe()
{
	cfg::DrawWireframe = not cfg::DrawWireframe;
	R()->refresh();
}

// =============================================================================
//
void MainWindow::actionSetOverlay()
{
	OverlayDialog dlg;

	if (not dlg.exec())
		return;

	R()->setupOverlay ((ECamera) dlg.camera(), dlg.fpath(), dlg.ofsx(),
		dlg.ofsy(), dlg.lwidth(), dlg.lheight());
}

// =============================================================================
//
void MainWindow::actionClearOverlay()
{
	R()->clearOverlay();
}

// =============================================================================
//
void MainWindow::actionModeSelect()
{
	R()->setEditMode (EditModeType::Select);
}

// =============================================================================
//
void MainWindow::actionModeDraw()
{
	R()->setEditMode (EditModeType::Draw);
}

// =============================================================================
//
void MainWindow::actionModeRectangle()
{
	R()->setEditMode (EditModeType::Rectangle);
}

// =============================================================================
//
void MainWindow::actionModeCircle()
{
	R()->setEditMode (EditModeType::Circle);
}

// =============================================================================
//
void MainWindow::actionModeMagicWand()
{
 	R()->setEditMode (EditModeType::MagicWand);
}

void MainWindow::actionModeLinePath()
{
	R()->setEditMode (EditModeType::LinePath);
}

// =============================================================================
//
void MainWindow::actionDrawAngles()
{
	cfg::DrawAngles = not cfg::DrawAngles;
	R()->refresh();
}

// =============================================================================
//
void MainWindow::actionSetDrawDepth()
{
	if (R()->camera() == EFreeCamera)
		return;

	bool ok;
	double depth = QInputDialog::getDouble (g_win, "Set Draw Depth",
											format ("Depth value for %1 Camera:", R()->getCameraName()),
											R()->getDepthValue(), -10000.0f, 10000.0f, 3, &ok);

	if (ok)
		R()->setDepthValue (depth);
}

#if 0
// This is a test to draw a dummy axle. Meant to be used as a primitive gallery,
// but I can't figure how to generate these pictures properly. Multi-threading
// these is an immense pain.
void MainWindow::actiontestpic()
{
	LDDocumentPtr file = getFile ("axle.dat");
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

// =============================================================================
//
void MainWindow::actionScanPrimitives()
{
	PrimitiveScanner::start();
}

// =============================================================================
//
void MainWindow::actionBFCView()
{
	cfg::BFCRedGreenView = not cfg::BFCRedGreenView;

	if (cfg::BFCRedGreenView)
		cfg::RandomColors = false;

	updateActions();
	R()->refresh();
}

// =============================================================================
//
void MainWindow::actionJumpTo()
{
	bool ok;
	int defval = 0;
	LDObjectPtr obj;

	if (Selection().size() == 1)
		defval = Selection()[0]->lineNumber();

	int idx = QInputDialog::getInt (null, "Go to line", "Go to line:", defval,
		1, CurrentDocument()->getObjectCount(), 1, &ok);

	if (not ok or (obj = CurrentDocument()->getObject (idx - 1)) == null)
		return;

	CurrentDocument()->clearSelection();
	obj->select();
	updateSelection();
}

// =============================================================================
//
void MainWindow::actionSubfileSelection()
{
	if (Selection().size() == 0)
		return;

	QString			parentpath (CurrentDocument()->fullPath());

	// BFC type of the new subfile - it shall inherit the BFC type of the parent document
	BFCStatement	bfctype (BFCStatement::NoCertify);

	// Dirname of the new subfile
	QString			subdirname (Dirname (parentpath));

	// Title of the new subfile
	QString			subtitle;

	// Comment containing the title of the parent document
	LDCommentPtr	titleobj (CurrentDocument()->getObject (0).dynamicCast<LDComment>());

	// License text for the subfile
	QString			license (PreferredLicenseText());

	// LDraw code body of the new subfile (i.e. code of the selection)
	QStringList		code;

	// Full path of the subfile to be
	QString			fullsubname;

	// Where to insert the subfile reference?
	int				refidx (Selection()[0]->lineNumber());

	// Determine title of subfile
	if (titleobj != null)
		subtitle = "~" + titleobj->text();
	else
		subtitle = "~subfile";

	// Remove duplicate tildes
	while (subtitle.startsWith ("~~"))
		subtitle.remove (0, 1);

	// If this the parent document isn't already in s/, we need to stuff it into
	// a subdirectory named s/. Ensure it exists!
	QString topdirname = Basename (Dirname (CurrentDocument()->fullPath()));

	if (topdirname != "s")
	{
		QString desiredPath = subdirname + "/s";
		QString title = tr ("Create subfile directory?");
		QString text = format (tr ("The directory <b>%1</b> is suggested for "
			"subfiles. This directory does not exist, create it?"), desiredPath);

		if (QDir (desiredPath).exists() or Confirm (title, text))
		{
			subdirname = desiredPath;
			QDir().mkpath (subdirname);
		}
		else
			return;
	}

	// Determine the body of the name of the subfile
	if (not parentpath.isEmpty())
	{
		// Chop existing '.dat' suffix
		if (parentpath.endsWith (".dat"))
			parentpath.chop (4);

		// Remove the s?? suffix if it's there, otherwise we'll get filenames
		// like s01s01.dat when subfiling subfiles.
		QRegExp subfilesuffix ("s[0-9][0-9]$");
		if (subfilesuffix.indexIn (parentpath) != -1)
			parentpath.chop (subfilesuffix.matchedLength());

		int subidx = 1;
		QString digits;
		QFile f;
		QString testfname;

		// Now find the appropriate filename. Increase the number of the subfile
		// until we find a name which isn't already taken.
		do
		{
			digits.setNum (subidx++);

			// pad it with a zero
			if (digits.length() == 1)
				digits.prepend ("0");

			fullsubname = subdirname + "/" + Basename (parentpath) + "s" + digits + ".dat";
		} while (FindDocument ("s\\" + Basename (fullsubname)) != null or QFile (fullsubname).exists());
	}

	// Determine the BFC winding type used in the main document - it is to
	// be carried over to the subfile.
	LDIterate<LDBFC> (CurrentDocument()->objects(), [&] (LDBFCPtr const& bfc)
	{
		if (Eq (bfc->statement(), BFCStatement::CertifyCCW, BFCStatement::CertifyCW, 
			BFCStatement::NoCertify))
		{
			bfctype = bfc->statement();
			Break();
		}
	});

	// Get the body of the document in LDraw code
	for (LDObjectPtr obj : Selection())
		code << obj->asText();

	// Create the new subfile document
	LDDocumentPtr doc = LDDocument::createNew();
	doc->setImplicit (false);
	doc->setFullPath (fullsubname);
	doc->setName (LDDocument::shortenName (fullsubname));

	LDObjectList objs;
	objs << LDSpawn<LDComment> (subtitle);
	objs << LDSpawn<LDComment> ("Name: "); // This gets filled in when the subfile is saved
	objs << LDSpawn<LDComment> (format ("Author: %1 [%2]", cfg::DefaultName, cfg::DefaultUser));
	objs << LDSpawn<LDComment> ("!LDRAW_ORG Unofficial_Subpart");

	if (not license.isEmpty())
		objs << LDSpawn<LDComment> (license);

	objs << LDSpawn<LDEmpty>();
	objs << LDSpawn<LDBFC> (bfctype);
	objs << LDSpawn<LDEmpty>();

	doc->addObjects (objs);

	// Add the actual subfile code to the new document
	for (QString line : code)
	{
		LDObjectPtr obj = ParseLine (line);
		doc->addObject (obj);
	}

	// Try save it
	if (save (doc, true))
	{
		// Save was successful. Delete the original selection now from the
		// main document.
		for (LDObjectPtr obj : Selection())
			obj->destroy();

		// Add a reference to the new subfile to where the selection was
		LDSubfilePtr ref (LDSpawn<LDSubfile>());
		ref->setColor (MainColor());
		ref->setFileInfo (doc);
		ref->setPosition (Origin);
		ref->setTransform (IdentityMatrix);
		CurrentDocument()->insertObj (refidx, ref);

		// Refresh stuff
		updateDocumentList();
		doFullRefresh();
	}
	else
	{
		// Failed to save.
		doc->dismiss();
	}
}

void MainWindow::actionRandomColors()
{
	cfg::RandomColors = not cfg::RandomColors;

	if (cfg::RandomColors)
		cfg::BFCRedGreenView = false;

	updateActions();
	R()->refresh();
}

void MainWindow::actionOpenSubfiles()
{
	for (LDObjectPtr obj : Selection())
	{
		LDSubfilePtr ref = obj.dynamicCast<LDSubfile>();

		if (ref == null or not ref->fileInfo()->isImplicit())
			continue;

		ref->fileInfo()->setImplicit (false);
	}
}

void MainWindow::actionDrawSurfaces()
{
	cfg::DrawSurfaces = not cfg::DrawSurfaces;
	updateActions();
	update();
}

void MainWindow::actionDrawEdgeLines()
{
	cfg::DrawEdgeLines = not cfg::DrawEdgeLines;
	updateActions();
	update();
}

void MainWindow::actionDrawConditionalLines()
{
	cfg::DrawConditionalLines = not cfg::DrawConditionalLines;
	updateActions();
	update();
}
