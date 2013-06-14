/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 Santeri Piippo
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

#include <QProcess>
#include <QTemporaryFile>
#include <QDialog>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>

#include "common.h"
#include "config.h"
#include "misc.h"
#include "extprogs.h"
#include "gui.h"
#include "file.h"
#include "widgets.h"
#include "history.h"
#include "labeledwidget.h"

// =============================================================================
cfg (str, prog_isecalc, "");
cfg (str, prog_intersector, "");
cfg (str, prog_coverer, "");
cfg (str, prog_ytruder, "");
cfg (str, prog_rectifier, "");

const char* g_extProgNames[] = {
	"Isecalc",
	"Intersector",
	"Coverer",
	"Ytruder",
	"Rectifier",
};

// =============================================================================
static bool checkProgPath (str path, const extprog prog) {
	if (path.length () > 0)
		return true;
	
	const char* name = g_extProgNames[prog];
	
	critical (fmt ("Couldn't run %1 as no path has "
		"been defined for it. Use the configuration dialog's External Programs "
		"tab to define a path for %1.", name));
	return false;
}

// =============================================================================
static void processError (const extprog prog, QProcess& proc) {
	const char* name = g_extProgNames[prog];
	str errmsg;
	
	switch (proc.error ()) {
	case QProcess::FailedToStart:
		errmsg = fmt ("Failed to launch %1. Check that you have set the proper path "
			"to %1 and that you have the proper permissions to launch it.", name);
		break;
	
	case QProcess::Crashed:
		errmsg = fmt ("%1 crashed.", name);
		break;
	
	case QProcess::WriteError:
	case QProcess::ReadError:
		errmsg = fmt ("I/O error while interacting with %1.", name);
		break;
	
	case QProcess::UnknownError:
		errmsg = fmt ("Unknown error occurred while executing %1.", name);
		break;
	
	case QProcess::Timedout:
		errmsg = fmt ("%1 timed out.", name);
		break;
	}
	
	critical (errmsg);
}

// =============================================================================
static bool mkTempFile (QTemporaryFile& tmp, str& fname) {
	if (!tmp.open ())
		return false;
	
	fname = tmp.fileName ();
	tmp.close ();
	return true;
}

// =============================================================================
void writeObjects (vector<LDObject*>& objects, File& f) {
	for (LDObject* obj : objects) {
		if (obj->getType () == LDObject::Subfile) {
			vector<LDObject*> objs = static_cast<LDSubfile*> (obj)->inlineContents (true, false);
			
			writeObjects (objs, f);
			for (LDObject* obj : objs)
				delete obj;
		} else if (obj->getType () == LDObject::Radial) {
			vector<LDObject*> objs = static_cast<LDRadial*> (obj)->decompose (true);
			
			writeObjects (objs, f);
			for (LDObject* obj : objs)
				delete obj;
		} else
			f.write (obj->raw () + "\r\n");
	}
}

void writeObjects (vector<LDObject*>& objects, str fname) {
	// Write the input file
	File f (fname, File::Write);
	if (!f) {
		critical (fmt ("Couldn't open temporary file %1 for writing.\n", fname));
		return;
	}
	
	writeObjects (objects, f);
	f.close ();
}

// =============================================================================
void writeSelection (str fname) {
	writeObjects (g_win->sel (), fname);
}

// =============================================================================
void writeColorGroup (const short colnum, str fname) {
	vector<LDObject*> objects;
	for (LDObject*& obj : *g_curfile) {
		if (obj->isColored () == false || obj->color () != colnum)
			continue;
		
		objects << obj;
	}
	
	writeObjects (objects, fname);
}

