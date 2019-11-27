#include <iostream>
#include <stdlib.h>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <opencv2/opencv.hpp>

#define TRACKER_CONF_PATH ("./c++/faces_tracker/config/tracker_conf.ini")
#define MAIN_WINDOW_NAME ("tracker window")
#define FACE_WINDOW_NAME ("face-window-")
#define EXIT_KEY_CODE (27)
#define MAX_CORNERS_TO_DETECT_INSIDE_ROI 40
#define MAX_MUTEXES_BUFFER_SIZE 100

// default configuration values
#define DEF_VIDEO_CAPTURE_RESOURCE (0)
#define DEF_HAARCASCADE_FRONTAL_FACE_PATH ("haarcascade_frontalface_alt2.xml")
#define DEF_HAARCASCADE_EYE_PATH ("haarcascade_eye.xml")
#define DEF_HAARCASCADE_NOSE_PATH ("nose.xml")
#define DEF_HAARCASCADE_MOUTH_PATH ("mouth.xml")

// configurations fields
#define CONF_FIELD_IS_CAMERA ("IS_CAMERA")
#define CONF_FIELD_RESOURCE ("RESURCE")
#define CONF_FIELD_VIDEO_PATH ("VIDEO_PATH")
#define CONF_FIELD_FPS ("FPS")
#define CONF_FIELD_IS_RECORD ("IS_RECORD")
#define CONF_FIELD_OUTPUT_VIDEO_PATH ("OUTPUT_VIDEO_PATH")
#define CONF_FIELD_HAAR_FACE_FEATURES_PATH ("HAAR_FACE_FEATURES_PATH")
#define CONF_FIELD_HAAR_EYE_FEATURES_PATH ("HAAR_EYE_FEATURES_PATH")
#define CONF_FIELD_HAAR_NOSE_FEATURES_PATH ("HAAR_NOSE_FEATURES_PATH")
#define CONF_FIELD_HAAR_MOUTH_FEATURES_PATH ("HAAR_MOUTH_FEATURES_PATH")

typedef struct
{
    bool is_webcam;
    int resource;
    std::string video_path;
    int fps;
    bool is_record;
    std::string output_video_name;
    std::string face_Haar_features_path;
    std::string eye_Haar_features_path;
    std::string nose_Haar_features_path;
    std::string mouth_Haar_features_path;

} TrackerConfigurations;

/* 
 * Shared parameters between main thread(tracker) and frames sampler thread
 * 1. frames_queue: the que that contains the frames entered by the sampler
 *                  thread(the producer) and taken by the main thread(the consumer)
 * 2. sampler_lock: mutex for the frames queue
 * 3. is_program_running: if the sampler thread still running
 * 4. fps: frames per second of the current stream
 * 5. tracker_configs: configurations for the tracker(default or from file)
 */

std::queue<cv::Mat> frames_queue;
std::mutex sampler_lock;
bool is_program_running = true;
TrackerConfigurations tracker_confs;

typedef struct
{
    cv::Rect face;
    std::vector<cv::Rect> eyes;
    cv::Rect nose;
    cv::Rect mouth;
} FacialROIs;

typedef struct
{
    bool active;
    bool created;
    std::string name;
} FaceWindowParams;

typedef struct
{
    std::vector<std::vector<cv::Point2f> >* curr_ROIs;
    std::vector<FaceWindowParams>* face_windows_params;
} MouseCallbackData;

typedef struct
{
    cv::Mat inv;
    cv::Mat img;
} FaceWindowThreadParams;

cv::CascadeClassifier face_classifier;
cv::CascadeClassifier eye_classifier;
cv::CascadeClassifier nose_classifier;
cv::CascadeClassifier mouth_classifier;

void loadConfigurations();
cv::Rect getTranslatedROI(const cv::Rect& src_ROI, const cv::Rect& container_ROI);
bool detectFacialROIs(const cv::Mat& gray_img, std::vector<FacialROIs>& facial_ROIs);
bool findFeaturesInsideFacialROIs(const cv::Mat& gray_img, 
        const std::vector<FacialROIs>& facial_ROIs,
        std::vector<std::vector<cv::Point2f> >& features_groups);
bool buildLKPyr(const cv::Mat& gray_img, std::vector<cv::Mat>& pyr);
bool calcLKOpticalFlowForAllFeaturesGroups(const std::vector<cv::Mat>& prev_pyr,
        const std::vector<cv::Mat>& curr_pyr,
        const std::vector<std::vector<cv::Point2f> >& prev_features_groups,
        std::vector<std::vector<cv::Point2f> >&curr_features_groups);

void drawFacialROIs(cv::Mat& img, const std::vector<FacialROIs>& facial_ROIs);
void drawFacialFeaturesGroups(cv::Mat& img,
        const std::vector<std::vector<cv::Point2f> >& features_groups);
