#include "plot.h"
#include "tools.h"
#include <QGuiApplication>
#include <QScreen>

cv::Mat resizeToFitScreen(const cv::Mat& img, int margin = 500) {


    QScreen *screen = QGuiApplication::primaryScreen();
    QSize  s = screen->size();   // width(), height()
    int screen_w = s.width();
    int screen_h = s.height();
    // 获取屏幕分辨率
    // int screen_w = GetSystemMetrics(SM_CXSCREEN);
    // int screen_h = GetSystemMetrics(SM_CYSCREEN);

    // 计算缩放比例（保留一定 margin）
    double scale_w = (screen_w - margin) / static_cast<double>(img.cols);
    double scale_h = (screen_h - margin) / static_cast<double>(img.rows);
    double scale = std::min(scale_w, scale_h);

    // 若图像小于屏幕则无需缩放
    if (scale >= 1.0)
        return img.clone();

    // 缩放图像
    cv::Mat resized;
    cv::resize(img, resized, cv::Size(), scale, scale, cv::INTER_AREA);
    return resized;
}

cv::Mat tile_images(const std::vector<std::vector<cv::Mat>>& grid) {
    std::vector<cv::Mat> rows;
    for (const auto& row_imgs : grid) {
        // 如果图像大小不一致，先 resize 成统一大小
        std::vector<cv::Mat> resized_row;
        for (const auto& img : row_imgs) {
            if (img.empty()) continue;
            cv::Mat resized;
            cv::resize(img, resized, cv::Size(200, 200));  // 统一为 200x200
            resized_row.push_back(resized);
        }
        cv::Mat row;
        cv::hconcat(resized_row, row);  // 横向拼接一行
        rows.push_back(row);
    }

    cv::Mat out;
    cv::vconcat(rows, out);  // 多行纵向拼接
    return out;
}

void show_tile_mat(string title, const std::vector<std::vector<cv::Mat>>& grid) {
    cv::Mat tile = tile_images(grid);
    show_mat(title, tile);
}

/// 异步显示：在后台线程里循环 waitKey，让窗口保持响应
void show_mat(const std::string& winname,
    const cv::Mat& img,
    int delay_ms) {
    // 拷贝一份 img（防止原图释放后野指针）
    cv::Mat img_copy = img.clone();
    bool flag = true;
    std::thread([winname, img_copy, delay_ms, flag]() mutable {
        // 先建窗口
        cv::namedWindow(winname, cv::WINDOW_AUTOSIZE);

        // 只要窗口没被用户关闭，就持续刷新
        while (true) {
            // 如果窗口被关掉，VISIBLE 会 <1
            if (cv::getWindowProperty(winname, cv::WND_PROP_VISIBLE) < 1)
                break;
            img_copy = resizeToFitScreen(img_copy);
            QScreen *screen = QGuiApplication::primaryScreen();
            QSize  s = screen->size();   // width(), height()
            int screenWidth = s.width();
            int screenHeight = s.height();
            // int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            // int screenHeight = GetSystemMetrics(SM_CYSCREEN);

            int x = (screenWidth - img_copy.cols) / 2;
            int y = (screenHeight - img_copy.rows) / 2;
            // 显示图像并处理事件
            cv::imshow(winname, img_copy);
            if (flag) {
                cv::moveWindow(winname, x, y);
                flag = false;
            }
            if (cv::waitKey(delay_ms) >= 0)
                break;  // 用户按任意键也退出
        }

        // 用户关闭窗口或按键后，销毁窗口
        cv::destroyWindow(winname);
        }).detach();  // 分离线程，让它后台运行
}

void show_mat_on(const std::string& winname,
    const cv::Mat& img,
    int delay_ms) {
    cv::Mat img_copy = img.clone();
    img_copy = resizeToFitScreen(img_copy);
    QScreen *screen = QGuiApplication::primaryScreen();
    QSize  s = screen->size();   // width(), height()
    int screenWidth = s.width();
    int screenHeight = s.height();
    // int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    // int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenWidth - img_copy.cols) / 2;
    int y = (screenHeight - img_copy.rows) / 2;
    // 显示图像并处理事件
    cv::imshow(winname, img_copy);
    cv::moveWindow(winname, x, y);
    cv::waitKey(delay_ms);
}

// 在图上绘制所有符合面积阈值的轮廓外接矩形，并显示窗口
cv::Mat plot_all_rect(const cv::Mat& img,
    const vector<vector<cv::Point>>& contours) {
    // 1) 将灰度图转换为彩色，以便绘制彩色矩形
    cv::Mat img_copy = img.clone();
    cv::Mat boxed;
    cvtColor(img_copy, boxed, cv::COLOR_GRAY2BGR);

    // 2) 对轮廓按“左上→右下，一行一行”排序
    auto sorted = sort_contours_reading_order(contours);

    // 3) 遍历排序后的轮廓，按面积过滤并画矩形
    int index = 0;
    for (auto& cnt : sorted) {
        double area = contourArea(cnt);
        if (area < 500.0) {
            continue;
        }
        index++;
        cv::Rect r = boundingRect(cnt);
        rectangle(boxed,
            cv::Point(r.x, r.y),
            cv::Point(r.x + r.width, r.y + r.height),
            cv::Scalar(255, 0, 0),  // BGR 里的红色
            1,                  // 线宽
            cv::LINE_AA);           // 抗锯齿
    }
    return boxed;
}


/**
 * @brief 根据给定的线段，在图像上绘制红线并返回一张彩色图。
 *
 * @param title 窗口标题，也会被绘制到图像左上角（可选）。
 * @param img   输入图像，可以是灰度（1 通道）或彩色（3 通道）。
 * @param lines 要绘制的线段列表，每条 line 为 (x1,y1,x2,y2)。
 * @return cv::Mat 绘制完成后的 BGR 彩色图像。
 */
cv::Mat plot_lines(const cv::Mat& img,
    const Lines& lines) {
    cv::Mat boxed;
    // 如果是灰度图，先转成 BGR，否则直接 clone
    if (img.channels() == 1) {
        cv::cvtColor(img, boxed, cv::COLOR_GRAY2BGR);
    }
    else {
        boxed = img.clone();
    }

    // 在图上画每一条红色线段
    for (const auto& ln : lines) {
        cv::Point p1(ln[0], ln[1]), p2(ln[2], ln[3]);
        cv::line(boxed, p1, p2,
            cv::Scalar(0, 0, 255),  // 红色 (B=0,G=0,R=255)
            3,                      // 线宽
            cv::LINE_AA);           // 抗锯齿
    }

    return boxed;
}
