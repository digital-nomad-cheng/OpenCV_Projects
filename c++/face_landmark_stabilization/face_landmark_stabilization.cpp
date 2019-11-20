#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>

class PointState {
public:
    PointState(cv::Point2f point): m_point(point), m_kalman(4, 2, 0, CV_64F) {
        init();
    }
    
    void update(cv::Point2f point) {
        cv::Mat measurement(2, 1, CV_64FC1);
        if (point.x < 0 || point.y < 0) {
            predict();
            measurement.at<double>(0) = m_point.x; // update using prediction
            measurement.at<double>(1) = m_point.y;
            m_isPredicted = true;
        } else {
            measurement.at<double>(0) = point.x; // update using measurements
            measurement.at<double>(1) = point.y;
            m_isPredicted = false;
        }

        // correction 
        cv::Mat estimated = m_kalman.correct(measurement);
        m_point.x = static_cast<float>(estimated.at<double>(0));
        m_point.y = static_cast<float>(estimated.at<double>(1));

        predict();
    }

    cv::Point2f getPoint() const {
        return m_point;
    }

    bool isPredicted() const {
        return m_isPredicted;
    }

private:
    cv::Point2f m_point;
    cv::KalmanFilter m_kalman;
    
    double m_deltaTime = 0.9;
    double m_accelNoiseMag = 0.1;
    
    bool m_isPredicted = false;

    void init() {
        m_kalman.transitionMatrix = (cv::Mat_<double>(4,4) << 
                1, 0, m_deltaTime, 0,
                0, 1, 0, m_deltaTime, 
                0, 0, 1, 0,
                0, 0, 0, 1);
        // init position x, y
        m_kalman.statePre.at<double>(0) = m_point.x;
        m_kalman.statePre.at<double>(1) = m_point.y;
        // init velocity x, y
        m_kalman.statePre.at<double>(2) = 1;
        m_kalman.statePre.at<double>(3) = 1;
        
        m_kalman.statePost.at<double>(0) = m_point.x;
        m_kalman.statePost.at<double>(1) = m_point.y;
        
        cv::setIdentity(m_kalman.measurementMatrix);
        
        m_kalman.processNoiseCov = (cv::Mat_<double>(4, 4) << 
                pow(m_deltaTime, 4.0) / 4.0, 0, pow(m_deltaTime, 3.0) / 2.0, 0,
                0, pow(m_deltaTime, 4.0) / 4.0, 0, pow(m_deltaTime, 3.0) / 2.0,
                pow(m_deltaTime, 3.0) / 2.0, 0, pow(m_deltaTime, 2.0), 0,
                0, pow(m_deltaTime, 3.0) / 2.0, 0, pow(m_deltaTime, 2.0));


        m_kalman.processNoiseCov *= m_accelNoiseMag;
        cv::setIdentity(m_kalman.measurementNoiseCov, cv::Scalar::all(0.1));
        cv::setIdentity(m_kalman.errorCovPost, cv::Scalar::all(.1));
    }

    cv::Point2f predict() {
        cv::Mat prediction = m_kalman.predict();
        m_point.x = static_cast<float>(prediction.at<double>(0));
        m_point.y = static_cast<float>(prediction.at<double>(1));
        return m_point;
    }
}; // class PointState

void track(cv::Mat prevFrame, cv::Mat currFrame, const std::vector<cv::Point2f>& currLandmarks, 
        std::vector<PointState>& trackPoints) {
    cv::TermCriteria termcrit(cv::TermCriteria::COUNT | cv::TermCriteria::EPS, 30, 0.01);
    cv::Size winSize(15, 15);
    std::vector<uchar> status(trackPoints.size(), 0);
    std::vector<float> err;
    std::vector<cv::Point2f> newLandmarks;
    std::vector<cv::Point2f> prevLandmarks;
    std::for_each(trackPoints.begin(), trackPoints.end(), 
            [&](const PointState& pts) { prevLandmarks.push_back(pts.getPoint());});
    cv::calcOpticalFlowPyrLK(prevFrame, currFrame, prevLandmarks, newLandmarks, 
            status, err, winSize, 3, termcrit, 0, 0.001);
    for (size_t i = 0; i < status.size(); i++) {
        if (status[i]) {
            trackPoints[i].update((newLandmarks[i]*0.2 + 0.8*currLandmarks[i]));
        } else {
            trackPoints[i].update(currLandmarks[i]);
        }
    }
}

int main(int argc, char** argv) {
    cv::CascadeClassifier faceDetector("resources/haarcascade_frontalface_alt2.xml");
    cv::Ptr<cv::face::Facemark> facemark = cv::face::FacemarkLBF::create();
    facemark->loadModel("resources/lbfmodel.yaml");
    cv::VideoCapture cap("resources/face_tracking_test_video_1.mp4");
    cv::namedWindow("Facial Landmark Localization and Stabilization", cv::WINDOW_NORMAL);
    cv::Mat frame;
    cv::Mat currGray;
    cv::Mat prevGray;
    std::vector<PointState> trackPoints;
    trackPoints.reserve(68);

    while(cap.read(frame)) {
        std::vector<cv::Rect> faces;
        cv::cvtColor(frame, currGray, cv::COLOR_BGR2GRAY);        
        faceDetector.detectMultiScale(currGray, faces, 1.1, 3, cv::CASCADE_FIND_BIGGEST_OBJECT);
        std::vector<std::vector<cv::Point2f> > landmarks;
        bool success = facemark->fit(frame, faces, landmarks);

        if (success) {
            if (prevGray.empty()) {
                trackPoints.clear();
                for (cv::Point2f lp: landmarks[0]) {
                    trackPoints.emplace_back(lp);
                } 
            } else {
                if (trackPoints.empty()) {
                    for (cv::Point2f lp: landmarks[0]) {
                        trackPoints.emplace_back(lp);
                    }
                } else {
                    track(prevGray, currGray, landmarks[0], trackPoints);
                    // trackPoints.clear();
                    // for (cv::Point2f lp: landmarks[0]) {
                    //    trackPoints.emplace_back(lp);
                    // }
            
                }
            }
            for (const PointState& tp: trackPoints) {
                std::cout << trackPoints.size() << std::endl; 
                cv::circle(frame, tp.getPoint(), 3, tp.isPredicted() ? cv::Scalar(0, 0, 255) : cv::Scalar(0, 255, 0), cv::FILLED);
            }

            for (cv::Point2f lp: landmarks[0]) {
                cv::circle(frame, lp, 2, cv::Scalar(255, 0, 255), cv::FILLED);
            }
        }
        cv::imshow("Facial Landmark Localization and Stabilization", frame);
        if (cv::waitKey(1) == 27)
            break;
        prevGray = currGray;
    }

    return 0;
 }
