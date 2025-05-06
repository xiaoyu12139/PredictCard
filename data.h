#pragma once
#ifndef DATA_H
#define DATA_H
#include <opencv2/opencv.hpp>

using namespace std;

// 直线系数结构体
struct LineCoeff {
    double A;
    double B;
    double C;
};

typedef std::vector<cv::Vec4i> Lines;
using Line = cv::Vec4i;  // (x1, y1, x2, y2)
#endif