void drawPoly(cv::Mat& img, const std::vector<cv::Point2f>& pts);
void drawROIs(cv::Mat& img, const std::vector<std::vector<cv::Point2f> >& ROIs);
void convertFacialROIsToPolys(const std::vector<FacialROIs>& facial_ROIs,
        std::vector<std::vector<cv::Point2f> >& ROIs);

bool getRigidTransformationMatrices(
        const std::vector<std::vector<cv::Point2f> >& curr_features_groups,
        const std::vector<std::vector<cv::Point2f> >& prev_features_groups,
        const std::vector<std::vector<cv::Point2f> >& init_ROIs,
        const std::vector<std::vector<cv::Point2f> >& curr_ROIs,
        std::vector<cv::Mat>& trans_matrices,
        std::vector<cv::Mat>& trans_matrices_inv);
void performRigidTransformOnROIs(
        const std::vector<cv::Mat>& trans_matrices,
        std::vector<std::vector<cv::Point2f> >& ROIs,
        const cv::Size& img_size);
void performRigidTransformOnImg(const cv::Mat& matrix, cv::Mat& img);
bool acquireFrameFromBuffer(cv::Mat& output_frame);
void framesSamplerThread();
bool is_point_in_ROI(const cv::Point2f& pt, const std::vector<cv::Point2f>& ROI);
cv::Rect getReducedROI(const cv::Rect& src_ROI, double percents);
cv::Rect getEnlargeROI(const cv::Rect& src_ROI, double percents);

void mainWindowMouseCallback(int event, int x, int y, int, void* data);

std::mutex mtxs[MAX_MUTEXES_BUFFER_SIZE];
std::vector<std::queue<FaceWindowThreadParams>> face_window_threads_buffers;
std::vector<FacialROIs> curr_facial_ROIs_vector;
std::vector<FaceWindowParams> face_windows_params;

void faceThread(int queue_index);

int getMainLoopDelayByVideoFPS();

