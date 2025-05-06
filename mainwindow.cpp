#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "image_select.h"
#include "tools.h"
#include <QDir>



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
    cardLabel(new QLabel(this)),
    numflagLabel(new QLabel(this)),
    videoLabel(new QLabel(this)), timer(new QTimer(this))
{
    ui->setupUi(this);

    // 在初始化时（只做一次）：
    ft2 = cv::freetype::createFreeType2();
    // Windows 下用系统黑体；Linux 下可改成 "/usr/share/fonts/..."
    ft2->loadFontData("C:/Windows/Fonts/simhei.ttf", 0);
    ensureDirExists("flagnum");
    ensureDirExists("out");
    ensureDirExists("flagnum");

    // 设置 QLabel 属性
    videoLabel->setAlignment(Qt::AlignCenter);
    cardLabel->setAlignment(Qt::AlignCenter);
    numflagLabel->setAlignment(Qt::AlignCenter);

    QWidget *central = new QWidget(this);
    auto *hLayout = new QHBoxLayout(central);
    hLayout->addWidget(videoLabel, /*stretch=*/3);
    auto *vLayout = new QVBoxLayout;
    vLayout->addWidget(cardLabel);
    vLayout->addWidget(numflagLabel);
    // 右侧整体给1份伸缩因子
    hLayout->addLayout(vLayout, /*stretch=*/1);
    setCentralWidget(central);

    // 打开默认摄像头 (设备 0)
    cap.open(1);
    if (!cap.isOpened()) {
        videoLabel->setText(QString("摄像头打开失败"));
        return;
    }

    // 每 30 毫秒触发一次更新（约 33 FPS）
    connect(timer, SIGNAL(timeout()), this, SLOT(updateFrame()));
    timer->start(30);
    // timer->start(1000);
}

MainWindow::~MainWindow()
{
    cap.release();
    delete ui;
}

void MainWindow::updateFrame()
{
    cv::Mat frame;
    cv::Mat card, num_flag;
    cap.read(frame);  // 读取摄像头帧
    if (frame.empty()) return;

    //设置rect
    if (rectROI.empty()) {
        // rect 是“空”的
        // 获取帧的宽度和高度
        int frameWidth = frame.cols;
        int frameHeight = frame.rows;

        // 设定目标长宽比（宽:高 = 2:3）
        float aspectWidth = 2.0f;
        float aspectHeight = 3.5f;
        float targetRatio = aspectWidth / aspectHeight;
        float fix = 40.0f * 3;

        // 计算符合目标长宽比的矩形框尺寸，使其尽量大地适应帧的尺寸
        int boxWidth, boxHeight;
        if (frameWidth / static_cast<float>(frameHeight) > targetRatio) {
            // 帧相对较宽，高度为限制条件
            boxHeight = frameHeight - fix;
            boxWidth = static_cast<int>(frameHeight * targetRatio);
        } else {
            // 帧相对较窄，宽度为限制条件
            boxWidth = frameWidth - fix;
            boxHeight = static_cast<int>(frameWidth / targetRatio);
        }

        // 计算矩形框左上角坐标，使其在帧中居中显示
        int boxX = (frameWidth - boxWidth) / 2;
        int boxY = (frameHeight - boxHeight) / 2;

        // 绘制绿色矩形框，线宽为2
        rectROI = cv::Rect(boxX, boxY, boxWidth, boxHeight);
    }else{
        string num,flag;
        timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        QString filename = QString("out/")+ timestamp + QString(".png");
        filename.remove(' ');
        filename.replace(':', '-');
        // qDebug() << filename ;
        cv::imwrite(filename.toStdString(), frame(rectROI));
        try{
            imageCard(frame,&card,&num_flag,&num,&flag);
        }catch(cv::Exception e){
            qDebug() << e.what();
        }catch(std::exception e2){
            qDebug() << e2.what();
        }
        //处理图片显示到界面上
        // cv::flip(frame, frame, /*1=*/1);// —— 这行代码就把画面水平镜像 ——
        // 在图像左上角叠加当前时间戳
        cv::putText(frame, timestamp.toStdString(), cv::Point(10, 30),
                    cv::FONT_HERSHEY_SIMPLEX, 1.0,
                    cv::Scalar(255, 255, 255), 2);

        ft2->putText(frame, "花色: "+flag, cv::Point(10, 30*2),
                    30, cv::Scalar(0, 0, 255),
                    1, cv::LINE_AA,false);
        ft2->putText(frame, "数字: "+num, cv::Point(10, 30*3),
                     30, cv::Scalar(0, 0, 255),
                     1, cv::LINE_AA,false);

        // cv::putText(frame, "num: "+num, cv::Point(10, 30*3),
        //             cv::FONT_HERSHEY_SIMPLEX, 1.0,
        //             cv::Scalar(255, 255, 255), 2);
        //处理frame
        cv::rectangle(frame, rectROI, cv::Scalar(0, 255, 0), 2);
    }

    // 将 BGR 转为 RGB
    cv::Mat rgbFrame,rgbCard,rgbNumflag;
    cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);

    // 将 cv::Mat 转换为 QImage
    QImage image(rgbFrame.data, rgbFrame.cols, rgbFrame.rows, rgbFrame.step, QImage::Format_RGB888);
    QImage image_card;
    QImage image_numflag;
    // 在 QLabel 上显示图像
    videoLabel->setPixmap(QPixmap::fromImage(image));
    if(!card.empty()){
        cv::cvtColor(card, rgbCard, cv::COLOR_BGR2RGB);
        image_card = QImage(rgbCard.data, rgbCard.cols, rgbCard.rows, rgbCard.step, QImage::Format_RGB888);
        image_card = scaleToFitHalf(image_card, image);
        cardLabel->setPixmap(QPixmap::fromImage(image_card));
    }else{
        cardLabel->setText("card获取错误");
    }
    if(!num_flag.empty()){
        cv::cvtColor(num_flag, rgbNumflag, cv::COLOR_BGR2RGB);
        image_numflag = QImage(rgbNumflag.data, rgbNumflag.cols, rgbNumflag.rows, rgbNumflag.step, QImage::Format_RGB888);
        image_numflag = scaleToFitHalf(image_numflag, image);
        numflagLabel->setPixmap(QPixmap::fromImage(image_numflag));
    }else{
        numflagLabel->setText("num flag获取错误");
    }
    int imgw = image.width();
    int imgh = image.height();
    videoLabel->resize(imgw, imgh);  // 可选：调整窗口大小以匹配图像
    cardLabel->resize(imgw/2 + 200, imgh/2 + 200);  // 可选：调整窗口大小以匹配图像
    numflagLabel->resize(imgw/2 + 200, imgh/2 + 200);  // 可选：调整窗口大小以匹配图像
    // qDebug() << "image numflag h:" << image_numflag.height() << " w" << image_numflag.width() << " resize h:" << imgh/2 + 100 << " w" << imgw/2 + 100;
}

