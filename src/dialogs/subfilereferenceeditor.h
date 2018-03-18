#pragma once
#include <QDialog>
#include "../main.h"

class SubfileReferenceEditor : public QDialog
{
	Q_OBJECT

public:
	SubfileReferenceEditor(class LDSubfileReference* reference, QWidget *parent = nullptr);
	~SubfileReferenceEditor();

	void accept() override;
	void setPrimitivesTree(class PrimitiveManager* primitives);

private:
	class Ui_SubfileReferenceEditor& ui;
	class LDSubfileReference* const reference;
	LDColor color;
};