int main(int argc, char** argv)
{
    loadConfigurations();
    int delay = getMainLoopDelayByVideoFPS();
    cv::VideoCapture cap;
    std::thread streaming_job;
    
    if (tracker_confs.is_webcam) {
        streaming_job = std::thread(framesSamplerThread);
        std::cout << "start video streaming job" << std::endl;
    } 
    else {
        cap.open(tracker_confs.video_path);
        if (!cap.isOpened()) {
            std::cout << "failed to open video capture from file" << tracker_confs.video_path << std::endl;
            return EXIT_FAILURE;
        }
    }

    bool initial_processing_needed = true;
    cv::namedWindow(MAIN_WINDOW_NAME, cv::WINDOW_NORMAL);
    std::cout << "starting tracker main loop" << std::endl;
    
    cv::Mat curr_bgr_frame;
    cv::Mat curr_bgr_frame_copy;
    cv::Mat curr_gray_frame;
    cv::Mat prev_gray_frame;

    std::vector<cv::Mat> curr_pyr;
    std::vector<cv::Mat> prev_pyr;
    std::vector<std::vector<cv::Point2f> > init_ROIs;
    std::vector<std::vector<cv::Point2f> > curr_ROIs;
    std::vector<std::vector<cv::Point2f> > curr_features_groups;
    std::vector<std::vector<cv::Point2f> > prev_features_groups;
    std::vector<cv::Mat> trans_matrices;
    std::vector<cv::Mat> trans_matrices_inv;
    std::vector<std::pair<std::thread, bool> > face_threads;

    bool is_window_resized = false;
    cv::VideoWriter output_video;
    std::string output_video_path = tracker_confs.output_video_name;
    bool is_video_writer_initialized = false;

    while (is_program_running) {
        bool good_sampling = true;
        if (tracker_confs.is_webcam) {
            good_sampling = acquireFrameFromBuffer(curr_bgr_frame);
        }
        else {
            cap >> curr_bgr_frame;
            if (curr_bgr_frame.empty()) {
                good_sampling = false;
                is_program_running = false;
                break;
            }   
        }
        if (!is_video_writer_initialized) {
            if (tracker_confs.is_record) {
                output_video.open(output_video_path, CV_FOURCC('D', 'I', 'V', 'X'),
                        tracker_confs.fps, curr_bgr_frame.size(), true);
                if (!output_video.isOpened()) {
                    std::cout << "cannot open output  video writer" << std::endl;
                    is_program_running = false;
                    break;
                }
                is_video_writer_initialized = true;
            }
        }

        if (!is_window_resized) {
            cv::resizeWindow(MAIN_WINDOW_NAME, curr_bgr_frame.size().width, 
                    curr_bgr_frame.size().height);
            is_window_resized = true;
        }

        curr_bgr_frame_copy = curr_bgr_frame.clone();
        cv::cvtColor(curr_bgr_frame_copy, curr_gray_frame, cv::COLOR_RGB2GRAY);
        // histogram equalization for areas with inconsistent illumination
        cv::equalizeHist(curr_gray_frame, curr_gray_frame); 
        // --------------------------------------
        //           initial processing
        // -------------------------------------- 
        if (initial_processing_needed) {
            bool facial_ROIs_detection_succeeded = detectFacialROIs(curr_gray_frame, curr_facial_ROIs_vector);
            if (facial_ROIs_detection_succeeded) {
                bool facial_features_detection_succeeded = findFeaturesInsideFacialROIs(curr_gray_frame, 
                        curr_facial_ROIs_vector, curr_features_groups);
                if (facial_features_detection_succeeded) {
                    initial_processing_needed = false;
                    convertFacialROIsToPolys(curr_facial_ROIs_vector, curr_ROIs);
                    init_ROIs = curr_ROIs;
                
                    trans_matrices.clear();
                    trans_matrices_inv.clear();
                    for (int i = 0; i < curr_facial_ROIs_vector.size(); i++) {
                        trans_matrices.push_back(cv::Mat());
                        trans_matrices_inv.push_back(cv::Mat());
                    }
                
                    for (int i = 0; i < curr_facial_ROIs_vector.size(); i++) {
                        FaceWindowParams window_params;
                        window_params.active = window_params.created = false;
                        std::string window_name = FACE_WINDOW_NAME;
                        std::string buff = std::to_string(i+1);
                        window_name += buff;
                        window_params.name = window_name;
                        face_windows_params.push_back(window_params);
                        face_window_threads_buffers.push_back(std::queue<FaceWindowThreadParams>());
                    }

                    MouseCallbackData mouse_callback_data;
                    mouse_callback_data.face_windows_params = &face_windows_params;
                    mouse_callback_data.curr_ROIs = &curr_ROIs;
                    cv::setMouseCallback(MAIN_WINDOW_NAME, mainWindowMouseCallback, &mouse_callback_data);
                
                    // initialize stabilizers threads
                    for (int i = 0; i < curr_facial_ROIs_vector.size(); i++) {
                        face_threads.push_back(std::pair<std::thread, bool>(std::thread(faceThread, i), true));
                    }
                }
            }
        }
        // -----------------------------------------------
        //                rest frames
        // -----------------------------------------------
        else {
            // cv::imshow("prev_frame", prev_gray_frame);
            // cv::imshow("curr_frame", curr_gray_frame);
            // cv::waitKey(0);
            buildLKPyr(prev_gray_frame, prev_pyr);
            buildLKPyr(curr_gray_frame, curr_pyr);
            calcLKOpticalFlowForAllFeaturesGroups(prev_pyr, curr_pyr, prev_features_groups, curr_features_groups);
            if (getRigidTransformationMatrices(curr_features_groups, prev_features_groups, init_ROIs, curr_ROIs, 
                        trans_matrices, trans_matrices_inv)) {
                performRigidTransformOnROIs(trans_matrices, curr_ROIs, curr_bgr_frame_copy.size());
            }

            for (int i = 0; i < face_windows_params.size(); i++) {
                if (face_windows_params[i].active  && face_windows_params[i].created) {
                    FaceWindowThreadParams wtp;
                    wtp.img = curr_bgr_frame_copy.clone();
                    wtp.inv = trans_matrices_inv[i];
                    mtxs[i].lock();
                    face_window_threads_buffers[i].push(wtp);
                    mtxs[i].unlock();
                }
            }
        }

        drawFacialFeaturesGroups(curr_bgr_frame_copy, curr_features_groups);
        drawROIs(curr_bgr_frame_copy, curr_ROIs);
        cv::imshow(MAIN_WINDOW_NAME, curr_bgr_frame_copy);
        cv::swap(prev_gray_frame, curr_gray_frame);
        prev_features_groups.clear();
        for (std::vector<cv::Point2f>& cg: curr_features_groups) {
            std::vector<cv::Point2f> pg;
            std::swap(pg, cg);
            prev_features_groups.push_back(pg);
        }

        prev_pyr.clear();
        for (cv::Mat& cm: curr_pyr) {
            cv::Mat pm;
            cv::swap(pm, cm);
            prev_pyr.push_back(pm);
        }

        if (is_video_writer_initialized) {
            output_video << curr_bgr_frame_copy;
        }

        int key = cv::waitKey(delay);
        if (key == EXIT_KEY_CODE) {
            std::cout << "user stopped main loop" << std::endl;
            is_program_running = false;
            break; 
        }
    }

    std::cout << "main loop ended" << std::endl;
    cv::destroyAllWindows();
    std::cout << "resources released" << std::endl;
    std::cout << "program ended successfully" << std::endl;
    
    if (tracker_confs.is_webcam) {
        streaming_job.join();
    }

    for (int i = 0; i < face_threads.size(); i++) {
        if (face_threads[i].second) {
            face_threads[i].first.join();
        }

    }

    return 0;
}

