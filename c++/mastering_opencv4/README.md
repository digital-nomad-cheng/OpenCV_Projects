## Chapter 01 

1. isdigit

   ```c
   int isdigit ( int c );
   ```

   check if character is decimal digit.

2. atoi

   ```c
   int atoi (const char * str);
   ```

   convert string to integer

3. std::err: standar error stream

4. cv::Laplician() use gray images

5. cv::Scarr() edge detection

6. memset

   ```void * memset ( void * ptr, int value, size_t num );```

   Sets the first *num* bytes of the block of memory pointed by *ptr* to the specified *value* (interpreted as an `unsigned char`). 

7. cv::Mat.step: how many bytes each row occupies

8. set opencv camera resolution

   ```c++
   camera.set(cv::CV_CAP_PROP_FRAME_WIDTH, 640); 
   camera.set(cv::CV_CAP_PROP_FRAME_HEIGHT, 480);
   ```

9. two tricks applying bilateral filter:
   + processing smaller image size
   + stack mulitple smaller size of bilateral filter kernel

## Chapter 03 
1. kinds of landmark detectors:
   + holistic methods: model complete appearance of the face's pixel intensities(AAM)
   + constrained local model: examine local patches around each landmark in combination with a global model
   + regression methods: iteratively try to predict landmark locations using a cascade of small updates learned by regressors

2. OpenCV face detection: boosted cascade classifier
   ```c++
   void detectFace(const Mat &image, std::vector<Rect> &faces,
      CascadeClassifier &face_cascade) 
   {
      cv::Mat gray; // cascade classifier works best on grayscale images
      if (image.channels() > 1) {
         cvtColor(image, gray, COLOR_BGR2RAY);
      } else {
         gray = image.clone();
      }
      // histogram equalization helps face detection
      equalizeHist(gray, gray);
      faces.clear();
      face_cascade.detectMultiScale(gray,
                                    faces,
                                    1.4,
                                    3,
                                    CASCADE_SCALE_IMAGE+CASCADE_FIND_BIGGEST_OBJECT);
   }
   ```
3. OpenCV face landmarks
   ```c++
   const string landmark_file = "data/lbfmodel.yaml";
   std::Ptr<Facemark> facemark = createFacemarkLBF();
   facemark->loadModel(landmark_file);
   std::cout << "Loaded facemark LBF model" << std::endl;

   // detect faces and localize landmarks
   std::vector<Rect> faces;
   faceDetector(img, faces, face_cascade);
   if (faces.size() != 0) {
      // We assume a single face so we look at the first only
      cv::rectangle(img, faces[0], Scalar(255, 0, 0), 2);
      std::vector<std::vector<Point2f> > shapes;
      if (facemark->fit(img, faces, shapes)) {
        drawFacemarks(img, shapes[0], cv::Scalar(0, 0, 255));
      }
   } else {
      std::cout << "Faces not detected." << std::endl;
   }
   ```

4. Estimating face direction from landmarks
   [more simple explanation](https://www.learnopencv.com/head-pose-estimation-using-opencv-and-dlib/)
   + opencv calib3d: cv::solvePnP
   + camera focal length can be estimed by the image width
   + camera focal center can be estimed by the image center
   ```c++
   // 2D image points, normally obtained from landmark model
   std::vector<cv::Point2d> image_points;
   image_points.push_back( cv::Point2d(359, 391) );    // Nose tip
   image_points.push_back( cv::Point2d(399, 561) );    // Chin
   image_points.push_back( cv::Point2d(337, 297) );     // Left eye left corner
   image_points.push_back( cv::Point2d(513, 301) );    // Right eye right corner
   image_points.push_back( cv::Point2d(345, 465) );    // Left Mouth corner
   image_points.push_back( cv::Point2d(453, 469) );    // Right mouth corner
   // 3D model points
   std::vector<cv::Point3d> model_points;
   model_points.push_back(cv::Point3d(0.0f, 0.0f, 0.0f));               // Nose tip
   model_points.push_back(cv::Point3d(0.0f, -330.0f, -65.0f));          // Chin
   model_points.push_back(cv::Point3d(-225.0f, 170.0f, -135.0f));       // Left eye left corner
   model_points.push_back(cv::Point3d(225.0f, 170.0f, -135.0f));        // Right eye right corner
   model_points.push_back(cv::Point3d(-150.0f, -150.0f, -125.0f));      // Left Mouth corner
   model_points.push_back(cv::Point3d(150.0f, -150.0f, -125.0f));       // Right mouth corner
   // camera parameters
   double focal_length = im.cols; // approximate focal length with image width
   Point2d center = cv::Point2d(im.cols/2,im.rows/2);
   // cv::Mat camera_matrix = (cv::Mat_<double>(3,3) << focal_length, 0, center.x, 0 , focal_length, center.y, 0, 0, 1);
   cv::Matx33f camera_matrix(focal_length, 0, center.x,
                             0, focal_width, center.y,
                             0, 0, 1.0f); 
   cv::Mat dist_coeffs = cv::Mat::zeros(4,1,cv::DataType<double>::type); // assume no lens distortion
   // output rotation and translation
   cv::Mat rotation_vector; 
   cv::Mat translation_vector;
   
   cv::solvePnP(model_points, image_points, camera_matrix, dist_coeffs, rotation_vector, translation_vector);
   ```
5. boost format
   [boost format doc](https://www.boost.org/doc/libs/1_54_0/libs/format/doc/format.html)
   ```c++
   using $ = boost::format
   str($("MED: %.3f") % calMeanEuclideanDistance(shapes[0], ground_truth))
   ```

## Chapter 05
1. car plate recognition procedure
   ![procedure.jpg](https://www.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/e525e709-4de5-4d38-8ead-5c5d9cdf5e45.png)
   + segmentation
   + feature extraction
   + classification

2. 