// =============================================================================
void runUtilityProcess (extprog prog, str path, QString argvstr) {
	QTemporaryFile input, output;
	str inputname, outputname;
	QStringList argv = argvstr.split (" ", QString::SkipEmptyParts);
	
	printf ("cmdline: %s %s\n", qchars (path), qchars (argvstr));
	
	if (!mkTempFile (input, inputname) || !mkTempFile (output, outputname))
		return;
	
	QProcess proc;
	
	// Init stdin
	File stdinfp (inputname, File::Write);
	
	// Begin!
	proc.setStandardInputFile (inputname);
	proc.start (path, argv);
	
	// Write an enter - one is expected
	stdinfp.write ("\n");
	
	// Wait while it runs
	proc.waitForFinished ();
	
#ifndef RELEASE
	printf ("%s", qchars (QString (proc.readAllStandardOutput ())));
#endif // RELEASE
	
	if (proc.exitStatus () == QProcess::CrashExit) {
		processError (prog, proc);
		return;
	}
}

// ================================================================================================
static void insertOutput (str fname, bool replace, vector<short> colorsToReplace) {
#ifndef RELEASE
	QFile::copy (fname, "./debug_lastOutput");
#endif // RELEASE
	
	// Read the output file
	File f (fname, File::Read);
	if (!f) {
		critical (fmt ("Couldn't open temporary file %1 for reading.\n", fname));
		return;
	}
	
	vector<LDObject*> objs = loadFileContents (&f, null);
	
	// If we replace the objects, delete the selection now.
	if (replace)
		g_win->deleteSelection ();
	
	for (const short colnum : colorsToReplace)
		g_win->deleteByColor (colnum);
	
	// Insert the new objects
	g_win->sel ().clear ();
	for (LDObject* obj : objs) {
		if (!obj->isScemantic ()) {
			delete obj;
			continue;
		}
		
		g_curfile->addObject (obj);
		g_win->sel () << obj;
	}
	
	g_win->fullRefresh ();
}

// =============================================================================
// Interface for Ytruder
void runYtruder () {
	setlocale (LC_ALL, "C");
	
	if (!checkProgPath (prog_ytruder, Ytruder))
		return;
	
	QDialog dlg;
	
	RadioBox* rb_mode = new RadioBox ("Extrusion mode", {"Distance", "Symmetry", "Projection", "Radial"}, 0, Qt::Horizontal);
	RadioBox* rb_axis = new RadioBox ("Axis", {"X", "Y", "Z"}, 0, Qt::Horizontal);
	LabeledWidget<QDoubleSpinBox>* dsb_depth = new LabeledWidget<QDoubleSpinBox> ("Plane depth"),
		*dsb_condAngle = new LabeledWidget<QDoubleSpinBox> ("Conditional line threshold");
	
	rb_axis->setValue (Y);
	dsb_depth->w ()->setMinimum (-10000.0);
	dsb_depth->w ()->setMaximum (10000.0);
	dsb_depth->w ()->setDecimals (3);
	dsb_condAngle->w ()->setValue (30.0f);
	
	QVBoxLayout* layout = new QVBoxLayout (&dlg);
	layout->addWidget (rb_mode);
	layout->addWidget (rb_axis);
	layout->addWidget (dsb_depth);
	layout->addWidget (dsb_condAngle);
	layout->addWidget (makeButtonBox (dlg));
	
	dlg.setWindowIcon (getIcon ("extrude"));
	
	if (!dlg.exec ())
		return;
	
	// Read the user's choices
	const enum modetype { Distance, Symmetry, Projection, Radial } mode = (modetype) rb_mode->value ();
	const Axis axis = (Axis) rb_axis->value ();
	const double depth = dsb_depth->w ()->value (),
		condAngle = dsb_condAngle->w ()->value ();
	
	QTemporaryFile indat, outdat;
	str inDATName, outDATName;
	
	// Make temp files for the input and output files
	if (!mkTempFile (indat, inDATName) || !mkTempFile (outdat, outDATName))
		return;
	
	// Compose the command-line arguments
	str argv = join ({
		(axis == X) ? "-x" : (axis == Y) ? "-y" : "-z",
		(mode == Distance) ? "-d" : (mode == Symmetry) ? "-s" : (mode == Projection) ? "-p" : "-r",
		depth,
		"-a",
		condAngle,
		inDATName,
		outDATName
	});
	
	writeSelection (inDATName);
	runUtilityProcess (Ytruder, prog_ytruder, argv);
	insertOutput (outDATName, false, {});
}

