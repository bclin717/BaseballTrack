#include "stdafx.h"

using namespace std;
using namespace cv;

string ExePath(string files) {
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	string::size_type pos = string(buffer).find_last_of("\\/");
	
	return string(buffer).substr(0, pos) + files;
}

string urlOfVideo[4] = {
	"/video/2017-right-right.mp4",
	"/video/2017-left-right.mp4",
	"/video/2017-left-left.mp4",
	"/video/2017-right-left.mp4"
};
Ptr<BackgroundSubtractor> pKNN;

const int ROI_X = 262;
const int ROI_Y = 260;
const int ROI_X_MOVES = 560;
const int ROI_Y_MOVES = 175;

int main() {

	while (1) {
		int xMoves = 60;
		int yMoves = 40;
		int chosenNum = 0;
		pKNN = createBackgroundSubtractorKNN(100, 600, false);

		system("cls");
		cout << "Choose a video to Demo" << endl;
		cout << "1.Right to Right" << endl;
		cout << "2.Left to Right" << endl;
		cout << "3.Left to Left" << endl;
		cout << "4.Right to Left" << endl;
		cout << "5.Exit" << endl;
		
		
		cin >> chosenNum;
		if (chosenNum == 5) exit(0);


		cout << ExePath(urlOfVideo[0]) << endl;
		VideoCapture video(ExePath(urlOfVideo[chosenNum-1]));
		

		if (!video.isOpened()) {
			cout << "Can not Find the File!!" << endl;
			system("pause");
		}
		Mat videoFrame, temp, mask;
		vector<Mat> videoFrames;
		vector<Mat> framesROI;
		vector<Mat> frontObjectMask;
		vector<Mat> frontObject;
		vector<Mat> whiteFrontObject;
		vector<Mat> whiteFrontObjectMoves;
		vector<Point> centerPointOfBallsInAllPic;

		while (true) {
			video >> videoFrame;
			videoFrames.push_back(videoFrame.clone());
			if (videoFrame.empty()) {
				video.release();
				break;
			}
			//高斯模糊
			GaussianBlur(videoFrame, videoFrame, Size(5, 5), 2, 2);

			//ROI裁切
			framesROI.push_back(videoFrame.clone()(Rect(ROI_X, ROI_Y, ROI_X_MOVES, ROI_Y_MOVES)));
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
			inRange(temp, Scalar(0, 0, 230), Scalar(180, 70, 255), mask);
			Mat resultOfFrontAndWhite;
			frontObject.at(i).copyTo(resultOfFrontAndWhite, mask);
			cvtColor(resultOfFrontAndWhite, resultOfFrontAndWhite, COLOR_BGR2GRAY);
			whiteFrontObject.push_back(resultOfFrontAndWhite.clone());
		}

		for (int i = 0; i < whiteFrontObject.size() - 1; i++) {
			whiteFrontObjectMoves.push_back(Mat(whiteFrontObject.at(i + 1) - whiteFrontObject.at(i)));
		}

		

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
				if (ratio >= 0.9f && bBox.area() >= 20 && bBox.area() <= 200) {
					balls.push_back(contours[i]);
					ballsBox.push_back(bBox);
				}
			}

			

			//選出候選的白色物體
			centerPointOfBallsInAllPic.push_back(Point(-1, -1));
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
				vector<Point> centersPointsYisBiggerThanHalf;

				for (int j = 0; j < centers.size(); j++) {
					if (centers.at(j).y <= ROI_Y_MOVES * 2 / 3) {
						centersPointsYisBiggerThanHalf.push_back(Point(centers.at(j)));
					}
				}

				//第一次選出最左上角的點
				int xTarget = 99999, yTarget = 99999;
				int xPos = -1, yPos = -1;
				int num = 0;

				//x,y都dominate
				for (int j = 0; j < centersPointsYisBiggerThanHalf.size(); j++) {
					if (centersPointsYisBiggerThanHalf.at(j).x < xTarget) {
						xTarget = centersPointsYisBiggerThanHalf.at(j).x;
						xPos = j;
					}
					if (centersPointsYisBiggerThanHalf.at(j).y < yTarget) {
						yTarget = centersPointsYisBiggerThanHalf.at(j).y;
						yPos = j;
					}
				}

				if (xPos == yPos) {
					centerPointOfBallsInAllPic.at(i) = Point(centersPointsYisBiggerThanHalf.at(xPos).x, centersPointsYisBiggerThanHalf.at(xPos).y);
				}
				else {
					//無法Domiate
					int Target = 99999;
					int position = -1;
					int MaxY = ROI_Y_MOVES;
					for (int j = 0; j < centersPointsYisBiggerThanHalf.size(); j++) {
						if (centersPointsYisBiggerThanHalf.at(j).x + MaxY - centersPointsYisBiggerThanHalf.at(j).y < Target) {
							Target = centersPointsYisBiggerThanHalf.at(j).x + MaxY - centersPointsYisBiggerThanHalf.at(j).y;
							position = j;
						}
					}
					centerPointOfBallsInAllPic.at(i) = Point(centersPointsYisBiggerThanHalf.at(position).x, centersPointsYisBiggerThanHalf.at(position).y);
				}
			}
			else {
				//選出可能的點


				Point preprePoint;
				if (i >= 2)
					preprePoint = centerPointOfBallsInAllPic.at(i - 2);
				Point previousPoint = centerPointOfBallsInAllPic.at(i - 1);
				Point currentPoint;

				float xDiff = previousPoint.x - preprePoint.x;
				float yDiff = previousPoint.y - preprePoint.y;

				int yUp = yMoves, yDown = yMoves;


				for (int j = 0; j < centers.size(); j++) {
					currentPoint = centers.at(j);

					if (i >= 2) {
						if (yDiff > 0) {
							yUp = 20;
						}
						else {
							yDown = 20;
						}
					}

					if (previousPoint.x <= currentPoint.x &&
						currentPoint.x <= previousPoint.x + xMoves &&
						previousPoint.y - yUp <= currentPoint.y &&
						currentPoint.y <= previousPoint.y + yDown) {

						centerPointOfBallsInAllPic.at(i) = Point(currentPoint.x, currentPoint.y);
						break;
					}
				}

				float minXmoves = 1, minYmoves = 1;

				//找不到球
				if (centerPointOfBallsInAllPic.at(i).x == -1 &&
					centerPointOfBallsInAllPic.at(i).y == -1 &&
					i >= 2) {


					if (xDiff >= yDiff && yDiff != 0) {
						minYmoves = 1;
						minXmoves = (float)xDiff / (float)yDiff;
						if (minXmoves < 0) minXmoves *= -1;
					}
					else if (xDiff != 0) {
						minXmoves = 1;
						minYmoves = (float)yDiff / (float)xDiff;
						if (minYmoves < 0) minYmoves *= -1;
					}
					Point cur(previousPoint.x + minXmoves * 2, previousPoint.y + minYmoves * 2.5);
					xMoves *= 1.2;
					yMoves *= 1.4;

					centerPointOfBallsInAllPic.at(i) = cur;
				}

				vector<Point>(centers).swap(centers);

			}


		}

		/*for (int i = 0; i < centerPointOfBallsInAllPic.size(); i++) {
			cout << "No. " << i << " X : " << centerPointOfBallsInAllPic.at(i).x << " Y : " << centerPointOfBallsInAllPic.at(i).y << endl;
		}*/

		//for (int i = 0; i < framesROI.size() - 1; i++) {
		//	circle(framesROI.at(i + 1), centerPointOfBallsInAllPic.at(i), 4, Scalar(0, 0, 255), -1);
		//}

		for (int pic = 0; pic < centerPointOfBallsInAllPic.size(); pic++) {
			for (int i = 1; i < pic; i++) {
				line(videoFrames.at(pic), Point(centerPointOfBallsInAllPic.at(i - 1).x + ROI_X, centerPointOfBallsInAllPic.at(i - 1).y + ROI_Y), Point(centerPointOfBallsInAllPic.at(i).x + ROI_X, centerPointOfBallsInAllPic.at(i).y + ROI_Y), Scalar(0, 0, 255), 2, 8, 0);
			}
		}

		for (int i = 0; i < centerPointOfBallsInAllPic.size(); i++) {
			circle(videoFrames.at(i + 1), Point(centerPointOfBallsInAllPic.at(i).x + ROI_X, centerPointOfBallsInAllPic.at(i).y + ROI_Y), 4, Scalar(0, 0, 255), -1);
		}

		int i = 0;
		while (i + 1 < videoFrames.size() - 2) {
			imshow("Demo", videoFrames.at(i + 1));
			if (waitKey(10) == 13 ) i++;
		}
		destroyWindow("Demo");
	}
	
	return 0;
}