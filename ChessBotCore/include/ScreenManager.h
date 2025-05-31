#pragma once

#include "chessbotcore_global.h"

#include <QtGui/QScreen>
#include <QtCore/QVector>

class CHESSBOTCORE_EXPORT ScreenManager
{
public:
	static ScreenManager& instance();

	QVector<QScreen*> screens() const;

	QScreen* screen(int index) const;

	QScreen* screen(const QString& name) const;

	QScreen* primaryScreen() const;

private:
	ScreenManager() = default;
	~ScreenManager() = default;
	ScreenManager(const ScreenManager&) = delete;
	ScreenManager& operator=(const ScreenManager&) = delete;
};
