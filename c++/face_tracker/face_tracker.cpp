#include <iostream>
#include <vector>
#include <queue>
#include <thread>

#define TRACKER_CONF_PATH ("./config/tracker_con.ini")
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
#define CONF_FILED_VIDEO_PATH ("VIDEO_PATH")
#define CONF_FILED_FPS ("FPS")
#define CONF_FILED_IS_RECORD ("IS_RECORD")
#define CONF_FILED_OUTPUT_VIDEO_PATH ("OUTPUT_VIDEO_PATH")
#define CONF_FIELD_HAAR_FACE_FEATURES_PATH ("HAAR_FACE_FEATURES_PATH")
#define CONF_FIELD_EYE_FEATURES_PATH ("HAAR_EYE_FEATURES_PATH")
#define CONF_FIELD_NOSE_FEATURES_PATH ("HAAR_NOSE_FEATURES_PATH")
#define CONF_FIELD_MOUTH_FEATURES_PATH ("HAAR_MOUTH_FEATURES_PATH")

typedef struct
{
    bool is_webcam;
    int resource;
    std::string video_path;
    int fprs;
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
bool is_program_running = true
TrackerConfigurations tracker_configs;

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
    std::vector<FaceWindowParams>* face_window_params;
} MouseCallbackData;

typedef struct
{
    cv::Mat inv;
    cv::Mat img;
} FaceWindowThreadParams;

cv::CascadeClassifier faceClassifier;
cv::CascadeClassifier eyeClassifier;
cv::CascadeClassifier noseClassifier;
cv::CascadeClassifier mouthClassifer;

void loadConfiguration();
cv::Rect getTranslatedROI(const cv::Rect& src_ROI, const cv::Rect& container_ROI);
bool detectFacialROIs(const cv::Mat& gray_img, std::vector<FacialROIs>& facial_ROIs);
bool findFeaturesInFacialROIs(const cv::Mat& gray_img, 
        const std::vector<FacialROIs>& facial_ROIs,
        std::vector<std::vector<cv::Point2f>& features_groups);
bool buildLKPyr(const cv::Mat& gray_img, std::vector<cv::Mat>& pyr);
bool calcLKOpticalFlowForFeaturesGroups(const std::vector<cv::Mat>& prev_pyr,
        const std::vector<cv::Mat>& curr_pyr,
        const std::vector<std::vector<cv::Point2f> >& prev_features_groups,
        std::vector<std::vector<cv::Point2f> >&curr_features_groups);

void drawFacialROIs(cv::Mat& img, const std::vector<FacialROIs>& facial_ROIs);
void drawFacialFeaturesGroups(cv::Mat& img
        const std::vector<std::vector<cv::Point2f> >& features_groups);
void drawPoly(cv::Mat& img, const std::vector<cv::Point2f>& pts);
void drawROIs(cv::Mat& img, const std::vector<std::vector<cv::Point2f> >& ROIs);
void convertRectROIsToPolys(const std::vector<FacialROIs>& facial_ROIs,
        std::vector<std::vector<cv::Point2f> >& ROIs);

bool getRigidTransformationMatrices(
        const std::vector<std::vector<cv::Point2f> >& curr_features_groups,
        const std::vector<std::vector<cv::Point2f> >& prev_features_groups,
        const std::vector<std::vector<cv::Point2f> >& init_ROIs,
        const std::vector<std::vector<cv::Point2f> >& curr_ROIs,
        std::vector<cv::Mat>& trans_matrices,
        std::vector<cv::Mat>& trans_matrices_inv);
void performRigidTransformationOnROIs(
        const std::vector<cv::Mat>& trans_matrices,
        std::vector<std::vector<cv::Point2f> >& ROIs,
        cv::Size& img_size);
