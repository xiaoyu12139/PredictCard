#pragma once
#ifndef PLOT_H
#define PLOT_H
#include <matplot/matplot.h>
#include <opencv2/opencv.hpp>
#include "data.h"

void show_mat(const std::string& winname,
    const cv::Mat& img,
    int delay_ms = 30);

void show_mat_on(const std::string& winname,
    const cv::Mat& img,
    int delay_ms = 30);

cv::Mat plot_lines(const cv::Mat& img,
    const Lines& lines);

cv::Mat plot_all_rect(const cv::Mat& img,
    const vector<vector<cv::Point>>& contours);

void show_tile_mat(string title, const std::vector<std::vector<cv::Mat>>& grid);

#endif
