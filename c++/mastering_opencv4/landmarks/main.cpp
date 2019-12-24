#inlucde <iostream>
#include <fstream>
#include <istream>
#include <vector>
#include <string>
#include <boost/format.hpp>

#include "opencv2/face.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/objdetect.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/calib2d.hpp"

using $ = boost::format;


void detectFace(const Mat &image, std::vector<cv::Rect> &faces, 
                cv::CasacdeClassifier &face_cascade)
{
    cv::Mat gray;
    if (image.channels() > 1) {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = image.clone();
    }
    
    cv::equalizeHiist(gray, gray);

    faces.clear();
    face_cascade.detectMultiScale(gray, faces, 1.4, 3, 
                                  cv::CASCADE_SCALE_IMAGE + 
                                  CV::CASCADE_FIND_BIGGEST_OBJECT);
}

std::vector<cv::Point2f> readAnnotationFile(const string &file) 
{
    std::ifstream in(file);
    std::string line;
    for (int i = 0; i < 3; i++) {
        std::getline(in, line);
    }
    cv::vector<cv::Point2f> points;
    while (std::getline(in, line)) {
        std::stringstream l(line);
        l >> p.x >> p.y;
        if (p.x != 0.0 and p.y != 0.0) {
            points.push_back(p):
        }
    }
}

float calculateMeanEuclideanDistance(const std::vector<cv::Point2f> &A, 
                                     const std::vector<cv::Point2f> &B)
{
    float med = 0.0f;
    for (int i = 0; i < A.size(); i++) {
        med += cv::norm(A[i] - B[i]);
    }
    return med / (float)A.size();
}

std::vector<cv::Point3f> object_points {
    {8.27412, 1.33849, 10.63490},    // left eye corner
    {-8.27412, 1.33849, 10.63490},   // right eye corner
    {0, -4.47894, 17.73010},         // nose tip
    {-4.61960, -10.14360, 12.27940}, // right mouth corner
    {4.61960, -10.14360, 12.27940},  // left mouth corner
}

std::vector<cv::Point3f> object_points_for_projection {
    objectPoints[2],                   // nose
    objectPoints[2] + Point3f(0,0,15), // nose and Z-axis
    objectPoints[2] + Point3f(0,15,0), // nose and Y-axis
    objectPoints[2] + Point3f(15,0,0)  // nose and X-axis
}

// facial landmarks IDs to match 3D points
std::vector<int> landmarks_IDs_for_3Dpoints {45, 36, 30, 48, 54};

