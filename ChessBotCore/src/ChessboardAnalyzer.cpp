#include "pch.h"
#include "ChessboardAnalyzer.h"
#include "ImageUtils.h"

ChessboardAnalyzer::ChessboardAnalyzer()
	: boardDetector_("models/best.onnx", 0.5f, 0.5f),
	pieceDetector_("models/piece_detector.onnx", 0.5f, 0.5f)
{}

AnalysisResult ChessboardAnalyzer::analyze(const cv::Mat& image)
{
	AnalysisResult result;

	// Detect the chessboard in the image
	result.boardImage = detectBoard(image);
	if (result.boardImage.empty()) {
		return result; // No board detected
	}

	// Detect the pieces on the chessboard
	result.pieces = detectPieces(result.boardImage);

	return result;
}

cv::Mat ChessboardAnalyzer::detectBoard(const cv::Mat& image)
{
	auto boardDetections = boardDetector_.detect(image);
	if (boardDetections.empty()) {
		return cv::Mat();
	}

	// Find the largest board detection
	Detection largestBoard = getLargestDetection(boardDetections);

	return ImageUtils::crop(image, largestBoard.bbox);
}

std::vector<PieceInfo> ChessboardAnalyzer::detectPieces(const cv::Mat& boardImage)
{
	std::vector<PieceInfo> result;

	// Detect the pieces on the board
	auto pieceDetections = pieceDetector_.detect(boardImage);
	if (pieceDetections.empty()) {
		return result; // No pieces detected
	}

	// Calculate the size of each square on the chessboard
	const int numSquares = 8;
	int squareWidth = boardImage.cols / numSquares;
	int squareHeight = boardImage.rows / numSquares;

	// Iterate through the detected pieces and map them to the chessboard grid
	for (const auto& detection : pieceDetections) {
		// Calculate the center of the bounding box
		cv::Point center = (detection.bbox.tl() + detection.bbox.br()) * 0.5;

		// Calculate the piece position on the grid
		int col = center.x / squareWidth;
		int row = center.y / squareHeight;

		// Clamp the row and column to valid indices
		col = std::min(std::max(col, 0), 7);
		row = std::min(std::max(row, 0), 7);

		std::string name = classNames_.at(detection.classId);
		std::string square = toSquareNotation(col, row);

		result.push_back({ name, square, {col, row} });
	}

	return result;
}

Detection ChessboardAnalyzer::getLargestDetection(const std::vector<Detection>& detections) const
{
	Detection best;
	float maxArea = 0.0f;
	bool found = false;

	for (const auto& detection : detections) {
		float area = static_cast<float>(detection.bbox.area());

		if (area > maxArea) {
			maxArea = area;
			best = detection;
			found = true;
		}
	}

	return found ? best : Detection{};
}

std::string ChessboardAnalyzer::toSquareNotation(int col, int row) const
{
	char file = 'a' + col;
	char rank = '8' - row;
	return std::string() + file + rank;
}
