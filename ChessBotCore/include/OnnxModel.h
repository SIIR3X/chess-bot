/**
 * @file OnnxModel.h
 * @brief Object detection model wrapper using ONNX Runtime and OpenCV.
 *
 * Loads and runs inference on ONNX-based object detection models. Handles image
 * preprocessing (letterboxing), inference, and postprocessing including NMS.
 * Designed for use in ChessBotCore for detection automation tasks.
 *
 * @author Lucas
 * @date 2025-06-01
 */

#pragma once

#include "chessbotcore_global.h"

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

/**
 * @struct Detection
 * @brief Represents a single object detection result.
 */
struct Detection {
	cv::Rect bbox; ///< Bounding box of the detected object.
	float score;   ///< Confidence score of the detection.
	int classId;   ///< Class ID of the detected object.
};

/**
 * @class OnnxModel
 * @brief Loads and runs ONNX object detection models with preprocessing and postprocessing.
 */
class CHESSBOTCORE_EXPORT OnnxModel
{
public:
	/**
	 * @brief Constructor.
	 * @param modelPath Path to the ONNX model file.
	 * @param inputWidth Expected input width for the model.
	 * @param inputHeight Expected input height for the model.
	 * @param confidenceThreshold Minimum confidence to keep detections.
	 * @param nmsThreshold IOU threshold for Non-Maximum Suppression.
	 */
	OnnxModel(const std::string& modelPath,
			  int inputWidth, int inputHeight,
			  float confidenceThreshold = 0.5f,
			  float nmsThreshold = 0.45f);

	/**
	 * @brief Destructor.
	 */
	~OnnxModel() = default;

	/**
	 * @brief Performs object detection on the input image.
	 * @param image Input image (BGR, cv::Mat).
	 * @return A vector of Detection results.
	 */
	std::vector<Detection> detect(const cv::Mat& image);

private:
	/**
	 * @brief Resizes and pads the image to fit the model's expected input size (letterbox style).
	 * @param image Input image.
	 * @param out Output letterboxed image.
	 * @param r Scale ratio used.
	 * @param dw Horizontal padding added.
	 * @param dh Vertical padding added.
	 * @param color Padding color (default 114,114,114).
	 */
	void letterbox(const cv::Mat& image, cv::Mat& out,
				   float& r, int& dw, int& dh,
				   cv::Scalar color = cv::Scalar(114, 114, 114)) const;

	/**
	 * @brief Converts the image into a tensor suitable for inference.
	 * @param image Input image.
	 * @param inputTensor Output flattened tensor.
	 * @param r Output scale ratio.
	 * @param dw Output horizontal padding.
	 * @param dh Output vertical padding.
	 */
	void preprocess(const cv::Mat& image, std::vector<float>& inputTensor,
					float& r, int& dw, int& dh) const;

	/**
	 * @brief Runs inference on the input tensor.
	 * @param inputTensor Input data in flattened tensor format.
	 * @return Output tensor data as a float vector.
	 */
	std::vector<float> infer(const std::vector<float>& inputTensor);

	/**
	 * @brief Converts raw model output into structured detection results.
	 * @param output Raw model output.
	 * @param r Scale ratio used during preprocessing.
	 * @param dw Horizontal padding.
	 * @param dh Vertical padding.
	 * @param shape Output tensor shape.
	 * @return Vector of Detection results.
	 */
	std::vector<Detection> postprocess(const std::vector<float>& output,
									   float r, int dw, int dh,
									   const std::vector<int64_t>& shape) const;

	/**
	 * @brief Applies Non-Maximum Suppression to filter overlapping boxes.
	 * @param boxes List of bounding boxes.
	 * @param scores Associated confidence scores.
	 * @param iouThresh IOU threshold for suppression.
	 * @param keep Indices of boxes kept after suppression.
	 */
	static void nms(const std::vector <cv::Rect>& boxes,
				    const std::vector<float>& scores,
				    float iouThresh, std::vector<int>& keep);

	// --- Model parameters ---
	int inputWidth_;			///< Model input width.
	int inputHeight_;			///< Model input height.
	float confidenceThreshold_;	///< Minimum confidence to keep detections.
	float nmsThreshold_;  		///< IOU threshold for Non-Maximum Suppression.

	// --- ONNX Runtime resources ---
	Ort::Env env_;			///< ONNX Runtime environment.
	Ort::Session session_;  ///< ONNX Runtime session for inference.
	Ort::SessionOptions sessionOptions_; ///< ONNX session options.

	std::vector <std::string> inputNames_; ///< Names of model input nodes.
	std::vector<std::string> outputNames_; ///< Names of model output nodes.
};