void loadConfigurations() {
    std::ifstream ifs;
    ifs.open(TRACKER_CONF_PATH);

    if (!ifs.good()) {
        std::cout << "Program failed to open configuration file: " << TRACKER_CONF_PATH << std::endl;
        std::cout << "Loading default configuration" << std::endl;
        tracker_confs.is_webcam = true;
        tracker_confs.resource = DEF_VIDEO_CAPTURE_RESOURCE; 
        tracker_confs.video_path = "";
        tracker_confs.fps = 30;
        tracker_confs.is_record = false;
        tracker_confs.output_video_name = "";
        tracker_confs.face_Haar_features_path = DEF_HAARCASCADE_FRONTAL_FACE_PATH; 
        tracker_confs.eye_Haar_features_path = DEF_HAARCASCADE_EYE_PATH;
        tracker_confs.nose_Haar_features_path = DEF_HAARCASCADE_NOSE_PATH;
        tracker_confs.mouth_Haar_features_path = DEF_HAARCASCADE_MOUTH_PATH;
    }

    std::string line;
    char delimeter = '=';
    // read lines
    while (std::getline(ifs, line)) {
        std::string field = line.substr(0, line.find(delimeter));
        std::string field_value = line.substr(line.find(delimeter) + 1);
        std::cout << line << std::endl;

        if (field == CONF_FIELD_IS_CAMERA) {
            std::istringstream iss(field_value);
            iss >> tracker_confs.is_webcam;
        }
        else if (field == CONF_FIELD_RESOURCE) {
            std::istringstream iss(field_value);
            iss >> tracker_confs.resource;
        }
        else if (field == CONF_FIELD_VIDEO_PATH) {
            tracker_confs.video_path = field_value;
        }
        else if (field == CONF_FIELD_FPS) {
            std::istringstream iss(field_value);
            iss >> tracker_confs.fps;
        }
        else if (field == CONF_FIELD_HAAR_FACE_FEATURES_PATH) {
            tracker_confs.face_Haar_features_path = field_value;
        }
        else if (field == CONF_FIELD_HAAR_EYE_FEATURES_PATH) {
            tracker_confs.eye_Haar_features_path = field_value;
        }
        else if (field == CONF_FIELD_HAAR_NOSE_FEATURES_PATH) {
            tracker_confs.nose_Haar_features_path = field_value;
        }
        else if (field == CONF_FIELD_HAAR_MOUTH_FEATURES_PATH) {
            tracker_confs.mouth_Haar_features_path = field_value;
        }
        else if (field == CONF_FIELD_IS_RECORD) {
            std::istringstream iss(field_value);
            iss >> tracker_confs.is_record;
        }
        else if (field == CONF_FIELD_OUTPUT_VIDEO_PATH) {
            tracker_confs.output_video_name = field_value;
        }
    }
}


bool isPointInsideROI(const cv::Point2f& pt, const std::vector<cv::Point2f>& ROI) 
{
    return (pt.x > cv::max(ROI[0].x, ROI[3].x) && pt.x < cv::min(ROI[1].x, ROI[2].x)
            &&
            pt.y > cv::max(ROI[0].y, ROI[1].y) && pt.y < cv::min(ROI[2].y, ROI[3].y));
}

void mainWindowMouseCallback(int event, int x, int y, int, void* data)
{
    if (event == cv::EVENT_LBUTTONDBLCLK) {
        MouseCallbackData* mouse_callback_data = (MouseCallbackData*)data;
        const std::vector<std::vector<cv::Point2f> >& ROIs = *(mouse_callback_data->curr_ROIs);
        std::vector<FaceWindowParams>& windows_params = *(mouse_callback_data->face_windows_params);
        cv::Point2i test_point(x, y);
        for (int i = 0; i < windows_params.size(); i++) {
            if (!windows_params[i].active && !windows_params[i].created) {
                if (isPointInsideROI(test_point, ROIs[i])) {
                    std::cout << "user choose to activate tracker NO." << i << " ROI " << ROIs[i] << std::endl;
                    windows_params[i].active = true;
                    cv::namedWindow(windows_params[i].name, cv::WINDOW_AUTOSIZE);
                    windows_params[i].created = true;
                }
            }
        }
    }
}

