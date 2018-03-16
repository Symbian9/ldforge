#pragma once
#include <QWidget>
#include "../lddocument.h"

class HeaderEdit : public QWidget
{
	Q_OBJECT

public:
	HeaderEdit(QWidget* parent = nullptr);
	~HeaderEdit();

	void setHeader(LDHeader* header);
	LDHeader* header() const;
	bool hasValidHeader() const;

signals:
	void descriptionChanged(const QString& newDescription);
	void windingChanged(Winding newWinding);

private:
	class Ui_HeaderEdit& ui;
	class HeaderHistoryModel* headerHistoryModel = nullptr;
	LDHeader* m_header = nullptr;
};
