#include "mainwindow.h"

#include <QApplication>
#include "image_select.h"
#include "tools.h"
// #include "plot.h"

int main(int argc, char *argv[])
{
    // cv::Mat card = get_card("img/8.jpg");
    // show_mat("card", card);
    // cv::Mat cut = crop_with_margin(card, 0, 10, 0, 0);
    // show_mat("cut-card", cut);
    // vector<cv::Mat> res = get_num_flag(cut, "out");
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