bool getRigidTransformationMatrices(const std::vector<std::vector<cv::Point2f> >& curr_features_groups,
        const std::vector<std::vector<cv::Point2f> >& prev_features_groups,
        const std::vector<std::vector<cv::Point2f> >& init_ROIs, 
        const std::vector<std::vector<cv::Point2f> >& curr_ROIs, 
        std::vector<cv::Mat>& trans_matrices,
        std::vector<cv::Mat>& trans_matrices_inv) {
    if (curr_features_groups.size() != prev_features_groups.size() || init_ROIs.size() != curr_ROIs.size()) {
        std::cerr <<"Error - different number between current features groups and previous one" << std::endl;
        return false;
    }

    const bool full_affine = false;
    for (int i = 0; i < curr_features_groups.size(); i++) {
        const std::vector<cv::Point2f>& prev_group = prev_features_groups[i];
        const std::vector<cv::Point2f>& curr_group = curr_features_groups[i];
        const std::vector<cv::Point2f>& init_ROI = init_ROIs[i];
        const std::vector<cv::Point2f>& curr_ROI = curr_ROIs[i];

        if (prev_group.size() != curr_group.size() || prev_group.empty() || curr_group.empty()) {
            std::cout <<"Warning - something wrong with the size of the " << i << " features groups" << std::endl;
            cv::Mat empty;
            trans_matrices = empty;
        }
        else {
            cv::Mat R = cv::estimateRigidTransform(prev_group, curr_group, full_affine);
            trans_matrices[i] = R;
        }

        if (init_ROI.size() != curr_ROI.size() || init_ROI.empty() || curr_ROI.empty()) {
            std::cout << "Warning - something wrong with the size of the " << i << " ROI groups" << std::endl;
            cv::Mat empty;
            trans_matrices[i] = empty;
        }
        else {
            cv::Mat R = cv::estimateRigidTransform(curr_ROI, init_ROI, full_affine);
            trans_matrices_inv[i] = R;
        }
    }

    return true;
}

void performRigidTransformOnImg(const cv::Mat& M, cv::Mat& img) 
{
    const int flag = 1;
    const int border_mode = 1;
    const cv::Scalar border_value = cv::Scalar();
    if (!M.empty() && !img.empty()) {
        cv::warpAffine(img, img, M, img.size(), flag, border_mode, border_value);
    }
}

void performRigidTransformOnROIs(const std::vector<cv::Mat>& trans_matrices, std::vector<std::vector<cv::Point2f>>& ROIs, const cv::Size& img_size) 
{
    if (trans_matrices.size() != ROIs.size()) {
        std::cout << "Number of transormation matrices is not suitable for number of ROIs" << std::endl;
    }
    for (int i = 0; i < ROIs.size(); i++) {
        const cv::Mat& M = trans_matrices[i];
        std::vector<cv::Point2f>& ROI = ROIs[i];
        if (!M.empty()) {
            bool outof_img = false;
            std::vector<cv::Point2f> tmp_ROI;
            cv::transform(ROI, tmp_ROI, M);
            for (cv::Point2f pt: tmp_ROI) {
                if (pt.x < 0 || pt.y < 0 || pt.x > img_size.width || pt.y > img_size.height) {
                    outof_img = true;
                }
            }

            if (!outof_img) {
                ROI = tmp_ROI;
            }
        }
    }
}

void convertRectToPts(const cv::Rect& rect, std::vector<cv::Point2f>& pts) 
{
    float x = (float)rect.x;
    float y = (float)rect.y;
    float w = (float)rect.width;
    float h = (float)rect.height;

    pts.clear();
    pts.push_back(cv::Point2f(x, y));
    pts.push_back(cv::Point2f(x+w, y));
    pts.push_back(cv::Point2f(x+w, y+h));
    pts.push_back(cv::Point2f(x, y+h));
}

void drawPoly(cv::Mat& img, const std::vector<cv::Point2f>& pts)
{
    const bool is_closed = true;
    const cv::Scalar color(255, 255, 255);
    const int thickness = 1;
    const int line_type = cv::LINE_8;
    const int shift = 0;

    std::vector<cv::Point2i> pti;
    for (cv::Point2f ptf: pts) {
        pti.push_back(cv::Point2i((int)ptf.x, (int)ptf.y));
    }

    cv::polylines(img, pti, is_closed, color, thickness, line_type, shift);
}

void drawROIs(cv::Mat& img, const std::vector<std::vector<cv::Point2f> >& ROIs)
{
    for (const std::vector<cv::Point2f>& ROI: ROIs) {
        drawPoly(img, ROI);
    }
}

