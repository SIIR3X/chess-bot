#include "OnnxModel.h"
#include <numeric>

OnnxModel::OnnxModel(const std::string& modelPath,
					 int inputWidth, int inputHeight,
					 float confidenceThreshold,
					 float nmsThreshold)
	: env_(ORT_LOGGING_LEVEL_WARNING, "OnnxModel"),
	  sessionOptions_{},
	  session_(nullptr),
	  inputWidth_(inputWidth),
	  inputHeight_(inputHeight),
	  confidenceThreshold_(confidenceThreshold),
	  nmsThreshold_(nmsThreshold)
{
	std::wstring wModelPath(modelPath.begin(), modelPath.end());

	// Initialize ONNX Runtime session
	session_ = Ort::Session(env_, wModelPath.c_str(), sessionOptions_);

	// Retrieve model input and output names
	Ort::AllocatorWithDefaultOptions allocator;
	inputNames_.clear();
	outputNames_.clear();

	for (size_t i = 0; i < session_.GetInputCount(); ++i) {
		auto name = session_.GetInputNameAllocated(i, allocator);
		inputNames_.emplace_back(name.get());
	}
	for (size_t i = 0; i < session_.GetOutputCount(); ++i) {
		auto name = session_.GetOutputNameAllocated(i, allocator);
		outputNames_.emplace_back(name.get());
	}
}

std::vector<Detection> OnnxModel::detect(const cv::Mat& image)
{
	std::vector<float> inputTensor(3 * inputHeight_ * inputWidth_);

	float r;
	int dw, dh;

	// Preprocess the image
	preprocess(image, inputTensor, r, dw, dh);

	// Run inference
	auto output = infer(inputTensor);

	// Get output shape information
	auto shape_info = session_.GetOutputTypeInfo(0).GetTensorTypeAndShapeInfo();
	auto shape = shape_info.GetShape();

	// Postprocess the output to get detections
	return postprocess(output, r, dw, dh, shape);
}

void OnnxModel::letterbox(const cv::Mat& image, cv::Mat& out,
						  float& r, int& dw, int& dh,
						  cv::Scalar color) const
{
	int width = image.cols;
	int height = image.rows;

	// Comput resize ratio to fit input size
	r = std::min(inputWidth_ / (float)width, inputHeight_ / (float)height);

	// Compute new dimensions without padding
	int newUnpadW = int(round(width * r));
	int newUnpadH = int(round(height * r));

	// Compute padding to center the image
	dw = inputWidth_ - newUnpadW;
	dh = inputHeight_ - newUnpadH;
	dw /= 2;
	dh /= 2;

	// Resize the image and add padding
	cv::Mat resized;
	cv::resize(image, resized, cv::Size(newUnpadW, newUnpadH));
	cv::copyMakeBorder(resized, out, dh, inputHeight_ - newUnpadH - dh, dw, inputWidth_ - newUnpadW - dw, cv::BORDER_CONSTANT, color);
}

void OnnxModel::preprocess(const cv::Mat& image, std::vector<float>& inputTensor,
						   float& r, int& dw, int& dh) const
{
	cv::Mat imageLetterbox;
	letterbox(image, imageLetterbox, r, dw, dh);

	// Convert BGR to RGB
	cv::Mat imageRgb;
	cv::cvtColor(imageLetterbox, imageRgb, cv::COLOR_BGR2RGB);

	// Normalize the image to [0, 1] range
	imageRgb.convertTo(imageRgb, CV_32F, 1.0 / 255.0);


	// Convert from HWC to CHW format and flatten the tensor
	std::vector<cv::Mat> chw(3);
	for (int i = 0; i < 3; ++i) {
		chw[i] = cv::Mat(inputHeight_, inputWidth_, CV_32F, inputTensor.data() + i * inputHeight_ * inputWidth_);
	}
	cv::split(imageRgb, chw);
}