void MainWindow::imageCard(cv::Mat img, cv::Mat *card, cv::Mat *num_flag, string *num, string *flag){
    cv::Mat roiShared = img(rectROI);
    cv::Mat roi = roiShared.clone();
    vector<cv::Point2f> card_corners = get_card_rect(roi);
    cv::Mat tmpcard = warp_card(roi, card_corners);
    *card = tmpcard.clone();
    unordered_map<string,cv::Rect> map = get_num_flag_rect(tmpcard);
    auto it_flag = map.find("flag");
    bool found_flag = it_flag != map.end();
    auto it_num = map.find("num");
    bool found_num = it_num != map.end();
    //转为 BGR 以便画彩色框
    cv::Mat boxed = tmpcard.clone();

    // 如果找到了，就画框
    vector<cv::Mat> res;
    if (found_num) {
        cv::Rect num_rect = map["num"];
        rectangle(boxed, num_rect, cv::Scalar(0, 255, 0), 1, cv::LINE_AA);
        cv::Mat roi_num = tmpcard(num_rect).clone();
        *num = predictNum(roi_num);
        QString filename = QString("flagnum/")+ timestamp + QString("_number.png");
        filename.remove(' ');
        filename.replace(':', '-');
        imwrite(filename.toStdString(), roi_num);
        qDebug()<< QString::fromUtf8("找到 num,识别为：")<< QString::fromStdString(*num);
    }
    if (found_flag) {
        cv::Rect flag_rect = map["flag"];
        rectangle(boxed, flag_rect, cv::Scalar(0, 0, 255), 1, cv::LINE_AA);
        cv::Mat roi_num = tmpcard(flag_rect).clone();
        *flag = predictFlag(roi_num);
        QString filename = QString("flagnum/")+ timestamp + QString("_flag.png");
        filename.remove(' ');
        filename.replace(':', '-');
        imwrite(filename.toStdString(), roi_num);
        qDebug() << QString::fromUtf8("找到 flag,识别为：")<< QString::fromStdString(*flag);
    }
    *num_flag = boxed.clone();
}

/**
 * @brief 将 src 等比例缩放，使其完全“放进” ref 宽/2 × 高/2 的区域内
 * @param src   要缩放的图像
 * @param ref   参考图像
 * @return      缩放后的图像
 */
QImage MainWindow::scaleToFitHalf(const QImage& src, const QImage& ref) {
    // 参考图一半的宽高
    int maxW = ref.width()  / 2;
    int maxH = ref.height() / 2;

    // 若尺寸无效或 src 为空，返回空图
    if (maxW <= 0 || maxH <= 0 || src.isNull())
        return QImage();

    // 使用 Qt 的 scaled 方法，保持宽高比，平滑缩放
    return src.scaled(
        maxW,
        maxH,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
        );
}

void MainWindow::ensureDirExists(const QString &path)
{
    QDir dir(path);
    // 判断目录是否存在
    if (!dir.exists()) {
        // 不存在则递归创建（相当于 mkdir -p）
        if (dir.mkpath(".")) {
            qDebug() << "目录创建成功：" << path;
        } else {
            qWarning() << "目录创建失败：" << path;
        }
    } else {
        qDebug() << "目录已存在：" << path;
    }
}