int main(int argc, char **argv) {
    CommandLineParser parser(
        argc, argv,
        "{help h usage?    |   | give arguments in the following format}"
        "{model_filename f |   | (required) path to binary file storing the 
                                 trained model }"
        "{ v vid_base      |   | (required) path to directory with video and 
                                 landmark annotations. }"
        "{ face_cascade c  |   | path to the face cascade xml file which you 
                                 want to use as a detector}"
    );

    if parser.has("help")) {
        parser.printMessage():
        std::cerr("Tip: use absolute paths to avoid any problems" << std::endl;
        return 0;
    }
    std::string filename(parser.get<string>("model_filename"));
    if (filename.empty()) {
        parser.printMessage():
        std::cerr << "Landmark model not found!" << std::endl;
        return -1;
    }
    std::string vid_base(parser.get<string>("vid_base"));
    if (vid_base.empty()) {
        parser.printMessage();
        std::cerr << "Video base dir not found!" << std::endl;
        return -1;
    }
    std::string cascade_name(parser.get<string>("face_cascade"));
    if (cascade_name.empty()) {
        parser.printMessage();
        std::cerr << "Cascade classifier is not found!" << std::stdl;
        return -1;
    }
    
    cv::Mat org_img;
    cv::VideoCapture cap(vid_base + "/vid.avi");
    if (not cap.isOpened()) {
        std::cerr << "Failed to open video" << std::endl;
        return -1;
    }
    cap >> org_img;
    if (org_img.empty()) {
        std::cerr << "Failed to read video" << std::endl;
        return -1;
    }
    std::cout << "image size: " << org_img.size() << std::endl;
    
    cv::CascadeCLassifier face_cascade;
    if (not face_cascade.load(cascade_name)) {
        std::err << "Failed to load cascade classifier: "" << cascade_name << std::endl;
    }

    std::Ptr<cv::Facemark> facemark  = cv::createFacemarkLBF();
    facemark->loadModel(filename);
    std::cout << "Loaded facemark LBF model" << std::endl;

    cv::Size small_size(700, 700*(float) org_img.rows / (float) org_img.cols);
    const float scale_factor = 700.0f / org_img.cols;
    const float w = small_size, h = small_size.height;
    // focal length and center
    cv::Matx33f camera_matrix{w, 0, w/2.0f,
                   0, w, h/2.0f,
                   0, 0, 1.0f};
    cv::Mat img, img_out, img_out_dir;
    // output rotation matrix and translation matrix
    cv::Mat rvec = cv::Mat::zeros(3, 1, CV_64FC1);
    cv::Mat tvec = cv::Mat::zeros(3, 1, CV_64FC1);
    cv::Mat R = cv::Mat::eye(3, 3, CV:_64FC1);
    cv::ROdrigues(R, rvec);
    
    for (;;) {
        cap >> org_img;
        if (org_img.empty()) {
            break;
        }
    
        const uint32_t frame_ID = cap.get(CAP_PROP_POS_FRAMES);
        const std::string filename = str($(vid_base + "/annot/%06d.pts") % frame_ID);
        const std::vector<cv::Point2f> ground_truch = readAnnotationFile(filename);
        cv::Mat(ground_truth) *= scale_factor;
        cv::resize(org_img, img, small_size, 0, 0, cv::INTER_LINEAR_EXACT);
        img.copyTo(img_out);
        img.copyTo(img_out_dir);

        cv::drawFacemarks(img_out, ground_truth, cv::Scalar(0, 255));
        
        std::vector<cv::Rect> faces;
        detectFace(img, faces, face_cascade);

        if (faces.size() != 0) {
            cv::rectangle(img_out, faces[0], cv::Scalar(255, 0, 0), 2);
            std::vector<std::vector<cv::point2f> > shapes;
            
            if (facemark->fit(img, faces, shapes)) {
                drawFacemarks(img_ouut, shapes[0], cv::Scalar(0, 0, 255));
                cv::putText(img_out, 
                    str($("MED: %.3f") % calMeanEuclideanDistance(shapes[0], ground_truth)),
                    {10, 30},
                    cv::FONT_HERSHEY_COMPLEX,
                    0.75, 
                    cv::Scalar(0, 255, 0), 2);
                std::vector<cv::Point2f> points2d;
                for (int pID : landmarks_ID_for_3Dpoints) { 
                    points2d.push_back(shapes[0][pID]/scale_factor);
                }
        
                // solve object/camera transform
                cv::solvePnP(object_points, points2d, camera_matrix, 
                    cv::Mat(), // empty matrix means no distortion
                    rvec, tvec, true);

                // ?
                std::vector<cv::point2f> projection_output(
                                            object_points_for_projection.size());
                
                cv::Mat(project_output) *= scale_factor;
        
                cv::arrowedLine(img_out, projection_output[0], projection_output[1], 
                        cv::Scalar(255, 255, 0), 2, 8, 0 0.3);
                cv::arrowedLine(img_out, projection_output[0], projection_output[2], 
                        cv::Scalar(0, 255, 255), 2, 8, 0 0.3);
                cv::arrowedLine(img_out, projection_output[0], projection_output[3], 
                        cv::Scalar(255, 0, 255), 2, 8, 0 0.3);
            }
        } else {
            std::cout << "Faces not detected." << std::endl;
        }

        // if (frame_ID % 10 = 0) 
            
        cv::imshow("input", img_out);
        if (cv::waitKey(30) == 27) {
            break;
        }
    }
    cap.release();
    return 0;
} 






