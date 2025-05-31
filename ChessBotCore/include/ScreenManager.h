/**
 * @file ScreenManager.h
 * @brief Singleton class to access and manage available screens using QScreen.
 *
 * Provides access to the list of screens, primary screen, and utility methods
 * to retrieve screens by index or name. Used in the ChessBotCore module for screen capture.
 *
 * @author Lucas
 * @date 2025-05-31
 */

#pragma once

#include "chessbotcore_global.h"

#include <QtGui/QScreen>
#include <QtCore/QVector>

/**
 * @class ScreenManager
 * @brief Manages access to available screens (QScreen) in the system.
 *
 * This class follows the singleton pattern and provides utility methods
 * to retrieve information about available screens. It's used for screen capture
 * and screen-based logic within ChessBotCore.
 */
class CHESSBOTCORE_EXPORT ScreenManager
{
public:
	/**
	 * @brief Returns the singleton instance of the ScreenManager.
	 * @return Reference to the single instance.
	 */
	static ScreenManager& instance();

	/**
	 * @brief Returns a list of all available screens.
	 * @return QVector of QScreen pointers.
	 */
	QVector<QScreen*> screens() const;

	/**
	 * @brief Returns the screen at the given index.
	 * @param index Index of the screen in the list.
	 * @return Pointer to QScreen, or nullptr if index is invalid.
	 */
	QScreen* screen(int index) const;

	/**
	 * @brief Returns the screen with the given name.
	 * @param name Name of the screen (as given by QScreen::name()).
	 * @return Pointer to QScreen, or nullptr if not found.
	 */
	QScreen* screen(const QString& name) const;

	/**
	 * @brief Returns the primary screen.
	 * @return Pointer to the primary QScreen.
	 */
	QScreen* primaryScreen() const;

private:
	ScreenManager() = default;
	~ScreenManager() = default;
	ScreenManager(const ScreenManager&) = delete;
	ScreenManager& operator=(const ScreenManager&) = delete;
};
