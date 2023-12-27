/*
 * template.hpp
 *
 *  Created on: 26 дек. 2023 г.
 *      Author: vevdokimov
 */

#ifndef TEMPLATES_HPP_
#define TEMPLATES_HPP_

#include <iostream>
#include <string>

#include <fstream>
#include <vector>
#include <sstream>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>

using namespace std;

struct Template
{
	cv::Mat image;
	double match;
	cv::Rect roi;
};

struct TemplateDetectionResult
{
	int template_id;
	cv::Rect found_rect;
	double match;
};

extern std::vector<Template> templates;

void templates_load_config();

void templates_save_config();

void templates_detect(cv::Mat& srcImg, std::vector<TemplateDetectionResult>& detection_results);

#endif /* TEMPLATES_HPP_ */
