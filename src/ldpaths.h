#pragma once
#include "main.h"

class QDir;
class MainWindow;

class LDPaths : public QObject, public HierarchyElement
{
	Q_OBJECT

public:
	LDPaths (QObject* parent);
	void checkPaths();
	bool isValid (const class QDir& path) const;

	static QDir& baseDir();
	static QString& ldConfigPath();
	static QDir& primitivesDir();
	static QDir& partsDir();

public slots:
	bool configurePaths (QString path);

private:
	mutable QString m_error;
	class LDrawPathDialog* m_dialog;
};