bool calcLKOpticalFlowForAllFeaturesGroups(const std::vector<cv::Mat>& prev_pyr, const std::vector<cv::Mat>& curr_pyr,
        const std::vector<std::vector<cv::Point2f> >& prev_features_groups, 
        std::vector<std::vector<cv::Point2f> >& curr_features_groups) 
{
    if (prev_features_groups.empty()) {
        std::cout << "No features groups to move from in optical flow" << std::endl;
        return false;
    }

    curr_features_groups.clear();
    std::vector<cv::Point2f> full_prev_groups;
    for (const std::vector<cv::Point2f>& prev_group: prev_features_groups) {
        full_prev_groups.insert(full_prev_groups.end(), prev_group.begin(), prev_group.end());
    }

    const cv::Size optical_flow_win_size = cv::Size(21, 21);
    const int max_level = 3;
    std::vector<cv::Point2f> curr_corners, full_prev_groups_inv;
    std::vector<float> first_err, second_err;
    std::vector<uchar> first_status, second_status;
    cv::TermCriteria opt_flow_criteria = cv::TermCriteria(cv::TermCriteria::MAX_ITER | cv::TermCriteria::EPS, 30, 0.01);
    const int flags = 0;
    const double min_eig_threshold = 1e-4;

    cv::calcOpticalFlowPyrLK(prev_pyr, curr_pyr, full_prev_groups, curr_corners, first_status, first_err, optical_flow_win_size,
            max_level, opt_flow_criteria, flags, min_eig_threshold);
    cv::calcOpticalFlowPyrLK(curr_pyr, prev_pyr, curr_corners, full_prev_groups_inv, second_status, second_err, 
            optical_flow_win_size, max_level, opt_flow_criteria, flags, min_eig_threshold);
    std::vector<cv::Point2f> final_corners;
    for (int i = 0; i < curr_corners.size(); i++) {
        cv::Point2f diff = full_prev_groups_inv[i] - full_prev_groups[i];
        if (first_status[i] && second_status[i] && abs(diff.x) <=0.5 && abs(diff.y) <= 0.5) {
            final_corners.push_back(curr_corners[i]);
        }
        else {
            final_corners.push_back(full_prev_groups[i]);
        }
    }

    size_t curr_off = 0;
    for (int i = 0; i < prev_features_groups.size(); i++) {
        std::vector<cv::Point2f> curr_group;
        curr_group.insert(curr_group.end(), final_corners.begin(), final_corners.begin()+curr_off+prev_features_groups[i].size());
        curr_features_groups.push_back(curr_group);
        curr_off += prev_features_groups[i].size();
    }
    
    return true;
}

bool buildLKPyr(const cv::Mat& gray_img, std::vector<cv::Mat>& pyr) {
    if (gray_img.empty()) {
        std::cout << "Cannot build pyramid from empty img" << std::endl;
        return false;
    }

    const cv::Size win_size = cv::Size(21, 21);
    const int max_level = 3;
    const bool with_derivatives = true;
    const int pyr_border =  cv::BORDER_REFLECT_101;
    const int derive_border = cv::BORDER_CONSTANT;
    const bool reuse_input_img = true;

    pyr.clear();

    cv::buildOpticalFlowPyramid(gray_img, pyr, win_size, max_level, with_derivatives, pyr_border, derive_border, reuse_input_img);
    
    return true;
}


bool acquireFrameFromBuffer(cv::Mat& output_frame) {
    if (!frames_queue.empty()) {
        sampler_lock.lock();
        output_frame = frames_queue.front();
        frames_queue.pop();
        sampler_lock.unlock();

        if (!output_frame.empty()) {
            return true;
        }
    }

    return false;
}

void framesSamplerThread()
{
    cv::VideoCapture cap;
    if (tracker_confs.is_webcam) {
        std::cout << "video capture opened with resource" << tracker_confs.resource << std::endl;
        cap.open(tracker_confs.video_path);
    }
    else {
        std::cout << "video capture with video file" << tracker_confs.video_path << std::endl;
        cap.open(tracker_confs.video_path);
    }

    if (!cap.isOpened()) {
        std::cout << "sampling thread failed to open video capture" << std::endl;
    }

    cv::Mat frame;
    while (is_program_running) {
        cap >> frame;
        sampler_lock.lock();
        frames_queue.push(frame);
        sampler_lock.unlock();
    }
    
    cap.release();
    std::cout << "sampling thread ended" << std::endl;
}

cv::Rect getTranslatedROI(const cv::Rect& src_ROI, const cv::Rect& container_ROI) {
    return cv::Rect(container_ROI.x + src_ROI.x, container_ROI.y + src_ROI.y,
            src_ROI.width, src_ROI.height);
}

