#include "ScreenRecorder.h"
#include <iostream>

ScreenRecorder::~ScreenRecorder() {
    stopRecording();

	// Ensure the duplication interface realeases any acquired frames
    if (deskDupl_) {
        deskDupl_->ReleaseFrame();
    }
}

bool ScreenRecorder::initialize(UINT outputIndex) {
    chosenOutputIndex_ = outputIndex;
    
	// Create D3D11 device and context
    if (!createD3DDevice()) {
		std::cerr << "[ScreenRecorder] Failed to create D3D11 device." << std::endl;
		return false;
    }

	// Initialize desktop duplication for the chosen output
    if (!initDesktopDuplication()) {
		std::cerr << "[ScreenRecorder] Failed to initialize desktop duplication." << std::endl;
		return false;
    }

    return true;
}

bool ScreenRecorder::startRecording(FrameCallback callback) {
	// Check if the recorder is initialized
    if (!deskDupl_) {
		std::cerr << "[ScreenRecorder] Desktop duplication not initialized." << std::endl;
		return false;
    }

	// Check if already recording
    if (isRecording_.load()) {
		std::cerr << "[ScreenRecorder] Already recording." << std::endl;
		return false;
    }

    frameCallback_ = std::move(callback);
    isRecording_.store(true);
    captureThread_ = std::thread(&ScreenRecorder::captureLoop, this);

    return true;
}

void ScreenRecorder::stopRecording() {
	// Check if recording is active
    if (!isRecording_.exchange(false)) return;

	// Wait for the capture thread to finish
    if (captureThread_.joinable()) {
        captureThread_.join();
    }

	// Release any resources used by the desktop duplication interface
    if (deskDupl_) {
        deskDupl_->ReleaseFrame();
    }
}

bool ScreenRecorder::createD3DDevice() {
	// List of feature levels to attempt (from highest to lowest)
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    D3D_FEATURE_LEVEL obtainedLevel = D3D_FEATURE_LEVEL_11_0;

	// Create the D3D11 device with BGRA support for desktop duplication
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

	// Create the D3D11 device and context
    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        creationFlags,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &d3dDevice_,
        &obtainedLevel,
        &d3dContext_
    );
    if (FAILED(hr)) return false;

    hr = d3dDevice_.As(&dxgiDevice_);
    if (FAILED(hr)) return false;

    hr = dxgiDevice_->GetAdapter(&dxgiAdapter_);
    if (FAILED(hr)) return false;

    return true;
}

bool ScreenRecorder::initDesktopDuplication() {
    HRESULT hr;

    hr = dxgiAdapter_->EnumOutputs(chosenOutputIndex_, &dxgiOutput_);
    if (hr == DXGI_ERROR_NOT_FOUND) {
		std::cerr << "[ScreenRecorder] Output index " << chosenOutputIndex_ << " not found." << std::endl;
		return false;
    }

    if (FAILED(hr)) return false;

    hr = dxgiOutput_.As(&dxgiOutput1_);
    if (FAILED(hr)) return false;

	// Retrive output description (resolution, position, etc.)
    dxgiOutput_->GetDesc(&outputDesc_);
    width_ = outputDesc_.DesktopCoordinates.right - outputDesc_.DesktopCoordinates.left;
    height_ = outputDesc_.DesktopCoordinates.bottom - outputDesc_.DesktopCoordinates.top;

	// Attempt to duplicate the output
    hr = dxgiOutput1_->DuplicateOutput(d3dDevice_.Get(), &deskDupl_);
    if (FAILED(hr)) return false;

    // Create staging texture for CPU access (used to read pixel data)
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width_;
    desc.Height = height_;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;

    hr = d3dDevice_->CreateTexture2D(&desc, nullptr, &stagingTex_);

    return SUCCEEDED(hr);
}

void ScreenRecorder::captureLoop() {
    while (isRecording_.load()) {
        IDXGIResource* desktopResource = nullptr;

        // Try to acquire the next frame from the desktop duplication interface
        HRESULT hr = deskDupl_->AcquireNextFrame(timeoutMilliseconds_, &frameInfo_, &desktopResource);

        if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
            continue; // No new frame yet
        }

        if (FAILED(hr)) {
            std::cerr << "[ScreenRecorder] Failed to acquire next frame: 0x"
                << std::hex << hr << std::dec << std::endl;
            break;
        }

        if (!desktopResource) {
            std::cerr << "[ScreenRecorder] desktopResource is null after successful AcquireNextFrame.\n";
            deskDupl_->ReleaseFrame();
            continue;
        }

        ID3D11Texture2D* acquiredTexture = nullptr;
        hr = desktopResource->QueryInterface(__uuidof(ID3D11Texture2D),
            reinterpret_cast<void**>(&acquiredTexture));

        if (SUCCEEDED(hr) && acquiredTexture) {
            // Copy the texture to a staging texture for CPU read access
            d3dContext_->CopyResource(stagingTex_.Get(), acquiredTexture);

            // Map the staging texture for reading
            D3D11_MAPPED_SUBRESOURCE mapped = {};
            hr = d3dContext_->Map(stagingTex_.Get(), 0, D3D11_MAP_READ, 0, &mapped);
            if (SUCCEEDED(hr)) {
                // Wrap the mapped memory as an OpenCV matrix
                cv::Mat frame(height_, width_, CV_8UC4, mapped.pData, mapped.RowPitch);

                // Clone the frame to ensure ownership of memory
                cv::Mat frameCopy = frame.clone();

                // Unmap the texture
                d3dContext_->Unmap(stagingTex_.Get(), 0);

                // Invoke the callback with the captured frame
                if (frameCallback_) {
                    frameCallback_(frameCopy);
                }
            }
            else {
                std::cerr << "[ScreenRecorder] Failed to map staging texture: 0x"
                    << std::hex << hr << std::dec << std::endl;
            }

            acquiredTexture->Release();
        }
        else {
            std::cerr << "[ScreenRecorder] Failed to query ID3D11Texture2D: 0x"
                << std::hex << hr << std::dec << std::endl;
        }

        // Always release the current frame and associated resource
        deskDupl_->ReleaseFrame();
        desktopResource->Release();
    }
}
