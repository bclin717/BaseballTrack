#include "stdafx.h"



using namespace std;
using namespace cv;

//char* urlOfVideo1 = "C:\\Users\\Kevin\\Desktop\\Baseball Project\\video\\146km.mp4";
char* urlOfVideo1 = "C:\\Users\\ds934\\Desktop\\video\\change.mp4";
Ptr<BackgroundSubtractor> pKNN;

int main() {
	VideoCapture video(urlOfVideo1);
	pKNN = createBackgroundSubtractorKNN(100, 600, false);


	if (!video.isOpened()) return 1;

	Mat videoFrame, temp, mask;
	vector<Mat> videoFrames;
	vector<Mat> framesROI;
	vector<Mat> frontObjectMask;
	vector<Mat> frontObject;
	vector<Mat> whiteFrontObject;
	vector<Mat> whiteFrontObjectMoves;

	while (true) {
		video >> videoFrame;
		if (videoFrame.empty()) {
			video.release();
			break;
		}
		//高斯模糊
		GaussianBlur(videoFrame, videoFrame, Size(5, 5), 2, 2);

		//ROI裁切
		framesROI.push_back(videoFrame.clone()(Rect(292, 280, 500, 155)));
	}

	for (int i = 0; i < framesROI.size(); i++) {
		//前景檢測
		pKNN->apply(framesROI.at(i), temp, 0.6);
		frontObjectMask.push_back(temp.clone());

		//濾鏡回原影片
		framesROI.at(i).copyTo(temp, frontObjectMask.at(i));
		frontObject.push_back(temp.clone());

		//轉HSV、製作mask、疊加
		cvtColor(frontObject.at(i), temp, COLOR_BGR2HSV);
		inRange(temp, Scalar(0, 0, 230), Scalar(180, 80, 255), mask);
		Mat resultOfFrontAndWhite;
		frontObject.at(i).copyTo(resultOfFrontAndWhite, mask);
		cvtColor(resultOfFrontAndWhite, resultOfFrontAndWhite,  COLOR_BGR2GRAY);
		whiteFrontObject.push_back(resultOfFrontAndWhite.clone());
	}

	for (int i = 0; i < whiteFrontObject.size() - 1; i++) {
		whiteFrontObjectMoves.push_back(Mat(whiteFrontObject.at(i + 1) - whiteFrontObject.at(i)));
	}

	vector<Point> centerPointOfBallsInAllPic;

	for (int i = 0; i < whiteFrontObjectMoves.size(); i++) {
		//找輪廓
		Mat edge;
		blur(whiteFrontObjectMoves.at(i), edge, Size(3, 3));
		Canny(edge, edge, 50, 150, 3);

		//尋找邊緣
		vector<vector<cv::Point>> contours;
		cv::findContours(edge, contours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);

		//篩選矩形與大小、形狀
		vector<vector<cv::Point> > balls;
		vector<cv::Rect> ballsBox;
		for (size_t i = 0; i < contours.size(); i++) {
			cv::Rect bBox;
			bBox = cv::boundingRect(contours[i]);

			float ratio = (float)bBox.width / (float)bBox.height;

			// 大小與長條狀
			if (ratio >= 1.0f && bBox.area() >= 100 && bBox.area() <= 200) {
				balls.push_back(contours[i]);
				ballsBox.push_back(bBox);
			}
		}

		
		centerPointOfBallsInAllPic.push_back(Point(-1,-1));
		if (ballsBox.size() <= 0) {
			centerPointOfBallsInAllPic.at(i) = centerPointOfBallsInAllPic.at(i - 1);
			centerPointOfBallsInAllPic.at(i).x += 10;
			centerPointOfBallsInAllPic.at(i).y += 6;
			continue;
		}

		//計算中心點
		vector<Point> centers;
		int MaxY = -1;
		Point topLeft;
		for (int j = 0; j < ballsBox.size(); j++) {
			centers.push_back(Point(ballsBox[j].x + ballsBox[j].width / 2, ballsBox[j].y + ballsBox[j].height / 2));

		}
		
		if (i == 0) {
			//第一次選出最左上角的點
			int xTarget = 99999, yTarget = 99999;
			int xPos = -1, yPos = -1;
			int num = 0;
			
			//x,y都dominate
			for (int j = 0; j < centers.size(); j++) {
				if (centers.at(j).x < xTarget) {
					xTarget = centers.at(j).x;
					xPos = j;
				}
				if (centers.at(j).y < yTarget) {
					yTarget = centers.at(j).y;
					yPos = j;
				}
			}

			if (xPos == yPos) {
				centerPointOfBallsInAllPic.at(i) = Point(centers.at(xPos).x, centers.at(xPos).y);
			} else {
				//無法Domiate
				int Target = 99999;
				int position = -1;
				int MaxY = 155;
				for (int j = 0; j < centers.size(); j++) {
					if (centers.at(j).x + MaxY - centers.at(j).y < Target) {
						Target = centers.at(j).x + MaxY - centers.at(j).y;
						position = j;
					}
				}
				centerPointOfBallsInAllPic.at(i) = Point(centers.at(position).x, centers.at(position).y);
			}


			
		} else {
			//選出可能的點
			const int xMoves = 50;
			const int yMoves = 30;
			Point preprePoint;
			if(i>=2)
				preprePoint = centerPointOfBallsInAllPic.at(i - 2);
			Point previousPoint = centerPointOfBallsInAllPic.at(i - 1);
			Point currentPoint;
			for (int j = 0; j < centers.size(); j++) {
				currentPoint = centers.at(j);

				if (previousPoint.x <= currentPoint.x &&
					currentPoint.x <= previousPoint.x + xMoves &&
					previousPoint.y - 10 <= currentPoint.y &&
					currentPoint.y <= previousPoint.y + yMoves) {
					
					centerPointOfBallsInAllPic.at(i) = Point(currentPoint.x, currentPoint.y);
					break;
				}
			}

			if (centerPointOfBallsInAllPic.at(i).x == -1 &&
				centerPointOfBallsInAllPic.at(i).y == -1) {

				centerPointOfBallsInAllPic.at(i).x = 4 + previousPoint.x;
				centerPointOfBallsInAllPic.at(i).y = 4 + previousPoint.y;
			}
		}
		
		
	}
	
	for (int i = 0; i < centerPointOfBallsInAllPic.size(); i++) {
		cout << "No. " << i << " X : " << centerPointOfBallsInAllPic.at(i).x << " Y : " << centerPointOfBallsInAllPic.at(i).y << endl;
	}

	for (int i = 0; i < framesROI.size()-1; i++) {
		circle(framesROI.at(i+1), centerPointOfBallsInAllPic.at(i), 4, Scalar(0, 0, 255), -1);
	}
	
	
	

	int i = 0;
	while(waitKey(10) != 27 || i < frontObject.size()) {
		imshow("Source", framesROI.at(i+2));
		//imshow("front", frontObject.at(i));
		imshow("White", whiteFrontObjectMoves.at(i));
		
		if(waitKey() == 13) i++;
	}

	return 0;
}