std::vector<float> OnnxModel::infer(const std::vector<float>& inputTensor)
{
	std::array<int64_t, 4> inputShape{ 1, 3, inputHeight_, inputWidth_ };
	size_t inputTensorSize = 1 * 3 * inputHeight_ * inputWidth_;

	// Allocate memory for input tensor on the CPU
	Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

	// Create an Ort::Value for the input tensor
	Ort::Value inputOrt = Ort::Value::CreateTensor<float>(
		memoryInfo, const_cast<float*>(inputTensor.data()), inputTensorSize, inputShape.data(), inputShape.size()
	);

	// Convert input/output names to C-style strings
	std::vector<const char*> inputNamesCStr, outputNamesCStr;
	for (const auto& s : inputNames_) inputNamesCStr.push_back(s.c_str());
	for (const auto& s : outputNames_) outputNamesCStr.push_back(s.c_str());

	// Run inference
	auto outputTensors = session_.Run(
		Ort::RunOptions{ nullptr },
		inputNamesCStr.data(), &inputOrt, 1,
		outputNamesCStr.data(), 1
	);

	// Extract float data from the output tensor
	float* outputData = outputTensors[0].GetTensorMutableData<float>();
	size_t outputSize = outputTensors[0].GetTensorTypeAndShapeInfo().GetElementCount();

	return std::vector<float>(outputData, outputData + outputSize);
}

std::vector<Detection> OnnxModel::postprocess(const std::vector<float>& output, float r, int dw, int dh, const std::vector<int64_t>& shape) const
{
	std::vector<cv::Rect> boxes;
	std::vector<float> scores;
	std::vector<int> classIds;

	int numAttrs = shape[1]; // Number of attributes per prediction
	int numPreds = shape[2];
	int numClasses = numAttrs > 5 ? numAttrs - 5 : 0;

	// Iterate through all predictions
	for (int i = 0; i < numPreds; ++i) {
		float cx = output[0 * numPreds + i];
		float cy = output[1 * numPreds + i];
		float w = output[2 * numPreds + i];
		float h = output[3 * numPreds + i];
		float objectness = output[4 * numPreds + i];

		float finalScore = 0.0f;
		int classId = 0;

		// If class scores exist, combine with objectness
		if (numClasses > 0) {
			float bestClassScore = 0.0f;

			for (int c = 0; c < numClasses; ++c) {
				float classScore = output[(5 + c) * numPreds + i];
				float conf = objectness * classScore;

				if (conf > bestClassScore) {
					bestClassScore = conf;
					classId = c;
				}
			}
			finalScore = bestClassScore;
		}
		else {
			finalScore = objectness;
		}

		// Apply confidence threshold
		if (finalScore > confidenceThreshold_) {
			int x1 = int((cx - w / 2 - dw) / r);
			int y1 = int((cy - h / 2 - dh) / r);
			int x2 = int((cx + w / 2 - dw) / r);
			int y2 = int((cy + h / 2 - dh) / r);

			boxes.emplace_back(cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2)));
			scores.push_back(finalScore);
			classIds.push_back(classId);
		}
	}

	// Run Non-Maximum Suppression (NMS) to filter overlapping boxes
	std::vector<int> keep;
	nms(boxes, scores, nmsThreshold_, keep);

	// Collect final detections
	std::vector<Detection> detections;
	for (int idx : keep) {
		detections.push_back({ boxes[idx], scores[idx], classIds[idx] });
	}

	return detections;
}

void OnnxModel::nms(const std::vector <cv::Rect>& boxes,
	const std::vector<float>& scores,
	float iouThresh, std::vector<int>& keep) const
{
	std::vector<int> idxs(boxes.size());
	std::iota(idxs.begin(), idxs.end(), 0);

	// Sort indices by scores in descending order
	std::sort(idxs.begin(), idxs.end(), [&](int i, int j) {
		return scores[i] > scores[j];
	});

	while (!idxs.empty()) {
		int idx = idxs[0];
		keep.push_back(idx);
		std::vector<int> tmp;

		// Compare IoU with remaining boxes
		for (size_t i = 1; i < idxs.size(); ++i) {
			float iou = (boxes[idx] & boxes[idxs[i]]).area() / float((boxes[idx] | boxes[idxs[i]]).area());

			if (iou < iouThresh) {
				tmp.push_back(idxs[i]);
			}
		}

		idxs.swap(tmp);
	}
}