// ========================================================================================================================================
// Rectifier interface
void runRectifier () {
	setlocale (LC_ALL, "C");
	
	if (!checkProgPath (prog_rectifier, Rectifier))
		return;
	
	QDialog dlg;
	QCheckBox* cb_condense = new QCheckBox ("Condense triangles to quads"),
		*cb_subst = new QCheckBox ("Substitute rect primitives"),
		*cb_condlineCheck = new QCheckBox ("Don't replace quads with adj. condlines"),
		*cb_colorize = new QCheckBox ("Colorize resulting objects");
	LabeledWidget<QDoubleSpinBox>* dsb_coplthres = new LabeledWidget<QDoubleSpinBox> ("Coplanarity threshold");
	
	dsb_coplthres->w ()->setMinimum (0.0f);
	dsb_coplthres->w ()->setMaximum (360.0f);
	dsb_coplthres->w ()->setDecimals (3);
	dsb_coplthres->w ()->setValue (0.95f);
	cb_condense->setChecked (true);
	cb_subst->setChecked (true);
	
	QVBoxLayout* layout = new QVBoxLayout (&dlg);
	layout->addWidget (cb_condense);
	layout->addWidget (cb_subst);
	layout->addWidget (cb_condlineCheck);
	layout->addWidget (cb_colorize);
	layout->addWidget (dsb_coplthres);
	layout->addWidget (makeButtonBox (dlg));
	
	if (!dlg.exec ())
		return;
	
	const bool condense = cb_condense->isChecked (),
		subst = cb_subst->isChecked (),
		condlineCheck = cb_condlineCheck->isChecked (),
		colorize = cb_colorize->isChecked ();
	const double coplthres = dsb_coplthres->w ()->value ();
	
	QTemporaryFile indat, outdat;
	str inDATName, outDATName;
	
	// Make temp files for the input and output files
	if (!mkTempFile (indat, inDATName) || !mkTempFile (outdat, outDATName))
		return;
	
	// Compose arguments
	str argv = join ({
		(condense == false) ? "-q" : "",
		(subst == false) ? "-r" : "",
		(condlineCheck) ? "-a" : "",
		(colorize) ? "-c" : "",
		"-t",
		coplthres,
		inDATName,
		outDATName
	});
	
	writeSelection (inDATName);
	runUtilityProcess (Rectifier, prog_rectifier, argv);
	insertOutput (outDATName, true, {});
}

LabeledWidget<QComboBox>* buildColorSelector (const char* label) {
	LabeledWidget<QComboBox>* widget = new LabeledWidget<QComboBox> (label, new QComboBox);
	makeColorSelector (widget->w ());
	return widget;
}

