/**
 * @file ScreenManager.h
 * @brief Singleton class to access and manage available screens without using Qt.
 *
 * Provides access to the list of screens, primary screen, and utility methods
 * to retrieve screens by index or name. Used in the ChessBotCore module for screen capture.
 * This version uses standard C++ and platform APIs to remain portable and testable in CI.
 *
 * @author Lucas
 * @date 2025-05-31
 */

#pragma once

#include "pch.h"
#include "chessbotcore_global.h"

#include <vector>
#include <string>

 /**
  * @struct ScreenInfo
  * @brief Stores basic information about a screen.
  *
  * This structure holds screen dimensions and an optional name identifier.
  */
struct ScreenInfo {
	int width;               ///< Width of the screen in pixels.
	int height;              ///< Height of the screen in pixels.
	std::string name;        ///< Name or identifier of the screen.
};

/**
 * @class ScreenManager
 * @brief Manages access to available screens using platform APIs (no Qt dependency).
 *
 * Follows the singleton pattern and provides utility methods
 * to retrieve screen information for screen capture or screen-based logic.
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
	 * @return std::vector of ScreenInfo objects.
	 */
	std::vector<ScreenInfo> screens() const;

	/**
	 * @brief Returns the screen at the given index.
	 * @param index Index of the screen in the list.
	 * @return Pointer to ScreenInfo, or nullptr if index is invalid.
	 */
	const ScreenInfo* screen(int index) const;

	/**
	 * @brief Returns the screen with the given name.
	 * @param name Name of the screen.
	 * @return Pointer to ScreenInfo, or nullptr if not found.
	 */
	const ScreenInfo* screen(const std::string& name) const;

	/**
	 * @brief Returns the primary screen.
	 * @return Pointer to the primary ScreenInfo, or nullptr if unavailable.
	 */
	const ScreenInfo* primaryScreen() const;

private:
	ScreenManager() = default;
	~ScreenManager() = default;
	ScreenManager(const ScreenManager&) = delete;
	ScreenManager& operator=(const ScreenManager&) = delete;
};
