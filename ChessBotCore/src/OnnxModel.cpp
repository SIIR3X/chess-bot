#include "OnnxModel.h"
#include <numeric>

OnnxModel::OnnxModel(const std::string& modelPath,
					 int inputWidth, int inputHeight,
					 float confidenceThreshold,
					 float nmsThreshold)
	: inputWidth_(inputWidth),
	  inputHeight_(inputHeight),
	  confidenceThreshold_(confidenceThreshold),
	  nmsThreshold_(nmsThreshold),
	  env_(ORT_LOGGING_LEVEL_WARNING, "OnnxModel"),
	  session_(nullptr),
	  sessionOptions_{}
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

	std::transform(inputNames_.begin(), inputNames_.end(),
		std::back_inserter(inputNamesCStr),
		[](const std::string& s) { return s.c_str(); });

	std::transform(outputNames_.begin(), outputNames_.end(),
		std::back_inserter(outputNamesCStr),
		[](const std::string& s) { return s.c_str(); });


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

std::vector<Detection> OnnxModel::postprocess(const std::vector<float>& output,
											  float r, int dw, int dh,
											  const std::vector<int64_t>& shape) const
{
	std::vector<cv::Rect> boxes;
	std::vector<float> scores;
	std::vector<int> classIds;

	const int numAttrs = static_cast<int>(shape[1]); // Attributes per prediction
	const int numPreds = static_cast<int>(shape[2]);
	const int numClasses = numAttrs > 5 ? numAttrs - 5 : 0;

	// Indices for fixed attributes
	constexpr int IDX_CX = 0;
	constexpr int IDX_CY = 1;
	constexpr int IDX_W = 2;
	constexpr int IDX_H = 3;
	constexpr int IDX_OBJECTNESS = 4;
	constexpr int SCORE_OFFSET = 5;

	// Precomputed attribute offsets
	const int offsetCX = IDX_CX * numPreds;
	const int offsetCY = IDX_CY * numPreds;
	const int offsetW = IDX_W * numPreds;
	const int offsetH = IDX_H * numPreds;
	const int offsetObj = IDX_OBJECTNESS * numPreds;

	for (int i = 0; i < numPreds; ++i) {
		// Check if the objectness score is above the threshold
		float objectness = output[offsetObj + i];
		if (objectness < confidenceThreshold_)
			continue;

		float finalScore = objectness;
		int classId = 0;

		// If there are classes, find the best class score
		if (numClasses > 0) {
			float bestClassScore = 0.0f;

			// Iterate through class scores to find the best one
			for (int c = 0; c < numClasses; ++c) {
				int classScoreIndex = (SCORE_OFFSET + c) * numPreds + i;
				float classScore = output[classScoreIndex];
				float conf = objectness * classScore;

				if (conf > bestClassScore) {
					bestClassScore = conf;
					classId = c;
				}
			}

			finalScore = bestClassScore;
		}

		// Check if the final score is above the confidence threshold
		if (finalScore > confidenceThreshold_) {
			float cx = output[offsetCX + i];
			float cy = output[offsetCY + i];
			float w = output[offsetW + i];
			float h = output[offsetH + i];

			int x1 = static_cast<int>((cx - w / 2 - dw) / r);
			int y1 = static_cast<int>((cy - h / 2 - dh) / r);
			int x2 = static_cast<int>((cx + w / 2 - dw) / r);
			int y2 = static_cast<int>((cy + h / 2 - dh) / r);

			boxes.emplace_back(cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2)));
			scores.push_back(finalScore);
			classIds.push_back(classId);
		}
	}

	// Apply Non-Maximum Suppression (NMS)
	std::vector<int> keep;
	nms(boxes, scores, nmsThreshold_, keep);

	// Collect final detections
	std::vector<Detection> detections;
	detections.reserve(keep.size());
	std::transform(keep.begin(), keep.end(), std::back_inserter(detections),
		[&](int idx) {
			return Detection{ boxes[idx], scores[idx], classIds[idx] };
		});

	return detections;
}

void OnnxModel::nms(const std::vector <cv::Rect>& boxes,
	const std::vector<float>& scores,
	float iouThresh, std::vector<int>& keep)
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