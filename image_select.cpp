#pragma execution_character_set("utf-8")
#include <opencv2/opencv.hpp>
#include "plot.h"
#include <vector>
#include "image_select.h"
#include "tools.h"


/// @brief 对扑克牌图像做透视矫正，输出 300×450 的俯视图。
/// @param image   原始 BGR 图像
/// @param corners 四角点坐标，顺时针顺序：左上、右上、右下、左下
/// @return 矫正后的扑克牌图像 (300×450)
cv::Mat warp_card(const cv::Mat& image,
                  const vector<cv::Point2f> &corners) {
    // 1. 固定输出尺寸
    const int width = 300;
    const int height = 450;

    // 2. 构造源点和目标点
    CV_Assert(corners.size() == 4);
    // srcPts 必须是 CV_32F
    cv::Mat srcPts(4, 1, CV_32FC2);
    for (int i = 0; i < 4; ++i) {
        srcPts.at<cv::Point2f>(i, 0) = corners[i];
    }
    std::vector<cv::Point2f> dst = {
        {0.f,       0.f      },
        {float(width - 1), 0.f      },
        {float(width - 1), float(height - 1)},
        {0.f,       float(height - 1)}
    };
    cv::Mat dstPts(dst);

    // 3. 计算透视变换矩阵
    cv::Mat M = cv::getPerspectiveTransform(srcPts, dstPts);

    // 4. 执行透视变换
    cv::Mat warped;
    cv::warpPerspective(image, warped, M, cv::Size(width, height));

    return warped;
}


/// @brief 根据四个角点裁剪包含该四边形的最小轴对齐矩形子图
/// @param image   原始图像（BGR 或灰度）
/// @param corners 四角点列表，类型为 std::vector<cv::Point> 或等价的 int 坐标对
/// @return std::pair<cv::Mat, cv::Rect>
///         first  = 裁剪出的子图 (深拷贝)
///         second = 裁剪框的左上角坐标及宽高 (x, y, w, h)
std::pair<cv::Mat, cv::Rect> crop_quad_bbox(
    const cv::Mat& image,
    const std::vector<cv::Point>& corners)
{
    // 1) 计算最小轴对齐外接矩形
    cv::Rect bbox = cv::boundingRect(corners);

    // 2) 裁剪 ROI 并深拷贝
    cv::Mat cropped = image(bbox).clone();

    return { cropped, bbox };
}



/// @brief 从一组 HoughLinesP 输出的线段中筛选出近似水平和近似垂直的线段
/// @param lines  输入：HoughLinesP 得到的线段列表
/// @param horiz  输出：筛选出的“近水平”线段
/// @param vert   输出：筛选出的“近垂直”线段
/// @param angle_thresh_deg  水平判断阈值（度），默认 15°
void get_vh_lines(const Lines& lines,
    std::vector<Line>& horiz,
    std::vector<Line>& vert,
    double angle_thresh_deg = 15.0) {
    double ang_low = angle_thresh_deg;
    double ang_high = 90.0 - angle_thresh_deg;
    for (const auto& ln : lines) {
        int x1 = ln[0], y1 = ln[1], x2 = ln[2], y2 = ln[3];
        // 计算线段与水平轴的夹角 (0~180)
        double theta = std::atan2(double(y2 - y1), double(x2 - x1)) * 180.0 / CV_PI;
        double angle = std::abs(theta);
        if (angle > 90.0)
            angle = 180.0 - angle;  // 归一到 [0,90]

        if (angle < ang_low) {
            // 近水平
            horiz.push_back(ln);
        }
        else if (angle > ang_high) {
            // 近垂直
            vert.push_back(ln);
        }
    }
}


/// 从水平线集 horiz 和垂直线集 vert 中挑出最外侧的四条边，
/// 并计算它们的四个交点（左上、右上、右下、左下）
///
/// @param horiz 若干近水平的线段
/// @param vert  若干近垂直的线段
/// @return 顺时针排列的四个角点：左上、右上、右下、左下
std::vector<cv::Point2f> find_largest_quad(std::vector<Line> horiz,
    std::vector<Line> vert) {
    if (horiz.size() < 2 || vert.size() < 2) {
        throw runtime_error("There are not enough horizontal or vertical lines to form a quadrilateral.");
    }

    // 1) 水平线按中点 y 排序，最上和最下
    std::sort(horiz.begin(), horiz.end(),
        [](const Line& a, const Line& b) {
            return (a[1] + a[3]) < (b[1] + b[3]);
        });
    Line top_line = horiz.front();
    Line bottom_line = horiz.back();

    // 2) 垂直线按中点 x 排序，最左和最右
    std::sort(vert.begin(), vert.end(),
        [](const Line& a, const Line& b) {
            return (a[0] + a[2]) < (b[0] + b[2]);
        });
    Line left_line = vert.front();
    Line right_line = vert.back();

    // 3) 计算这四条直线的一般式系数
    LineCoeff c_top = line_to_coeff(top_line);
    LineCoeff c_bot = line_to_coeff(bottom_line);
    LineCoeff c_left = line_to_coeff(left_line);
    LineCoeff c_right = line_to_coeff(right_line);

    // 4) 求交点：左上、右上、右下、左下
    auto opt_lt = intersect_point(c_top, c_left);
    auto opt_rt = intersect_point(c_top, c_right);
    auto opt_rb = intersect_point(c_bot, c_right);
    auto opt_lb = intersect_point(c_bot, c_left);

    if (!opt_lt || !opt_rt || !opt_rb || !opt_lb) {
        throw std::runtime_error("直线平行或无法求交点");
    }

    std::vector<cv::Point2f> corners = {
        cv::Point2f(opt_lt->x, opt_lt->y),
        cv::Point2f(opt_rt->x, opt_rt->y),
        cv::Point2f(opt_rb->x, opt_rb->y),
        cv::Point2f(opt_lb->x, opt_lb->y)
    };
    return corners;
}

