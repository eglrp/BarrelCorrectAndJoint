#include "ImageJointor.h"

using namespace cv;

ImageJointor::ImageJointor()
{

}


ImageJointor::~ImageJointor()
{

}

cv::Mat ImageJointor::Joint(const std::string &src_dir)
{
	std::vector<std::string> files = YXPFileIO::GetDirectoryFiles(src_dir);
	cv::Mat origin = imread(files[0]);

	for (int i=1;i!=files.size();++i)
	{
		origin = Joint(origin, imread(files[i]));
		std::cout << "��" << i + 1 << "��ͼ��ƴ�����" << std::endl;
		imshow("ƴ��ͼ��",origin);
		cv::waitKey(0);
	}

	return origin;
}


//����ԭʼͼ���λ�ھ�������任����Ŀ��ͼ���϶�Ӧλ��
Point2f ImageJointor::getTransformPoint(const Point2f originalPoint, const Mat &transformMaxtri)
{
	Mat originelP, targetP;
	originelP = (Mat_<double>(3, 1) << originalPoint.x, originalPoint.y, 1.0);
	targetP = transformMaxtri*originelP;
	float x = targetP.at<double>(0, 0) / targetP.at<double>(2, 0);
	float y = targetP.at<double>(1, 0) / targetP.at<double>(2, 0);
	return Point2f(x, y);
}

cv::Mat ImageJointor::Joint(const cv::Mat &src1, const cv::Mat &src2)
{
	//Mat src1 = imread("11.jpg");
	//Mat src2 = imread("22.jpg");
	//imshow("ƴ��ͼ��1", src1);
	//imshow("ƴ��ͼ��2", src2);

	//�Ҷ�ͼת��
	Mat src1_gray, src2_gray;
	cvtColor(src1, src1_gray, CV_RGB2GRAY);
	cvtColor(src2, src2_gray, CV_RGB2GRAY);

	//��ȡ������  
	SiftFeatureDetector siftDetector(800);  // ����������ֵ
	vector<KeyPoint> keyPoint1, keyPoint2;
	siftDetector.detect(src1_gray, keyPoint1);
	siftDetector.detect(src2_gray, keyPoint2);

	//������������Ϊ�±ߵ�������ƥ����׼��  
	SiftDescriptorExtractor siftDescriptor;
	Mat imageDesc1, imageDesc2;
	siftDescriptor.compute(src1_gray, keyPoint1, imageDesc1);
	siftDescriptor.compute(src2_gray, keyPoint2, imageDesc2);

	//���ƥ�������㣬����ȡ�������  	
	FlannBasedMatcher matcher;
	vector<DMatch> matchePoints;
	matcher.match(imageDesc1, imageDesc2, matchePoints, Mat());
	sort(matchePoints.begin(), matchePoints.end()); //����������	
													//��ȡ����ǰN��������ƥ��������
	vector<Point2f> imagePoints1, imagePoints2;
	for (int i = 0; i < 10; i++)
	{
		imagePoints1.push_back(keyPoint1[matchePoints[i].queryIdx].pt);
		imagePoints2.push_back(keyPoint2[matchePoints[i].trainIdx].pt);
	}

	//��ȡͼ��1��ͼ��2��ͶӰӳ����󣬳ߴ�Ϊ3*3
	Mat homo = findHomography(imagePoints1, imagePoints2, CV_RANSAC);
	Mat adjustMat = (Mat_<double>(3, 3) << 1.0, 0, src1.cols, 0, 1.0, 0, 0, 0, 1.0);
	Mat adjustHomo = adjustMat*homo;

	//��ȡ��ǿ��Ե���ԭʼͼ��;���任��ͼ���ϵĶ�Ӧλ�ã�����ͼ��ƴ�ӵ�Ķ�λ
	Point2f originalLinkPoint, targetLinkPoint, basedImagePoint;
	originalLinkPoint = keyPoint1[matchePoints[0].queryIdx].pt;
	targetLinkPoint = getTransformPoint(originalLinkPoint, adjustHomo);
	basedImagePoint = keyPoint2[matchePoints[0].trainIdx].pt;

	//ͼ����׼
	Mat imageTransform1;
	warpPerspective(src1, imageTransform1, adjustMat*homo, Size(src2.cols + src1.cols, src2.rows));

	//����ǿƥ��������ص���������ۼӣ����ν��ȶ����ɣ�����ͻ��
	Mat image1Overlap, image2Overlap; //ͼ1��ͼ2���ص�����	
	image1Overlap = imageTransform1(Rect(Point(targetLinkPoint.x - basedImagePoint.x, 0), Point(targetLinkPoint.x, src2.rows)));
	image2Overlap = src2(Rect(0, 0, image1Overlap.cols, image1Overlap.rows));
	Mat image1ROICopy = image1Overlap.clone();  //����һ��ͼ1���ص�����
	for (int i = 0; i < image1Overlap.rows; i++)
	{
		for (int j = 0; j < image1Overlap.cols; j++)
		{
			double weight;
			weight = (double)j / image1Overlap.cols;  //�����ı���ı�ĵ���ϵ��
			image1Overlap.at<Vec3b>(i, j)[0] = (1 - weight)*image1ROICopy.at<Vec3b>(i, j)[0] + weight*image2Overlap.at<Vec3b>(i, j)[0];
			image1Overlap.at<Vec3b>(i, j)[1] = (1 - weight)*image1ROICopy.at<Vec3b>(i, j)[1] + weight*image2Overlap.at<Vec3b>(i, j)[1];
			image1Overlap.at<Vec3b>(i, j)[2] = (1 - weight)*image1ROICopy.at<Vec3b>(i, j)[2] + weight*image2Overlap.at<Vec3b>(i, j)[2];
		}
	}
	Mat ROIMat = src2(Rect(Point(image1Overlap.cols, 0), Point(src2.cols, src2.rows)));	 //ͼ2�в��غϵĲ���
	ROIMat.copyTo(imageTransform1(Rect(/*targetLinkPoint.x*/imageTransform1.cols - ROIMat.cols, 0, ROIMat.cols,/*image02.rows*/ROIMat.rows))); //���غϵĲ���ֱ���ν���ȥ
	//namedWindow("ƴ�ӽ��");
	//imshow("ƴ�ӽ��", imageTransform1);

	return imageTransform1;
}


cv::Mat ImageJointor::Joint(const std::string &src1_name,const std::string &src2_name)
{
	return Joint(imread(src1_name), imread(src2_name));
}