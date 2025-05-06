#include <iostream>
#include <string>
#include <opencv2\opencv.hpp>

using namespace std;
using namespace cv;
using namespace cv::ml;

void train_pixel()
{
	Mat NumData, NumLabels, FlagData, FlagLabels;
	int trainNum = 40;
	string NumName[] = { "1_", "2_", "3_", "4_", "5_", "6_", "7_", "8_", "9_", "10_", "j_", "q_", "k_" };
	string FlagName[] = { "hongtao_", "fangkuai_", "meihua_", "heitao_" };
	for (int i = 0; i < trainNum * 13; i++) {
		Mat img, tmp;
		string path = "TrainSample\\";
		path.append(NumName[i / trainNum]).append(to_string((i % trainNum))).append(".jpg");
		img = imread(path, 0);
		resize(img, tmp, Size(30, 40));
		NumData.push_back(tmp.reshape(0, 1));  //序列化后放入特征矩阵
		NumLabels.push_back(i / trainNum + 1);  //对应的标注
	}
	NumData.convertTo(NumData, CV_32F); //uchar型转换为cv_32f
	//使用KNN算法
	int K = 5;
	Ptr<TrainData> tData = TrainData::create(NumData, ROW_SAMPLE, NumLabels);
	Ptr<KNearest> NumModel = KNearest::create();
	NumModel->setDefaultK(K);
	NumModel->setIsClassifier(true);
	NumModel->train(tData);
	NumModel->save("./num_knn_pixel.yml");
	//-----------------------------------------------------------------------------------------------
	for (int i = 0; i < trainNum * 4; i++) {
		Mat img, tmp;
		string path = "TrainSample\\";
		path.append(FlagName[i / trainNum]).append(to_string((i % trainNum))).append(".jpg");
		img = imread(path, 0);
		resize(img, tmp, Size(30, 30));
		FlagData.push_back(tmp.reshape(0, 1));  //序列化后放入特征矩阵
		FlagLabels.push_back(i / trainNum + 1);  //对应的标注
	}
	FlagData.convertTo(FlagData, CV_32F); //uchar型转换为cv_32f
	//使用KNN算法
	int L = 5;
	Ptr<TrainData> tFlag = TrainData::create(FlagData, ROW_SAMPLE, FlagLabels);
	Ptr<KNearest> FlagModel = KNearest::create();
	FlagModel->setDefaultK(L);
	FlagModel->setIsClassifier(true);
	FlagModel->train(tFlag);
	FlagModel->save("./flag_knn_pixel.yml");
}

int main() {
	train_pixel();
	return 0;
}