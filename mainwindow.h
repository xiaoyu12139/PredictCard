#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <QDateTime>
#include <opencv2/opencv.hpp>
#include <opencv2/freetype.hpp>

using namespace std;


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void imageCard(cv::Mat img, cv::Mat *card, cv::Mat *num_flag, string *num, string *flag);
    QImage scaleToFitHalf(const QImage& src, const QImage& ref);
    void ensureDirExists(const QString &path);

public slots:
    void updateFrame();  // 槽：读取并处理下一帧

private:
    Ui::MainWindow *ui;
    QLabel *videoLabel;          // 用于显示图像的标签
    QLabel *cardLabel;
    QLabel *numflagLabel;
    QTimer *timer;               // 定时器，用于触发帧更新
    cv::VideoCapture cap;        // OpenCV 视频捕获对象
    cv::Rect rectROI; //识别框
    QString timestamp;
    cv::Ptr<cv::freetype::FreeType2> ft2;
};
#endif // MAINWINDOW_H
