#include <QFileDialog>
#include <QPushButton>
#include <QLabel>
#include "ldrawpathdialog.h"
#include "ui_ldrawpathdialog.h"
#include "../mainWindow.h"

LDrawPathDialog::LDrawPathDialog (const QString& defaultPath, bool validDefault, QWidget* parent, Qt::WindowFlags f) :
	QDialog (parent, f),
	m_hasValidDefault (validDefault),
	ui (*new Ui_LDrawPathDialog)
{
	ui.setupUi (this);
	ui.status->setText ("---");

	if (validDefault)
		ui.heading->hide();
	else
	{
		cancelButton()->setText ("Exit");
		cancelButton()->setIcon (GetIcon ("exit"));
	}

	okButton()->setEnabled (false);

	connect (ui.path, SIGNAL (textChanged (QString)), this, SIGNAL (pathChanged (QString)));
	connect (ui.searchButton, SIGNAL (clicked()), this, SLOT (searchButtonClicked()));
	connect (ui.buttonBox, SIGNAL (rejected()), this, validDefault ? SLOT (reject()) : SLOT (slot_exit()));
	connect (ui.buttonBox, SIGNAL (accepted()), this, SLOT (slot_accept()));
	setPath (defaultPath);
}

LDrawPathDialog::~LDrawPathDialog()
{
	delete &ui;
}

QPushButton* LDrawPathDialog::okButton()
{
	return ui.buttonBox->button (QDialogButtonBox::Ok);
}

QPushButton* LDrawPathDialog::cancelButton()
{
	return ui.buttonBox->button (QDialogButtonBox::Cancel);
}

void LDrawPathDialog::setPath (QString path)
{
	ui.path->setText (path);
}

QString LDrawPathDialog::path() const
{
	return ui.path->text();
}

void LDrawPathDialog::searchButtonClicked()
{
	QString newpath = QFileDialog::getExistingDirectory (this, "Find LDraw Path");

	if (not newpath.isEmpty())
		setPath (newpath);
}

void LDrawPathDialog::slot_exit()
{
	Exit();
}

void LDrawPathDialog::setStatusText (const QString& statusText, bool ok)
{
	okButton()->setEnabled (ok);

	if (statusText.isEmpty() && ok == false)
		ui.status->setText ("---");
	else
	{
		ui.status->setText (QString ("<span style=\"color: %1\">%2</span>")
			.arg (ok ? "#700" : "#270")
			.arg (statusText));
	}
}

void LDrawPathDialog::slot_accept()
{
	Config::Save();
	accept();
}