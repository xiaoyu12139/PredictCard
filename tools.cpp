#include <vector>
#include <utility>
#include <cmath>
#include <opencv2/opencv.hpp>
#include <optional>
#include "tools.h"


// 计算多边形面积（Shoelace 公式）
// points: 按顺/逆时针顺序排列的顶点列表
// 返回值: 多边形的正面积
double polygon_area(const std::vector<std::pair<double, double>>& points) {
    double area = 0.0;
    const int n = static_cast<int>(points.size());
    for (int i = 0; i < n; ++i) {
        double x1 = points[i].first;
        double y1 = points[i].second;
        double x2 = points[(i + 1) % n].first;
        double y2 = points[(i + 1) % n].second;
        area += x1 * y2 - x2 * y1;
    }
    return std::abs(area) * 0.5;
}


/**
 * @brief 计算两条直线的交点
 * @param c1 直线1 的系数 (A1, B1, C1)
 * @param c2 直线2 的系数 (A2, B2, C2)
 * @return std::optional<cv::Point2d>：如果两直线不平行，则返回交点，否则返回 std::nullopt
 */
std::optional<cv::Point2d> intersect_point(const LineCoeff& c1, const LineCoeff& c2) {
    // 行列式 D = A1*B2 - A2*B1
    double D = c1.A * c2.B - c2.A * c1.B;
    if (std::abs(D) < 1e-6) {
        // 平行或重合，无唯一交点
        return std::nullopt;
    }
    // 克拉默法则求解：
    double x = (c2.B * (-c1.C) - c1.B * (-c2.C)) / D;
    double y = (c1.A * (-c2.C) - c2.A * (-c1.C)) / D;
    return cv::Point2d(x, y);
}

/// 将两点表示的线段转换为直线一般式 Ax + By + C = 0 的系数
LineCoeff line_to_coeff(const Line& line) {
    double x1 = line[0], y1 = line[1];
    double x2 = line[2], y2 = line[3];
    LineCoeff c;
    c.A = y2 - y1;
    c.B = x1 - x2;
    c.C = x2 * y1 - x1 * y2;
    return c;
}

/// 判断两条线段是否近似共线
bool is_lines_colinear(const Line& l1,
    const Line& l2,
    double angle_thresh = 5.0,
    double dist_thresh = 10.0) {
    // 端点
    double x1 = l1[0], y1 = l1[1], x2 = l1[2], y2 = l1[3];
    double x3 = l2[0], y3 = l2[1], x4 = l2[2], y4 = l2[3];

    // 1) 零长度检查
    double dx1 = x2 - x1, dy1 = y2 - y1;
    double dx2 = x4 - x3, dy2 = y4 - y3;
    if ((dx1 == 0 && dy1 == 0) || (dx2 == 0 && dy2 == 0)) {
        return false;
    }

    // 2) 角度差
    double mag1 = std::hypot(dx1, dy1);
    double mag2 = std::hypot(dx2, dy2);
    double dot = dx1 * dx2 + dy1 * dy2;
    double cos_angle = dot / (mag1 * mag2);
    // clamp
    cos_angle = std::max(-1.0, std::min(1.0, cos_angle));
    double angle_diff = std::acos(std::abs(cos_angle)) * 180.0 / CV_PI;
    if (angle_diff > angle_thresh) {
        return false;
    }

    // 3) 距离：l2 的一个端点到 l1 对应直线的距离
    LineCoeff c1 = line_to_coeff(l1);
    double dist = std::abs(c1.A * x3 + c1.B * y3 + c1.C) /
        std::hypot(c1.A, c1.B);
    if (dist > dist_thresh) {
        return false;
    }

    return true;
}

/// 从一堆线段中去除近似共线的冗余线
std::vector<Line> adjust_lines(const std::vector<Line>& lines) {
    std::vector<Line> res;
    for (auto const& ln : lines) {
        bool flag = false;
        for (auto const& existing : res) {
            if (is_lines_colinear(existing, ln)) {
                flag = true;
                break;
            }
        }
        if (!flag) {
            res.push_back(ln);
        }
    }
    return res;
}


cv::Mat crop_with_margin(const cv::Mat& img,
    int left, int top,
    int right, int bottom) {
    // 计算新的 ROI 坐标
    int x = std::clamp(left, 0, img.cols);
    int y = std::clamp(top, 0, img.rows);
    int w = img.cols - x - std::clamp(right, 0, img.cols - x);
    int h = img.rows - y - std::clamp(bottom, 0, img.rows - y);

    // 如果宽或高不合法，则返回空 Mat
    if (w <= 0 || h <= 0) {
        return cv::Mat();
    }

    // 裁剪并深拷贝
    cv::Rect roi(x, y, w, h);
    return img(roi).clone();
}


/// @brief 裁剪掉图像的下方 1/3，只保留上方 2/3 区域。
/// @param img 待裁剪的 BGR（或灰度）图像
/// @return 裁剪后的图像，上方 2/3 区域（深拷贝）
cv::Mat crop_bottom_third(const cv::Mat& img) {
    // 1. 获取图像高和宽
    int h = img.rows;
    int w = img.cols;
    // 2. 计算要保留的高度（上方 2/3）
    int new_h = static_cast<int>(h * 2.0 / 3.0);
    // 3. 定义 ROI：从 (0,0) 到 (w, new_h)
    cv::Rect roi(0, 0, w, new_h);
    // 4. 裁剪并深拷贝返回
    return img(roi).clone();
}


