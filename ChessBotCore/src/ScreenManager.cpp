#include "ScreenManager.h"

ScreenManager& ScreenManager::instance()
{
	static ScreenManager instance;
	return instance;
}

QVector<QScreen*> ScreenManager::screens() const
{
	return QGuiApplication::screens().toVector();
}

QScreen* ScreenManager::screen(int index) const
{
	auto screenList = QGuiApplication::screens();

	if (index < 0 || index >= screenList.size()) {
		return nullptr;
	}

	return screenList.at(index);
}

QScreen* ScreenManager::screen(const QString& name) const
{
	auto screenList = QGuiApplication::screens();

	for (auto qscreen : screenList) {
		if (qscreen->name() == name) {
			return qscreen;
		}
	}
	return nullptr;
}

QScreen* ScreenManager::primaryScreen() const
{
	return QGuiApplication::primaryScreen();
}