std::vector<cv::Point2f> get_card_rect(cv::Mat img){
    cv::Mat gray, blur;
    cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    medianBlur(gray, blur, 7);

    // 2. 边缘 & 闭运算
    cv::Mat edges, closed;
    Canny(blur, edges, 50, 150);
    cv::Mat kern = getStructuringElement(cv::MORPH_RECT, cv::Size(7, 7));
    morphologyEx(edges, closed, cv::MORPH_CLOSE, kern);

    // 3. HoughLinesP → 筛选水平/垂直
    Lines lines;
    HoughLinesP(closed, lines, 1, CV_PI / 180, 80, 20, 50);
    Lines horiz, vert;
    get_vh_lines(lines, horiz, vert);

    // 4. 预览
    // cv::Mat for_show1, for_show2, for_show3;
    // for_show1 = plot_lines(img, lines);
    //show_mat("所有线", for_show);
    vector<Line> vh = horiz;
    vh.insert(vh.end(), vert.begin(), vert.end());
    // for_show2=plot_lines(img, vh);
    //show_mat("垂直&水平", for_show);

    // 5. 去重 & 再预览
    horiz = adjust_lines(horiz);
    vert = adjust_lines(vert);
    vh = horiz; vh.insert(vh.end(), vert.begin(), vert.end());
    // for_show3=plot_lines(img, vh);
    //show_mat("去重后V&H", for_show);

    // show_tile_mat("预处理", {{img,for_show1}, {for_show2,for_show3}});

    // 6. 找四角 & 画轮廓
    auto corners = find_largest_quad(horiz, vert);
    return corners;
}

cv::Mat get_card(string path) {
    // 1. 读图 & 预处理
    cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
    if (img.empty()) { cerr << "读图失败\n"; return img;}

    // 6. 找四角 & 画轮廓
    auto corners = get_card_rect(img);
    cv::Mat copy = img.clone();
    vector<cv::Point> poly;
    for (auto& p : corners) poly.emplace_back(cvRound(p.x), cvRound(p.y));
    polylines(copy, vector<vector<cv::Point>>{poly}, true,
        cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
    for (auto& p : poly)
        circle(copy, p, 5, cv::Scalar(0, 0, 255), cv::FILLED);
    // show_mat("画轮廓", copy);

    // 7. 透视矫正 & 保存
    cv::Mat card = warp_card(img, corners);
    return card;
}



vector<cv::Mat> get_num_flag(std::string path, string outpath) {
    // 1. 读取 & 二值化
    cv::Mat src = cv::imread(path);
    if (src.empty()) {
        cerr << "无法读取 img/test.jpg\n";
        return vector<cv::Mat>();
    }
    return get_num_flag(src, outpath);
}

unordered_map<string,cv::Rect> get_num_flag_rect(cv::Mat img){
    img = crop_bottom_third(img);

    cv::Mat gray, thresh;
    cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    threshold(gray, thresh, 0, 255,
              cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    //查找外部轮廓
    vector<vector<cv::Point>> contours;
    findContours(thresh.clone(), contours,
                 cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    // cv::Mat forshow1;
    // forshow1 = plot_all_rect(thresh, contours);

    unordered_map<std::string,cv::Rect> res;

    //遍历所有轮廓，取 y 值最小的那个
    float min_y = FLT_MAX;
    cv::Rect num_rect;
    double num_area = 0;
    bool found_num = false;

    for (auto& cnt : contours) {
        cv::Rect r = boundingRect(cnt);
        double area = contourArea(cnt);
        if (area < 500) continue;
        if (r.y < min_y) {
            min_y = static_cast<float>(r.y);
            num_rect = r;
            num_area = area;
            found_num = true;
            res["num"] = num_rect;
        }
    }

    // 进一步找到“flag”框：面积 > num_area 的最大那个
    cv::Rect flag_rect;
    if (contours.size() >= 2 && found_num) {
        //遍历排序后的轮廓，找到大于num的第一个轮廓
        auto sorted = sort_contours_reading_order(contours);
        for (auto& cnt : sorted) {
            double area = contourArea(cnt);
            if (area > num_area) {
                flag_rect = boundingRect(cnt);
                res["flag"] = flag_rect;
                break;
            }
        }
    }
    return res;
}

vector<cv::Mat> get_num_flag(cv::Mat img, string outpath) {
    unordered_map<string, cv::Rect> map = get_num_flag_rect(img);
    auto it = map.find("flag");
    bool found_flag = it != map.end();
    it = map.find("num");
    bool found_num = it != map.end();
    //转为 BGR 以便画彩色框
    cv::Mat boxed;
    cvtColor(img, boxed, cv::COLOR_GRAY2BGR);

    // 如果找到了，就画框并保存 ROI
    vector<cv::Mat> res;
    if (found_num) {
        cv::Rect num_rect = map["num"];
        rectangle(boxed, num_rect, cv::Scalar(0, 255, 0), 1, cv::LINE_AA);
        cv::Mat roi_num = img(num_rect).clone();
        imwrite(outpath + "/card_number.png", roi_num);
    }
    if (found_flag) {
        cv::Rect flag_rect = map["flag"];
        rectangle(boxed, flag_rect, cv::Scalar(0, 0, 255), 1, cv::LINE_AA);
        cv::Mat roi_flag = img(flag_rect).clone();
        cv::imwrite(outpath + "/card_symbol.png", roi_flag);
    }
    // show_tile_mat("画框", { {forshow1,boxed} });
    return res;
}