/**
 * 按阅读顺序（逐行从左到右）排序轮廓。
 * @param contours 输入的轮廓列表（vector of vector<Point>）。
 * @param row_tol_ratio 行分组的垂直容差系数（例如 0.5 表示允许相差 50% 行高）。
 * @return 排序后的轮廓列表。
 */
vector<vector<cv::Point>> sort_contours_reading_order(const vector<vector<cv::Point>>& contours, double row_tol_ratio) {
    if (contours.empty())
        return {};

    // 步骤1：计算每个轮廓的外接矩形和中心点坐标
    struct ContourInfo { int idx; double centerY; double centerX; int height; };
    vector<ContourInfo> info;
    info.reserve(contours.size());
    for (int i = 0; i < (int)contours.size(); ++i) {
        cv::Rect bb = cv::boundingRect(contours[i]);
        double cx = bb.x + bb.width * 0.5;
        double cy = bb.y + bb.height * 0.5;
        info.push_back({ i, cy, cx, bb.height });
    }

    // 步骤2：按中心Y坐标升序排序
    sort(info.begin(), info.end(), [](const ContourInfo& a, const ContourInfo& b) {
        return a.centerY < b.centerY;
        });

    // 步骤3：遍历排序后的轮廓，以垂直容差划分行
    vector<vector<int>> rows;
    int ptr = 0;
    while (ptr < (int)info.size()) {
        vector<int> row;
        // 新行基准：使用当前轮廓的Y和高度
        double rowY = info[ptr].centerY;
        double rowH = info[ptr].height;
        row.push_back(info[ptr].idx);
        ptr++;
        // 将后续轮廓中，如果中心Y与行基准差 <= row_tol_ratio*行高，则归入此行
        while (ptr < (int)info.size()) {
            double dy = fabs(info[ptr].centerY - rowY);
            if (dy <= row_tol_ratio * rowH) {
                row.push_back(info[ptr].idx);
                // 更新行高为该行中最大的高度
                rowH = max(rowH, (double)info[ptr].height);
                ptr++;
            }
            else {
                break;
            }
        }
        rows.push_back(row);
    }

    // 步骤4：对每行内部按中心X坐标排序，并合并结果
    vector<vector<cv::Point>> sortedContours;
    for (auto& row : rows) {
        sort(row.begin(), row.end(), [&](int a, int b) {
            cv::Rect ra = cv::boundingRect(contours[a]);
            cv::Rect rb = cv::boundingRect(contours[b]);
            double ax = ra.x + ra.width * 0.5;
            double bx = rb.x + rb.width * 0.5;
            return ax < bx;
            });
        for (int idx : row) {
            sortedContours.push_back(contours[idx]);
        }
    }
    return sortedContours;
}

string predictNum(cv::Mat &img){
    cv::Mat gray;
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    cv::Mat thresh;
    cv::threshold(
        gray,               // 输入灰度图
        thresh,             // 输出二值图
        0,                  // 阈值（与 OTSU 配合时设为 0）
        255,                // 最大值
        cv::THRESH_BINARY_INV | cv::THRESH_OTSU
        );
    cv::Ptr<cv::ml::KNearest> model_pixel = cv::Algorithm::load<cv::ml::KNearest>("num_knn_pixel.yml");
    cv::Mat temp;
    cv::resize(thresh, temp, cv::Size(30, 40));
    cv::Mat vec1;
    vec1.push_back(temp.reshape(0, 1));
    vec1.convertTo(vec1, CV_32F);
    int r1 = model_pixel->predict(vec1);   //对所有行进行预测
    switch (r1){
    case 1:
        return "A";
    case 11:
        return "J";
    case 12:
        return "Q";
    case 13:
        return "K";
    default:
        return to_string(r1);
    }
    return "";
}

string predictFlag(cv::Mat &img){
    cv::Mat gray;
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    cv::Mat thresh;
    cv::threshold(
        gray,               // 输入灰度图
        thresh,             // 输出二值图
        0,                  // 阈值（与 OTSU 配合时设为 0）
        255,                // 最大值
        cv::THRESH_BINARY_INV | cv::THRESH_OTSU
        );
    cv::Ptr<cv::ml::KNearest> model_pixel = cv::Algorithm::load<cv::ml::KNearest>("flag_knn_pixel.yml");
    cv::Mat temp;
    resize(thresh, temp, cv::Size(30, 30));
    cv::Mat vec1;
    vec1.push_back(temp.reshape(0, 1));
    vec1.convertTo(vec1, CV_32F);
    int r1 = model_pixel->predict(vec1);   //对所有行进行预测
    switch (r1){
    case 1:
        return "红桃";
    case 2:
        return "方块";
    case 3:
        return "梅花";
    case 4:
        return "黑桃";
    default:
        break;
    }
    return "";
}
