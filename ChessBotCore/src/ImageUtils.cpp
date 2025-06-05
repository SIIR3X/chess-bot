#include "pch.h"
#include "ImageUtils.h"

cv::Mat ImageUtils::crop(const cv::Mat& image, const cv::Rect& roi)
{
	// Ensure the ROI is within the bounds of the image
	cv::Rect safeRoi = roi & cv::Rect(0, 0, image.cols, image.rows);

	// If the safe ROI is empty, return an empty Mat
	if (safeRoi.width <= 0 || safeRoi.height <= 0) {
		return cv::Mat();
	}

	// Crop the image using the safe ROI
	return image(safeRoi).clone();
}