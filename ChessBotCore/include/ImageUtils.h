/**
 * @file ImageUtils.h
 * @brief Utility functions for image processing using OpenCV.
 *
 * Provides common image operations such as safe cropping with bounds checking.
 * Designed to be lightweight and usable in ChessBotCore's vision pipeline.
 *
 * @author Lucas
 * @date 2025-06-05
 */

#pragma once

#include "chessbotcore_global.h"

#include <opencv2/opencv.hpp>

/**
 * @class ImageUtils
 * @brief Provides static utility functions for OpenCV image manipulation.
 *
 * Includes helper methods such as cropping an image with automatic bounds checking
 * to ensure safety against out-of-bounds access.
 */
class CHESSBOTCORE_EXPORT ImageUtils
{
public:
	/**
	 * @brief Crops an image to a given region of interest (ROI), with bounds checking.
	 *
	 * This method ensures the ROI is within the image dimensions and returns a
	 * copy of the cropped region. If the ROI is invalid or out-of-bounds, an empty image is returned.
	 *
	 * @param image The source image to crop.
	 * @param roi The region of interest to crop from the image.
	 * @return Cropped image as a cv::Mat. Returns an empty cv::Mat if the ROI is invalid.
	 */
	static cv::Mat crop(const cv::Mat& image, const cv::Rect& roi);
};
