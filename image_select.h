#pragma once
#ifndef IMAGE_SELECT_H
#define IMAGE_SELECT_H
#include <opencv2/opencv.hpp>
#include "data.h"

using namespace std;
cv::Mat get_card(string path);
vector<cv::Mat> get_num_flag(std::string path, string outpath);
vector<cv::Mat> get_num_flag(cv::Mat img, string outpath);

std::vector<cv::Point2f> get_card_rect(cv::Mat img);
unordered_map<string,cv::Rect> get_num_flag_rect(cv::Mat img);

cv::Mat warp_card(const cv::Mat& image,
                  const vector<cv::Point2f>& corners);

#endif