// =======================================================================================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =======================================================================================================================================
// Intersector interface
void runIntersector () {
	setlocale (LC_ALL, "C");
	
	if (!checkProgPath (prog_intersector, Intersector))
		return;
	
	QDialog dlg;
	
	LabeledWidget<QComboBox>* cmb_incol = buildColorSelector ("Input"),
		*cmb_cutcol = buildColorSelector ("Cutter");
	QCheckBox* cb_colorize = new QCheckBox ("Colorize output"),
		*cb_nocondense = new QCheckBox ("No condensing"),
		*cb_repeatInverse = new QCheckBox ("Repeat inverse"),
		*cb_edges = new QCheckBox ("Add edges");
	LabeledWidget<QDoubleSpinBox>* dsb_prescale = new LabeledWidget<QDoubleSpinBox> ("Prescaling factor");
	
	cb_repeatInverse->setWhatsThis ("If this is set, " APPNAME " runs Intersector a second time with inverse files to cut the "
		" cutter group with the input group. Both groups are cut by the intersection.");
	cb_edges->setWhatsThis ("Makes " APPNAME " try run Isecalc to create edgelines for the intersection.");
	
	dsb_prescale->w ()->setMinimum (0.0f);
	dsb_prescale->w ()->setMaximum (10000.0f);
	dsb_prescale->w ()->setSingleStep (0.01f);
	dsb_prescale->w ()->setValue (1.0f);
	
	QVBoxLayout* layout = new QVBoxLayout (&dlg);
	layout->addWidget (cmb_incol);
	layout->addWidget (cmb_cutcol);
	
	QHBoxLayout* cblayout = new QHBoxLayout;
	cblayout->addWidget (cb_colorize);
	cblayout->addWidget (cb_nocondense);
	
	QHBoxLayout* cb2layout = new QHBoxLayout;
	cb2layout->addWidget (cb_repeatInverse);
	cb2layout->addWidget (cb_edges);
	
	layout->addLayout (cblayout);
	layout->addLayout (cb2layout);
	layout->addWidget (dsb_prescale);
	layout->addWidget (makeButtonBox (dlg));
	
exec:
	if (!dlg.exec ())
		return;
	
	const short inCol = cmb_incol->w ()->itemData (cmb_incol->w ()->currentIndex ()).toInt (),
		cutCol =  cmb_cutcol->w ()->itemData (cmb_cutcol->w ()->currentIndex ()).toInt ();
	const bool repeatInverse = cb_repeatInverse->isChecked ();
	
	if (inCol == cutCol) {
		critical ("Cannot use the same color group for both input and cutter!");
		goto exec;
	}
	
	// Five temporary files!
	// indat = input group file
	// cutdat = cutter group file
	// outdat = primary output
	// outdat2 = inverse output
	// edgesdat = edges output (isecalc)
	QTemporaryFile indat, cutdat, outdat, outdat2, edgesdat;
	str inDATName, cutDATName, outDATName, outDAT2Name, edgesDATName;
	
	if (!mkTempFile (indat, inDATName) || !mkTempFile (cutdat, cutDATName) ||
		!mkTempFile (outdat, outDATName) || !mkTempFile (outdat2, outDAT2Name) ||
		!mkTempFile (edgesdat, edgesDATName))
	{
		return;
	}
	
	str parms = join ({
		(cb_colorize->isChecked ()) ? "-c" : "",
		(cb_nocondense->isChecked ()) ? "-t" : "",
		"-s",
		dsb_prescale->w ()->value ()
	});
	
	str argv_normal = join ({
		parms,
		inDATName,
		cutDATName,
		outDATName
	});
	
	str argv_inverse = join ({
		parms,
		cutDATName,
		inDATName,
		outDAT2Name
	});
	
	writeColorGroup (inCol, inDATName);
	writeColorGroup (cutCol, cutDATName);
	runUtilityProcess (Intersector, prog_intersector, argv_normal);
	insertOutput (outDATName, false, {inCol});
	
	if (repeatInverse) {
		runUtilityProcess (Intersector, prog_intersector, argv_inverse);
		insertOutput (outDAT2Name, false, {cutCol});
	}
	
	if (cb_edges->isChecked ()) {
		runUtilityProcess (Isecalc, prog_isecalc, join ({inDATName, cutDATName, edgesDATName}));
		insertOutput (edgesDATName, false, {});
	}
}

