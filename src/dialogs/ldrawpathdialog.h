#pragma once
#include <QDialog>
#include "../main.h"

class LDrawPathDialog : public QDialog
{
	Q_OBJECT

public:
	LDrawPathDialog (const QString& defaultPath, bool validDefault, QWidget* parent = null, Qt::WindowFlags f = 0);
	virtual ~LDrawPathDialog();
	QString path() const;
	void setPath (QString path);
	void setStatusText (const QString& statusText, bool ok);

signals:
	void pathChanged (QString newPath);

private:
	const bool m_hasValidDefault;
	class Ui_LDrawPathDialog& ui;
	QPushButton* okButton();
	QPushButton* cancelButton();

private slots:
	void searchButtonClicked();
	void slot_exit();
	void slot_accept();
};