bool detectFacialROIs(const cv::Mat& gray_img, std::vector<FacialROIs>& facial_ROIs_vector) {
    facial_ROIs_vector.clear();
    static bool already_loaded = false;
    if (!already_loaded) {
        if (!face_classifier.load(tracker_confs.face_Haar_features_path)) {
            std::cout << "Failed to load face classifier with file" << tracker_confs.face_Haar_features_path << std::endl;
            return false;
        }
        if (!eye_classifier.load(tracker_confs.eye_Haar_features_path)) {
            std::cout << "Failed to load eye classifier with file" << tracker_confs.eye_Haar_features_path << std::endl;
            return false;
        }
        if (!nose_classifier.load(tracker_confs.nose_Haar_features_path)) {
            std::cout << "Failed to load nose classifier with file" << tracker_confs.nose_Haar_features_path << std::endl;
            return false;
        }
        if (!mouth_classifier.load(tracker_confs.mouth_Haar_features_path)) {
            std::cout << "Failed to load mouth classifier with file" << tracker_confs.mouth_Haar_features_path << std::endl;
            return false;
        }
        already_loaded = true;
    }

    std::vector<cv::Rect> faces_ROIs;
    const double scale_factor = 1.1;
    const int min_neighbors = 3;
    const int flags = 0;
    const cv::Size min_size = cv::Size();
    const cv::Size max_size = cv::Size();
    
    face_classifier.detectMultiScale(gray_img, faces_ROIs, scale_factor, min_neighbors, flags, min_size, max_size);
    if (faces_ROIs.empty()) {
        std::cout << "No faces detected in image" << std::endl;
        return false;
    }
    const double percents_of_reduction = 30.0;
    for (const cv::Rect& face_ROI: faces_ROIs) {
        // detect eyes, nose and mouth inside current ROI.
        FacialROIs facial_ROIs;
        facial_ROIs.face = face_ROI;
        cv::Mat ROI_img = gray_img(face_ROI);
        cv::Rect eyes_region(face_ROI.x, face_ROI.y, face_ROI.width, face_ROI.height/2);
        cv::Rect mouth_region(face_ROI.x, face_ROI.y + face_ROI.height/2, face_ROI.width, face_ROI.height/2);
        cv::Mat lower_half_face = gray_img(mouth_region);
        cv::Mat higher_half_face = gray_img(eyes_region);
        
        std::vector<cv::Rect> eyes_ROIs;
        eye_classifier.detectMultiScale(
                higher_half_face, eyes_ROIs, scale_factor, min_neighbors, flags, min_size, max_size);
        if (eyes_ROIs.size() == 1) {
            facial_ROIs.eyes.push_back(getReducedROI(getTranslatedROI(eyes_ROIs[0], eyes_region), percents_of_reduction));
        }
        else if (eyes_ROIs.size() == 1) {
            facial_ROIs.eyes.push_back(getReducedROI(getTranslatedROI(eyes_ROIs[0], eyes_region), percents_of_reduction));
            facial_ROIs.eyes.push_back(getReducedROI(getTranslatedROI(eyes_ROIs[1], eyes_region), percents_of_reduction));
        }
        else {
            facial_ROIs.eyes.push_back(getReducedROI(eyes_region, percents_of_reduction));
        }

        std::vector<cv::Rect> nose_ROIs;
        nose_classifier.detectMultiScale(
                ROI_img, nose_ROIs, scale_factor, min_neighbors, flags, min_size, max_size);
        if (nose_ROIs.size() == 1) {
            facial_ROIs.nose = getReducedROI(getTranslatedROI(nose_ROIs[0], face_ROI), percents_of_reduction);
        }
        else {
            facial_ROIs.nose = getReducedROI(face_ROI, percents_of_reduction*2);
        }

        std::vector<cv::Rect> mouth_ROIs;
        mouth_classifier.detectMultiScale(
                lower_half_face, mouth_ROIs, scale_factor, min_neighbors, flags, min_size, max_size);
        if (mouth_ROIs.size() == 1) {
            facial_ROIs.mouth = getReducedROI(getTranslatedROI(mouth_ROIs[0], mouth_region), percents_of_reduction);
        }
        else {
            facial_ROIs.mouth = getReducedROI(mouth_region, percents_of_reduction);
        }
        facial_ROIs_vector.push_back(facial_ROIs);
   }
   std::cout << "facial features fully detected successfully for at least one face in the img" << std::endl;
   return true;
} 

void convertFacialROIsToPolys(const std::vector<FacialROIs>& facial_ROIs_vector, std::vector<std::vector<cv::Point2f> >& ROIs)
{
    ROIs.clear();
    for (const FacialROIs& facial_ROIs: facial_ROIs_vector) {
        std::vector<cv::Point2f> ROI;
        convertRectToPts(facial_ROIs.face, ROI);
        ROIs.push_back(ROI);
    }
}

void drawFacialROIsVector(cv::Mat& img, const std::vector<FacialROIs>& facial_ROIs_vector)
{
    const cv::Scalar color(255, 255, 255);
    const int thickness = 1;
    const int line_type = cv::LINE_8;
    const int shift = 0;

    for (const FacialROIs& facial_ROIs: facial_ROIs_vector) {
        cv::rectangle(img, facial_ROIs.face, color, thickness, line_type, shift);
        for (const cv::Rect& eye_ROI: facial_ROIs.eyes) {
            cv::rectangle(img, eye_ROI, color, thickness, line_type, shift);
        }
        cv::rectangle(img, facial_ROIs.nose, color, thickness, line_type, shift);
        cv::rectangle(img, facial_ROIs.mouth, color, thickness, line_type, shift);
    }
}