void performRigidTransformationOnImg(const cv::Mat& matrix, cv::Mat& img);
bool acquireFrameFromBuffer(cv::Mat& output_frame);
void frames_sampler_thread();
bool is_point_in_ROI(const cv::Point2f& pt, const std::vector<cv::Point2f>& ROI);
cv::Rect getReducedROI(const cv::Rect& src_ROI, double percents);
cv::Rect getEnlargeROI(const cv::Rect& src_ROI, double percents);

void mainWindowMouseCallback(int event, int x, int y, int, void* data);

std::mutex mtxs[MAX_MUTEXES_BUFFER_SIZE];
std::vector<std::queue<WindowThreadParams> > window_thread_params;
std::vector<FacialROIs> curr_facial_ROIs;
std::vector<FaceWindowParams> face_window_params;

void face_thread_function(int queue_index);

int getMainLoopDelayByVideoFps();

int main(int argc, char** argv)
{
    loadConfigurations();
    int delay = getMainLoopDelayByVideoFPS();
    cv::VideoCapture cap;
    std::thread streaming_job;
    
    if (tracker_confs.is_webcam) {
        streaming_job = std::thread(frames_sampler_thread);
        std::cout << "start video streaming job" << std::endl;
    } 
    else {
        cap.open(tracker_confs.video_path);
        if (!cap.isOpened()) {
            std::cout << "failed to open video capture from file" << tracker_confs.video_path << std::endl;
            return EXIT_FAILURE
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
            good_sampling = acquireFrameBuffer(curr_bgr_frame);
        }
        else {
            cap >> curr_bgr_frame;
            if (curr_bgr_frame.empty()) {
                good_sampling = false;
                program_running = false;
                break;
            }   
        }
        if (!is_video_writer_initialized) {
            if (tracker_confs.is_record) {
                output_video.open(ouput_video_path, CV_FOUR('D', 'I', 'V', 'X'),
                        tracker_confs.fps, curr_bgr_frame.size(), true);
                if (!output_video.isOpened()) {
                    std::cout << "cannot open output  video writer" << std::endl;
                    program_running = false;
                    break;
                }
                is_video_writer_initialized = true;
            }
        }

        if (!is_window_resized) {
            cv::resizeWindow(MAIN_WINDOW_NAME, curr_bgr_frame.size().width, 
                    curr_bgr_frame.size().height);
            is_window_resized = true
        }

        curr_bgr_frame_copy = curr_bgr_frame.clone();
        cv::cvtColor(curr_bgr_frame_copy, curr_gray_frame, cv::COLOR_RGB2GRAY);
        // histogram equalization for areas with inconsistent illumination
        cv::equalizeHist(curr_gray_frame, curr_gray_frame); 
        // --------------------------------------
        //           initial processing
        // -------------------------------------- 
        if (initial_processing_needed) {
            bool facial_ROIs_detection_succeeded = detectFacialROIs(curr_gray_frame, curr_facial_ROIs);
            if (facial_features_detection_succeeded) {
                initial_proessing_needed = false;
                convertRectROIsToPolys(curr_facial_ROIs, curr_ROIs);
                init_ROIs = curr_ROIs;
                
                trans_matrices.clear();
                trans_matrices_inv.clear();
                for (int i = 0; i < curr_facial_ROIs.size(); i++) {
                    trans_matrices.push_back(cv::Mat());
                    trans_matrices_inv.push_back(cv::Mat());
                }
                
                for (int i = 0; i < curr_facial_ROIs.size(); i++) {
                    FaceWindowParams window_params;
                    window_params.active = window_params.created = false;
                    std::string window_name = FACE_WINDOW_NAME;
                    char buff[3];
                    itoa(i+1, buff, 10);
                    window_name += buff;
                    window_params.name = window_name;
                    face_window_params.push_back(window_params);
                    face_thread_params.push_back(std::queue<WindowThreadParams>());
                }

                MouseCallbackData mouse_callback_data;
                mouse_callback_data.windows_params = &face_window_params;
                mouse_callback_data.curr_ROIs = &curr_ROIs;
                cv::setMouseCallback(MAIN_WINDOW_NAME, main_window_mouse_callbacl, &mouse_callback_data);
                
                // initialize stabilizers threads
                for (int i = 0; i < curr_facial_ROIs.size(); i++) {
                    face_threads.push_back(std::pair<std::thread, bool>(std::thread(face_thread_function, i), true));
                }
            }
        }
        // -----------------------------------------------
        //                rest frames
        // -----------------------------------------------
        else {
            buildLKPyr(prev_gray_frame, prev_pyr);
            buildLKPyr(curr_gray_frame, curr_pyr);
            cv::calcLKOpticalFlowForAllTheFeatureGroups(prevPyr, currPyr, prev_feature_groups, curr_feature_groups);
            if (getRigidTransformationMatrices(curr_features_groups, prev_feature_groups, init_ROIs, curr_ROIs, 
                        trans_matrices, trans_matrices_inv)) {
                performRigidTransormationOnROIs(trans_matrices, curr_ROIs, curr_bgr_frame_copy.size());
            }

            for (int i = 0; i < face_window_params.size(); i++) {
                if (face_window_params[i].active  && face_window_params[i].created) {
                    WindowThreadParams wtp;
                    wtp.img = curr_bgr_frame_copy.clone();
                    wtp.inv = trans_matrices_inv[i];
                    mtxs[i].lock();
                    face_thread_buffers[i].push(wtp);
                    mtxs[i].unlock();
                }
            }
        }

        drawFacialFeaturesGroups(curr_features_groups, curr_bgr_frame_copy);
        drawROIs(curr_ROIs, curr_bgr_frame_copy);
        cv::imshow(MAIN_WINDOW_NAME, curr_bgr_frame_copy);
        cv::swap(prev_gray_frame, curr_gray_frame);
        prev_feautures_groups.clear();
        for (std::vector<cv::Point2f>& cg: curr_features_groups) {
            std::vector<cv::Point2f> pg;
            std::swap(pg, cg);
            prev_features_groups.push_back(pg);
        }

        prev_pyr.clear();
        for (cv::Mat& cm: curr_pyr) {
            cv::Mat pm;
            cv::swap(pm, cm);
            prevPyr.push_back(pm);
        }

        if (is_video_writer_initialized) {
            output_video << curr_bgr_frame_copy;
        }

        int key = cv::waitKey(delay);
        if (key == EXIT_KEY_CODE) {
            std::cout << "user stopped main loop" << std::endl;
            program_running = false;
            break; 
        }
    }

    std::cout << "main loop ended" << std::endl;
    cv::destropyAllWindows();
    std::cout << "resources released" << std::endl;
    std::cout << "program ended successfully" << std::endl;
    
    if (track_confs.is_webcam) {
        streaming_job.join()
    }

    for (int i = 0; i < face_threads.size(); i++) {
        if (faces_threads[i].second) {
            faces_threads[i].first.join();
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
        tracker_confs.resource =; 
        tracker_confs.video_path = "";
        tracker_confs.fps = 30;
        tracker_confs.is_record = false;
        tracker_confg.output_videoname = "";
        tracker_confs.face_haar_features_path = 
        trakcer_confs.eye_haar_features_path = 
        tracker_confs.nose_haar_features_path = 
        trakcer_confs.mouth_haar_features_path = 
    }

    std::string line;
    char delimeter = '=';
    // read lines
    while (std::getline(ifs, line)) {
        std::string field = line.substr(0, line.find(delimeter));
        str::string field_value = line.substr(line.find(delimeter) + 1);
        std::cout << line << std::endl;

        if (field == CONF_FIELD_IS_CAMERA) {
            std::istringstream iss(field_value);
            iss >> tracker_confs.is_webcam;
        }
        else if (field == CONF_FIELD_RESOURCE) {



}   
