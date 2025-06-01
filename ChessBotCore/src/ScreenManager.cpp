#include "ScreenManager.h"
#include <sstream>
#include <Windows.h>
#include <algorithm>

/**
 * @struct EnumContext
 * @brief Holds the collected list of screen information during monitor enumeration.
 */
struct EnumContext {
	std::vector<ScreenInfo> screens; ///< List of detected screens.
};

/**
 * @brief Callback function used by EnumDisplayMonitors to gather screen details.
 *
 * This function is called once per monitor. It extracts the screen dimensions
 * and converts the monitor's device name from wide characters to a UTF-8 std::string.
 *
 * @param hMonitor Handle to the current monitor.
 * @param hdcMonitor Unused device context.
 * @param lprcMonitor RECT defining the screen area of the monitor.
 * @param dwData Pointer to EnumContext to collect screen info.
 * @return TRUE to continue enumerating monitors.
 */
static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC, LPRECT lprcMonitor, LPARAM dwData)
{
	EnumContext* ctx = reinterpret_cast<EnumContext*>(dwData);

	MONITORINFOEX monitorInfo;
	monitorInfo.cbSize = sizeof(MONITORINFOEX);

	if (!GetMonitorInfo(hMonitor, &monitorInfo)) {
		return TRUE; // Skip monitor if we can't get info
	}

	ScreenInfo info;
	info.width = lprcMonitor->right - lprcMonitor->left;
	info.height = lprcMonitor->bottom - lprcMonitor->top;

	// Convert wchar_t[] (szDevice) to std::string
    char nameBuffer[64] = {0}; // Initialize the buffer with zeros.  
    size_t convertedChars = 0;  
    wcstombs_s(&convertedChars, nameBuffer, monitorInfo.szDevice, _TRUNCATE);  
    nameBuffer[sizeof(nameBuffer) - 1] = '\0';
    info.name = std::string(nameBuffer);

	ctx->screens.push_back(info);
	return TRUE;
}

ScreenManager& ScreenManager::instance()
{
	static ScreenManager instance;
	return instance;
}

std::vector<ScreenInfo> ScreenManager::screens() const
{
	std::vector<ScreenInfo> result;

	EnumContext ctx;
	EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&ctx));
	result = std::move(ctx.screens);

	return result;
}

const ScreenInfo* ScreenManager::screen(int index) const
{
	static std::vector<ScreenInfo> all = screens();

	if (index >= 0 && index <static_cast<int>(all.size())) {
		return &all[index];
	}

	return nullptr;
}

const ScreenInfo* ScreenManager::screen(const std::string& name) const
{
	static std::vector<ScreenInfo> all = screens();

	auto it = std::find_if(all.begin(), all.end(), [&](const ScreenInfo& s) {
		return s.name == name;
	});

	return nullptr;
}

const ScreenInfo* ScreenManager::primaryScreen() const
{
	static std::vector<ScreenInfo> all = screens();

	if (!all.empty()) {
		return &all[0];
	}

	return nullptr;
}