void drawFacialFeaturesGroups(cv::Mat& img, const std::vector<std::vector<cv::Point2f> >& facial_features_groups)
{
    const int radius = 2;
    const cv::Scalar color(0, 255, 0);
    const int thickness = 1;
    const int line_type = cv::LINE_8;
    const int shift = 0;

    for (const std::vector<cv::Point2f>& group: facial_features_groups) {
        for (const cv::Point2f& pt: group) {
            cv::circle(img, pt, radius, color, thickness, line_type, shift);
        }
    }
}

cv::Rect getReducedROI(const cv::Rect& src_ROI, double percents) {
    if (percents <= 0 || percents >=100) {
        return src_ROI;
    }
    
    double dx = src_ROI.x;
    double dy = src_ROI.y;
    double dw = src_ROI.width;
    double dh = src_ROI.height;

    double wred = (percents*dw/100.0) * 0.5;
    double hred = (percents*dh/100.0) * 0.5;

    return cv::Rect(
            (int)(dx + wred),
            (int)(dy + hred),
            (int)(dw - 2*wred),
            (int)(dh - 2*hred));
}

bool findFeaturesInsideFacialROIs(const cv::Mat& gray_img, const std::vector<FacialROIs>& facial_ROIs_vector, 
        std::vector<std::vector<cv::Point2f> >& features_groups) 
{
    features_groups.clear();
    const int max_corners = MAX_CORNERS_TO_DETECT_INSIDE_ROI;
    const double quality_level = 0.01;
    const double min_distance = 10;
    const int block_size = 3;
    const bool use_haar_detector = false;
    const double k = 0.04;

    cv::Size win_size = cv::Size(5, 5);
    cv::Size zero_zone = cv::Size(-1, -1);
    cv::TermCriteria criteria = cv::TermCriteria(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 40, 0.001);
    for (const FacialROIs& facial_ROIs: facial_ROIs_vector) {
        cv::Mat mask = cv::Mat::zeros(gray_img.size(), CV_8UC1);
        for (const cv::Rect& eye_ROI: facial_ROIs.eyes) {
           mask(eye_ROI) = 255;
        }
        mask(facial_ROIs.nose) = 255;
        mask(facial_ROIs.mouth) = 255;
        
        std::vector<cv::Point2f> features_group;
        cv::goodFeaturesToTrack(gray_img, features_group, max_corners, quality_level, min_distance,
                mask, block_size, use_haar_detector, k);

        if (features_group.size() <= 0) {
            std::cout << "the features detector failed to detect features inside " << facial_ROIs.face << std::endl;
            return false;
        }
        cv::cornerSubPix(gray_img, features_group, win_size, zero_zone, criteria);
        features_groups.push_back(features_group);
    }
    
    std::cout << "Features detection succeeded inside facial ROIs" << std::endl;
    return true;
}

void faceThread(int queue_index)
{
    std::queue<FaceWindowThreadParams>& queue = face_window_threads_buffers[queue_index];
    int delay = getMainLoopDelayByVideoFPS();
    cv::VideoWriter output_video;
    bool is_video_writer_initialized = false;

    while (is_program_running) {
        if (face_windows_params[queue_index].active && face_windows_params[queue_index].created) {
            if (!queue.empty()) {
                mtxs[queue_index].lock();
                FaceWindowThreadParams& fwtp = queue.front();
                mtxs[queue_index].unlock();
                performRigidTransformOnImg(fwtp.inv, fwtp.img);
                cv::imshow(face_windows_params[queue_index].name, fwtp.img(curr_facial_ROIs_vector[queue_index].face));

                if (!is_video_writer_initialized) {
                    if (!tracker_confs.is_record) {
                        std::string base_name = tracker_confs.output_video_name;
                        std::string output_video_path = base_name.substr(0, base_name.find_last_of(".")) + 
                                                        "-" + face_windows_params[queue_index].name + ".avi";
                        output_video.open(output_video_path, CV_FOURCC('D', 'I', 'V', 'X'), 
                                tracker_confs.fps, curr_facial_ROIs_vector[queue_index].face.size(), true);
                        
                        if (!output_video.isOpened()) {
                            std::cout << "Could not open the output video for writer: " << output_video_path << std::endl;
                            is_program_running = false;
                            break;
                        }
                        is_video_writer_initialized = true;
                    }
                }

                if (is_video_writer_initialized) {
                    output_video << fwtp.img(curr_facial_ROIs_vector[queue_index].face);
                }
                queue.pop();
            }
        }
        
        int key = cv::waitKey(delay);
        if (key == EXIT_KEY_CODE) {
            std::cout << "User stoped main loop" << std::endl;
            is_program_running = false;
            break;
        }
    }
}

int getMainLoopDelayByVideoFPS()
{
    int fps = tracker_confs.fps;
    if (!tracker_confs.is_webcam) {
        fps = 1000 / fps;
    }
    return fps;
}



