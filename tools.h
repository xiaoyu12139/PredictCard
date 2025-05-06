#pragma once
#ifndef TOOLS_H
#define TOOLS_H
#include "data.h"
#include <optional>


double polygon_area(const std::vector<std::pair<double, double>>& points);

std::optional<cv::Point2d> intersect_point(const LineCoeff& c1, const LineCoeff& c2);
LineCoeff line_to_coeff(const Line& line);
std::optional<cv::Point2d> intersect_point(const LineCoeff& c1, const LineCoeff& c2);

Lines adjust_lines(const Lines& lines);

std::vector<std::vector<cv::Point>> sort_contours_reading_order(
    const std::vector<std::vector<cv::Point>>& contours,
    double row_tol_ratio = 0.5);

//图像裁剪
cv::Mat crop_bottom_third(const cv::Mat& img);
cv::Mat crop_with_margin(const cv::Mat& img,
    int left, int top,
    int right, int bottom);

string predictNum(cv::Mat &img);
string predictFlag(cv::Mat &img);

#endif
