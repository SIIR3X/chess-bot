/**
 * @file ScreenRecorder.h
 * @brief Screen recording class using DirectX and the Desktop Duplication API.
 *
 * Captures screen content via the Desktop Duplication API using D3D11, and streams
 * frames back to the user via a callback mechanism. Used in ChessBotCore for screen capture automation.
 *
 * @author Lucas
 * @date 2025-05-31
 */

#pragma once

#include "pch.h"
#include "chessbotcore_global.h"

#include <atomic>
#include <thread>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <Windows.h>
#include <functional>
#include <wrl/client.h>
#include <opencv2/core.hpp>

 /**
  * @class ScreenRecorder
  * @brief Records screen frames using the Windows Desktop Duplication API.
  *
  * Initializes a D3D11 device, selects a monitor by index, and captures screen
  * frames asynchronously. Captured frames are returned as OpenCV matrices via callback.
  */
class CHESSBOTCORE_EXPORT ScreenRecorder
{
public:
	/**
	 * @brief Callback type used to receive captured frames.
	 * @param mat Captured screen frame as an OpenCV cv::Mat.
	 */
	using FrameCallback = std::function<void(const cv::Mat&)>;

	/**
	 * @brief Default constructor.
	 */
	ScreenRecorder() = default;

	/**
	 * @brief Destructor. Stops recording if active.
	 */
	~ScreenRecorder();

	/**
	 * @brief Initializes the screen recorder for a specific monitor.
	 * @param outputIndex Index of the screen (monitor) to capture.
	 * @return True if initialization succeeded, false otherwise.
	 */
	bool initialize(UINT outputIndex);

	/**
	 * @brief Starts screen recording and registers the frame callback.
	 * @param callback Function to be called with each captured frame.
	 * @return True if recording started successfully, false otherwise.
	 */
	bool startRecording(FrameCallback callback);

	/**
	 * @brief Stops screen recording.
	 */
	void stopRecording();

private:
	/**
	 * @brief Main loop that performs screen capture in a separate thread.
	 */
	void captureLoop();

	/**
	 * @brief Creates the D3D11 device and context required for duplication.
	 * @return True on success, false otherwise.
	 */
	bool createD3DDevice();

	/**
	 * @brief Initializes the desktop duplication interface for the selected output.
	 * @return True on success, false otherwise.
	 */
	bool initDesktopDuplication();

	// --- DirectX Interfaces ---
	Microsoft::WRL::ComPtr<ID3D11Device>           d3dDevice_;	  ///< D3D11 device.
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>    d3dContext_;	  ///< D3D11 context.
	Microsoft::WRL::ComPtr<IDXGIDevice>            dxgiDevice_;	  ///< DXGI device interface.
	Microsoft::WRL::ComPtr<IDXGIAdapter>           dxgiAdapter_;  ///< DXGI adapter.
	Microsoft::WRL::ComPtr<IDXGIOutput>            dxgiOutput_;   ///< DXGI output (monitor).
	Microsoft::WRL::ComPtr<IDXGIOutput1>           dxgiOutput1_;  ///< Extended output interface.
	Microsoft::WRL::ComPtr<IDXGIOutputDuplication> deskDupl_;     ///< Desktop duplication object.
	Microsoft::WRL::ComPtr<ID3D11Texture2D>        stagingTex_;   ///< Staging texture to read frame data.

	// --- Threading and Control ---
	std::thread             captureThread_;        ///< Thread used to run capture loop.
	std::atomic<bool>       isRecording_{ false }; ///< Flag indicating if recording is active.
	FrameCallback           frameCallback_;        ///< Callback function to send captured frames.

	// --- Capture Parameters ---
	UINT                    width_{ 0 };	  ///< Width of the captured output.
	UINT                    height_{ 0 };	  ///< Height of the captured output.
	DXGI_OUTPUT_DESC        outputDesc_ = {}; ///< Output description.
	DXGI_OUTDUPL_FRAME_INFO frameInfo_ = {};  ///< Frame metadata info.
	UINT                    timeoutMilliseconds_{ 5 }; ///< Frame acquisition timeout in ms.
	UINT                    chosenOutputIndex_{ 0 };   ///< Selected output index.

	// --- Deleted copy operations ---
	ScreenRecorder(const ScreenRecorder&) = delete;
	ScreenRecorder& operator=(const ScreenRecorder&) = delete;
};
