#pragma once

#include "chessbotcore_global.h"

#include "OnnxModel.h"
#include <string>
#include <vector>
#include <opencv2/core.hpp>

struct PieceInfo {
	std::string name;
	std::string square;
	cv::Point gridPos;
};

struct AnalysisResult {
	cv::Mat boardImage;
	std::vector<PieceInfo> pieces;
	bool success = false;
};

const std::vector<std::string> classNames_ = {
	"wp", "wn", "wb", "wr", "wq", "wk",
	"bp", "bn", "bb", "br", "bq", "bk"
};

class CHESSBOTCORE_EXPORT ChessboardAnalyzer
{
public:
	ChessboardAnalyzer();

	~ChessboardAnalyzer() = default;

	AnalysisResult analyze(const cv::Mat& image);

private:
	cv::Mat detectBoard(const cv::Mat& image);

	std::vector<PieceInfo> detectPieces(const cv::Mat& boardImage);

	Detection getLargestDetection(const std::vector<Detection>& detections) const;

	std::string toSquareNotation(int col, int row) const;

	OnnxModel boardDetector_;
	OnnxModel pieceDetector_;
};