// =======================================================================================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =======================================================================================================================================
void runCoverer () {
	setlocale (LC_ALL, "C");
	
	if (!checkProgPath (prog_coverer, Coverer))
		return;
	
	QDialog dlg;
	
	LabeledWidget<QComboBox>* cmb_col1 = buildColorSelector ("Shape 1"),
		*cmb_col2 = buildColorSelector ("Shape 2");
	
	QDoubleSpinBox* dsb_segsplit = new QDoubleSpinBox;
	QLabel* lb_segsplit = new QLabel ("Segment split length:");
	QSpinBox* sb_bias = new QSpinBox;
	QLabel* lb_bias = new QLabel ("Bias:");
	QCheckBox* cb_reverse = new QCheckBox ("Reverse shape 2"),
		*cb_oldsweep = new QCheckBox ("Old sweep method");
	
	dsb_segsplit->setSpecialValueText ("No splitting");
	dsb_segsplit->setRange (0.0f, 10000.0f);
	sb_bias->setSpecialValueText ("No bias");
	sb_bias->setRange (-100, 100);
	
	QGridLayout* spinboxlayout = new QGridLayout;
	spinboxlayout->addWidget (lb_segsplit, 0, 0);
	spinboxlayout->addWidget (dsb_segsplit, 0, 1);
	spinboxlayout->addWidget (lb_bias, 1, 0);
	spinboxlayout->addWidget (sb_bias, 1, 1);
	
	QVBoxLayout* layout = new QVBoxLayout (&dlg);
	layout->addWidget (cmb_col1);
	layout->addWidget (cmb_col2);
	layout->addLayout (spinboxlayout);
	layout->addWidget (cb_reverse);
	layout->addWidget (cb_oldsweep);
	layout->addWidget (makeButtonBox (dlg));
	
exec:
	if (!dlg.exec ())
		return;
	
	const short in1Col = cmb_col1->w ()->itemData (cmb_col1->w ()->currentIndex ()).toInt (),
		in2Col = cmb_col1->w ()->itemData (cmb_col2->w ()->currentIndex ()).toInt ();
	
	if (in1Col == in2Col) {
		critical ("Cannot use the same color group for both input and cutter!");
		goto exec;
	}
	
	QTemporaryFile in1dat, in2dat, outdat;
	str in1DATName, in2DATName, outDATName;
	
	if (!mkTempFile (in1dat, in1DATName) || !mkTempFile (in2dat, in2DATName) || !mkTempFile (outdat, outDATName))
		return;
	
	str argv = join ({
		(cb_oldsweep->isChecked () ? "-s" : ""),
		(cb_reverse->isChecked () ? "-r" : ""),
		(dsb_segsplit->value () != 0 ? fmt ("-l %1", dsb_segsplit->value ()) : ""),
		(sb_bias->value () != 0 ? fmt ("-s %1", sb_bias->value ()) : ""),
		in1DATName,
		in2DATName,
		outDATName
	});
	
	writeColorGroup (in1Col, in1DATName);
	writeColorGroup (in2Col, in2DATName);
	runUtilityProcess (Coverer, prog_coverer, argv);
	insertOutput (outDATName, false, {});
}

void runIsecalc () {
	setlocale (LC_ALL, "C");
	
	if (!checkProgPath (prog_isecalc, Isecalc))
		return;
	
	QDialog dlg;
	
	LabeledWidget<QComboBox>* cmb_col1 = buildColorSelector ("Shape 1"),
		*cmb_col2 = buildColorSelector ("Shape 2");
	
	QVBoxLayout* layout = new QVBoxLayout (&dlg);
	layout->addWidget (cmb_col1);
	layout->addWidget (cmb_col2);
	layout->addWidget (makeButtonBox (dlg));
	
exec:
	if (!dlg.exec ())
		return;
	
	const short in1Col = cmb_col1->w ()->itemData (cmb_col1->w ()->currentIndex ()).toInt (),
		in2Col = cmb_col1->w ()->itemData (cmb_col2->w ()->currentIndex ()).toInt ();
	
	if (in1Col == in2Col) {
		critical ("Cannot use the same color group for both input and cutter!");
		goto exec;
	}
	
	QTemporaryFile in1dat, in2dat, outdat;
	str in1DATName, in2DATName, outDATName;
	
	if (!mkTempFile (in1dat, in1DATName) || !mkTempFile (in2dat, in2DATName) || !mkTempFile (outdat, outDATName))
		return;
	
	str argv = join ({
		in1DATName,
		in2DATName,
		outDATName
	});
	
	writeColorGroup (in1Col, in1DATName);
	writeColorGroup (in2Col, in2DATName);
	runUtilityProcess (Isecalc, prog_isecalc, argv);
	insertOutput (outDATName, false, {});
}