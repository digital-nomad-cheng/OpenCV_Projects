

# Chapter 01 Cartoonifier and Skin Color Analysis on the RaspberryPi

This chapter will show how to write some  image processing filters for desktops and for small embedded systems  such as Raspberry Pi. First, we develop for the desktop (in C/C++) and  then port the project to Raspberry Pi, since this is the recommended  scenario when developing for embedded devices. This chapter will cover  the following topics:

- How to convert a real-life image to a sketch drawing
- How to convert to a painting and overlay the sketch to produce a cartoon
- A scary evil mode to create bad characters instead of good characters
- A basic skin detector and skin color changer, to give someone green alien skin
- Finally, how to create an embedded system based on our desktop application

Note that an **embedded system** is basically a computer motherboard placed inside a product or device, designed to perform specific tasks, and **Raspberry Pi** is a very low-cost and popular motherboard for building an embedded system:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/b3311efd-593c-425d-beb8-dd7e897c8c8a.png)

The preceding picture shows what you could  make after this chapter: a battery-powered Raspberry Pi plus screen you  could wear to Comic Con, turning everyone into a cartoon!

We want to make the real-world camera frames automatically look like they are from a cartoon. The basic idea is to  fill the flat parts with some color and then draw thick lines on the  strong edges. In other words, the flat areas should become much more  flat and the edges should become much more distinct. We will detect  edges, smooth the flat areas, and draw enhanced edges back on top, to  produce a cartoon or comic book effect.

When developing an embedded computer vision  system, it is a good idea to build a fully working desktop version first before porting it to an embedded system, since it is much easier to  develop and debug a desktop program than an embedded system! So, this  chapter will begin with a complete Cartoonifier desktop program that you can create using your favorite IDE (for example, Visual Studio, XCode,  Eclipse, or QtCreator). After it is working properly on your desktop,  the last section shows how to create an embedded system based on the  desktop version. Many embedded projects require some custom code for the embedded system, such as to use different inputs and outputs, or use  some platform-specific code optimizations. However, for this chapter, we will actually be running identical code on the embedded system and the  desktop, so we only need to create one project.

The application uses an **OpenCV** GUI window, initializes the camera, and with each camera frame it calls the cartoonifyImage() function, containing most of the code in this chapter. It then displays the  processed image in the GUI window. This chapter will explain how to  create the desktop application from scratch using a USB webcam and the  embedded system based on the desktop application, using the Raspberry Pi Camera Module. So, first you will create a desktop project in your  favorite IDE, with a main.cpp file to hold the  GUI code given in the following sections, such as the main loop, webcam  functionality, and keyboard input, and you will create a cartoon.cpp file with the image processing operations with most of this chapter's code in a function called cartoonifyImage().

# Chapter 8 iOS Panoramas with the Stitching Module

Panoramic imaging has existed since the early days of photography. In those ancient times, roughly 150 years ago, it was called the art of **panography**, carefully putting together individual images using tape or glue to  recreate a panoramic view. With the advancement of computer vision,  panorama stitching became a handy tool in almost all digital cameras and mobile devices. Nowadays, creating panoramas is as simple as swiping  the device or camera across the view, the stitching calculations happen  immediately, and the final expanded scene is available for viewing. In  this chapter, we will implement a modest panoramic image stitching  application on the iPhone using OpenCV's precompiled library for iOS. We will first examine a little of the math and theory behind image  stitching, choose the relevant OpenCV functions to implement it, and  finally integrate it into an iOS app with a basic UI.

The following topics will be covered in this chapter:

- Introduction to the concept of image stitching and panorama building
- OpenCV's image stitching module and its functions
- Building a Swift iOS application UI for panorama capturing
- Integrating OpenCV component written in Objective C++ with the Swift application

# Technical requirements

The following technologies and installations are required to recreate the contents of this chapter:

- macOSX machine (for example, MacBook, iMac) running macOS High Sierra v10.13+
- iPhone 6+ running iOS v11+
- Xcode v9+
- CocoaPods v1.5+: https://cocoapods.org/
- OpenCV v4.0 installed via CocoaPods

Build instructions for the preceding components, as well as the code  to implement the concepts presented in this chapter, will be provided in the accompanying code repository.

The code for this chapter can be accessed via GitHub: https://github.com/PacktPublishing/Mastering-OpenCV-4-Third-Edition/tree/master/Chapter_08.

# Panoramic image stitching methods

Panoramas are essentially multiple images fused together into a  single image. The process of panorama creation from multiple images  involves many steps; some are common to other computer vision tasks,  such as the following:

- Extracting 2D features
- Matching pairs of images based on their features
- Transforming or warping images to a communal frame 
- Using (blending) the seams between the images for the pleasing continuous effect of a larger image

Some of these basic operations are also commonplace in **Structure-from-Motion** (**SfM**), **3D reconstruction** , **visual odometry**, and **simultaneous localization and mapping** (**SLAM**). We've already discussed some of these in [Chapter 2](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/3229555f-ce7e-4330-9197-9cc4169f230f.xhtml), *Explore Structure from Motion with the SfM Module* and [Chapter 7](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/9a5f3e9a-3065-494b-a893-2139e504925a.xhtml), *Android Camera Calibration and AR Using the ArUco Module*. The following is a rough image of the panorama creation process:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/99415d7a-f1d5-4fcd-9506-57e08f93cb42.png)

In this section, we will briefly review feature matching, camera pose estimation, and image warping. In reality, panorama stitching has  multiple pathways and classes, depending on the type of input and  required output. For example, if the camera has a fisheye lens (with an  extremely high degree view angle) a special process is needed.

# Feature extraction and robust matching for panoramas

We create panoramas from overlapping images. In the overlapping region, we look for common visual features that **register** (align) the two images together. In SfM or SLAM, we do this on a frame-by-frame basis, looking for matching features in a real-time video sequence  where the overlap between frames is extremely high. However, in  panoramas we get frames with a big motion component between them, where  the overlap might be as low as just 10%-20% of the image. At first, we  extract image features, such as the **scale invariant feature transform (SIFT)**, **speeded up robust features** (**SURF**), **oriented BRIEF** (**ORB**), or another kind of feature, and then match them between the images in  the panorama. Note the SIFT and SURF features are protected by patents  and cannot be used for commercial purposes. ORB is a considered a free  alternative, but not as robust.

The following image shows extracted features and their matching:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/1e56a991-ecca-4474-8d6a-713ef5fa96bf.png)

# Affine constraint

For a robust and meaningful pairwise matching, we often apply a geometric constraint. One such constraint can be an **affine transform**, a transform that allows only for scale, rotation, and translation. In  2D, an affine transform can be modeled in a 2 x 3 matrix:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/997365e9-c351-409a-83c9-164e9c622095.png)

To impose the constraint, we look for an affine transform $\hat{M}$ minimizes the distance (error) between matching points from the left $X_i^L$ and right $X_i^R$ images.

# Random sample consensus (RANSAC)

In the preceding image, we illustrate the fact that not all points  conform to the affine constraint, and most of the matched pairs are  discarded as incorrect. Therefore, in most cases we employ a  voting-based estimation method, such as **random sample consensus (RANSAC)**, where a group of points is chosen at random to solve for a hypothesis of *M* directly (via a homogeneous linear system) and then a voting is cast between all points to support or reject this hypothesis.

The following is a pseudo-algorithm for RANSAC:

1. Find matches between points in image *i* and image *j*.

2. Initialize the hypothesis for the transform between image *i* and *j*, with minimal support.

3. While not converged: 

   1. Pick a small random set of point-pairs. For an affine transform, three pairs will suffice.

   2. Calculate the affine transform *T* directly based on the pairs set, for instance with a linear equation set

   3. Calculate the support. For each point p in the entire i, j matching: 

      If the distance (the **error**) between the transformed point in image *j* and the matched point in image *i* is within a small threshold *t*:  $\left \|P_i - T_{p_j}  \right \| < t$, add 1 to the support counter.

   4. If the support count is bigger than the current hypothesis' support, take *T* as the new hypothesis.

   5. Optional: if the support is large enough (or a different breaking policy is true), break; otherwise, keep iterating.

4. Return the latest and best supported hypothesis transform.

5. Also, return the **support mask**: a binary variable stating whether a point in the matching was supporting the final hypothesis.

The output of the algorithm will provide the transform that has the  highest support, and the support mask can be used to discard points that are not supportive. We can also reason about the number of supporting  points, for example, if we observe less than 50% supporting points, we  can deem this match as bad and not try to match the two images at all.

There are alternatives to RANSAC, such as the **least median squares (LMedS)** algorithm, which is not too different from RANSAC: instead of counting  supporting points, it calculates the median of the square error for each transform hypothesis, and finally return the hypothesis with the least  median square error.

# Homography constraint

While affine transforms are useful for stitching scanned documents (for example, from a flatbed scanner), they cannot be used for stitching photo panoramas.  For stitching photos, we can employ the same process to find a **homography**, a transform between one plane and another, instead of an affine  transform, which has eight degrees of freedom, and is represented in a 3 x 3 matrix as follows:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/fb58daf4-0a86-4cc4-8890-838715adcbeb.png)

Once a proper matching has been found, we can find an ordering of the images to sequence them for the panorama, essentially to understand how the images relate to one another. In most cases, in panoramas the  assumption is that the photographer (camera) is standing still and only  rotating on its axis, sweeping from left to right, for example.  Therefore, the goal is to recover the rotation component between the  camera poses. Homographies can be decomposed to recover rotation, if we  regard the input as purely rotational: ![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/8ff4d431-fad9-4373-8bd1-2f4a3a601d42.png). If we assume the homography was originally composed from the camera intrinsic (calibration), matrix *K,* and a 3 x 3 rotation matrix *R*, we can recover *R* if we know *K*. The intrinsic matrix can be calculated by camera calibration ahead of  time, or can be estimated during the panorama creation process.



# Bundle Adjustment

When a transformation has been achieved *locally* between all photo *pairs*, we can further optimize our solution in a *global* step. This is called the process of **bundle adjustment**, and is widely constructed as a global optimization of all the  reconstruction parameters (camera or image transforms). Global bundle  adjustment is best performed if all the matched points between images  are put in the same coordinate frame, for example, a 3D space, and there are constraints that span more than two images. For example, if a  feature point appears in more than two images in the panorama, it can be useful for *global* optimization, since it involves registering three or more views. 

The goal in most bundle adjustment methods is to minimize the **reconstruction error**. This means, looking to bring the approximate parameters of the views,  for example, camera or image transforms, to values such that the  re-projected 2D points back on the original views will align with  minimal error. This can be expressed mathematically like so:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/0fa28b19-fdb3-4997-b4ec-8f5403fa2852.png)

Where we look for the best camera or image transforms *T*, such that the distance between original point *Xi* and reprojected point *Proj(Tj, Xi)* is minimal. The binary variable *vij* marks whether point *i* can be seen in image *j*, and can contribute to the error. These kinds of optimization problems can be solved with **iterative non-linear least squares** solvers, such as **Levenberg-Marquardt**, since the previous *Proj* function is usually non-linear.



# Warping images for panorama creation

Given that we know the homographies between images, we can apply  their inverse to project all the images on the same plane. However, a  direct warping using the homography ends up with a stretched-out look  if, for example, all the images are projected on the plane of the first  image. In the following image, we can see a stitching of 4 images using *concatenated* homography (perspective) warping, meaning all the images are registered to the plane of the first image, which illustrates the ungainly  stretching:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/2b8a4bb2-30d5-402c-8eab-3d76541bfeb3.png)

To cope with this problem, we think of the panorama as looking at the images from inside a cylinder, where the images are projected on the  wall of the cylinder, and we rotate the camera at the center. To achieve this effect, we first need to warp the images to **cylindrical coordinates**, as if the round wall of the cylinder was undone and flattened to a  rectangle. The following diagram explains the process of cylindrical  warping:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/4c458080-4517-47c0-a905-04ee9d3872c0.png)

To wrap the image in cylindrical coordinates, we first apply the  inverse of the intrinsic matrix to get the pixel in normalized  coordinates. We now assume the pixel is a point on the surface of the  cylinder, which is parameterized by the height *h* and the angle *θ*. Height *h* essentially corresponds to the *y* coordinate, while the *x* and *z* (which are perpendicular to one another with regards to *y*) exist on a unit circle and therefore correspond to sin*θ* and cos*θ,* respectively. To get the warped image in the same pixel size as the original image, we can apply the intrinsic matrix *K* again; however, we can change the focal length parameter *f*, for example, to affect the output resolution of our panorama.

In the cylindrical warping model, the relationship between the images becomes purely translational, and in fact governed by a single  parameter: *θ**.* To stitch the images in the same plane, we simply need to find the *θ*s, just a single degree of freedom, which is simple compared to finding  eight parameters for the homography between every two consecutive  images. One major drawback of the cylindrical method is that we assume  the camera's rotational axis motion is perfectly aligned with its up  axis, as well as static in its place, which is almost never the case  with handheld cameras. Still, cylindrical panoramas produce highly  pleasing results. Another option for warping is **spherical coordinates**, which allow for more options in stitching the images in both *x* and *y* axes.

# Project overview

This project will include two major parts as follows:

- iOS application to support capturing the panorama
- OpenCV Objective-C++ code for creating the panorama from the images and integrating into the application

The iOS code will mostly be concerned with building the UI,  accessing the camera, and capturing images. Then, we will focus on  getting the images to OpenCV data structures and running the image  stitching functions from the stitch module.



# Setting up an iOS OpenCV project with CocoaPods 

To start using OpenCV in iOS, we must import the library compiled for iOS devices. This is easily done with CocoaPods, which is a vast  repository of external packages for iOS and macOS with a convenient  command-line package manager utility called pod.

We begin by creating an empty Xcode project for iOS, with the "*Single View App*" template. Make sure to select a Swift project, and not an Objective-C  one. The Objective-C++ code we will see will be added later.

After the project in initialized in a certain directory, we execute the pod init command in the terminal within that directory. This will create a new file called Podfile in the directory. We need to edit the file to look like the following:

```
# Uncomment the next line to define a global platform for your project
# platform :ios, '9.0'

target 'OpenCV Stitcher' do
  use_frameworks!
  # Pods for OpenCV Stitcher
  pod 'OpenCV2', '4.0.0.beta'
end
```

Essentially, just adding pod 'OpenCV2', '4.0.0' to the target tells CocoaPods to download and unpack the OpenCV framework in our project. Afterwards, we run pod install in the Terminal in the same directory, which will set up our project  and Workspace to include all the Pods (just OpenCV v4 in our case). To  start working on the project, we open the $(PROJECT_NAME).xcworkspace file, rather than the .xcodeproject file as usual with Xcode projects.



# iOS UI for panorama capture

Before we delve into the OpenCV code for turning an image collection  into a panorama, we will first build a UI to support the easy capture of a sequence of overlapping images. First, we must make sure we have  access to the camera as well as saved images. Open the Info.plist file and add the following three rows:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/c4da3d8f-54ac-43a6-b871-9b745cc119aa.png)

To start building the UI, we create a view with a View object for the camera preview on the right, and an overlapping ImageView on the left. ImageView should cover some area of the camera preview View, to help guide the user in  capturing an image with enough overlap from the last. We can also add a  few ImageView instances on top to show the previously captured images, and on the bottom a Capture button and a Stitch button to control the application flow:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/2469e03f-6ea8-474d-8d52-7cba6b670954.png)

To connect the camera preview to the preview View, we must do the following:

1. Start a capture session (AVCaptureSession)
2. Select a device (AVCaptureDevice)
3. Set up the capture session with input from the device (AVCaptureDeviceInput)
4. Add an output for capturing photos (AVCapturePhotoOutput)

Most of these can be set up immediately when they are  initialized as members of the ViewController class. The following code  shows setting up the capture session, device, and output on the fly:

```
class ViewController: UIViewController, AVCapturePhotoCaptureDelegate {

    private lazy var captureSession: AVCaptureSession = {
        let s = AVCaptureSession()
        s.sessionPreset = .photo
        return s
    }()
    private let backCamera: AVCaptureDevice? = AVCaptureDevice.default(.builtInWideAngleCamera, for: .video, position: .back)

    private lazy var photoOutput: AVCapturePhotoOutput = {
        let o = AVCapturePhotoOutput()
        o.setPreparedPhotoSettingsArray([AVCapturePhotoSettings(format: [AVVideoCodecKey: AVVideoCodecType.jpeg])], completionHandler: nil)
        return o
    }()
    var capturePreviewLayer: AVCaptureVideoPreviewLayer?
```

The rest of the initialization can be done from the viewDidLoad function, for example, adding the capture input to the session and  creating a preview layer for showing the camera feed onscreen. The  following code shows the rest of the initialization process, adding the  input and output to the capture session, and setting up the preview  layer.

```
    override func viewDidLoad() {
        super.viewDidLoad()

        let captureDeviceInput = try AVCaptureDeviceInput(device: backCamera!)
        captureSession.addInput(captureDeviceInput)
        captureSession.addOutput(photoOutput)

        capturePreviewLayer = AVCaptureVideoPreviewLayer(session: captureSession)
        capturePreviewLayer?.videoGravity = AVLayerVideoGravity.resizeAspect
        capturePreviewLayer?.connection?.videoOrientation = AVCaptureVideoOrientation.portrait
        
        // add the preview layer to the view we designated for preview
        let previewViewLayer = self.view.viewWithTag(1)!.layer
        capturePreviewLayer?.frame = previewViewLayer.bounds
        previewViewLayer.insertSublayer(capturePreviewLayer!, at: 0)
        previewViewLayer.masksToBounds = true
        captureSession.startRunning()
    }
```

With the preview set up, all that is left is to handle the photo  capture on a click. The following code shows how a button click (TouchUpInside) will trigger the photoOutput function via delegate, and then simply add the new image to a list as well as save it to memory in the photo gallery.

```
@IBAction func captureButton_TouchUpInside(_ sender: UIButton) {
    photoOutput.capturePhoto(with: AVCapturePhotoSettings(), delegate: self)
}

var capturedImages = [UIImage]()

func photoOutput(_ output: AVCapturePhotoOutput, didFinishProcessingPhoto photo: AVCapturePhoto, error: Error?) {
    let cgImage = photo.cgImageRepresentation()!.takeRetainedValue()
    let image = UIImage(cgImage: cgImage)
    prevImageView.image = image // save the last photo, for the overlapping ImageView
    capturedImages += [image] // add to array of captured photos
        
    // save to photo gallery on phone as well
    PHPhotoLibrary.shared().performChanges({
            PHAssetChangeRequest.creationRequestForAsset(from: image)
    }, completionHandler: nil)
}
```

This will allow us to capture multiple images in succession while  helping the user align one image with the next. Here is an example of  the UI running on the phone:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/dd8ecc55-542f-4a3a-959a-42db03335919.png)

Next, we will see how to take the images to an Objective-C++ context, where we can work with the OpenCV C++ API for panorama stitching.

# OpenCV stitching in an Objective-C++ wrapper

For working in iOS, OpenCV provides its usual C++ interface that can  be invoked from Objective-C++. In recent years, however, Apple has  encouraged iOS application developers to use the more versatile Swift  language for building applications and forgo Objective-C. Luckily, a  bridge between Swift and Objective-C (and Objective-C++) can be easily  created, allowing us to invoke Objective-C functions from Swift. Xcode  automates much of the process, and creates the necessary glue code.

To start, we create a new file (Command-N) in Xcode and select Cocoa Touch Class, as shown in the following screenshot:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/8c89ebc3-3218-4110-bebd-a03aef10f76f.png)

Choose a meaningful name for the file (for example, StitchingWrapper) and make sure to select Objective-C as the language, as shown in the following screenshot:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/86342b17-4a57-4e61-8594-c2f5694f7ea9.png)

Next, as shown in the following screenshot, confirm that Xcode should create a **bridging header** for your Objective-C code:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/a4b62a4d-d1ba-474a-b30f-7a1aefffb20e.png)

This process will result in three files: StitchingWrapper.h, StitchingWrapper.m, and OpenCV Stitcher-Bridging-Header.h. We should manually rename StitchingWrapper.m to StitchingWrapper.mm, to enable Objective-**C++** over plain Objective-C. At this point, we are prepared to start using OpenCV in our Objective-C++ code.

In StitchingWrapper.h, we will define a new function that will accept an NSMutableArray* as the list of images captured by our earlier UI Swift code:

```
@interface StitchingWrapper : NSObject

+ (UIImage* _Nullable)stitch:(NSMutableArray*) images;

@end
```

And, in the Swift code for our ViewController, we can implement a function to handle a click on the Stitch button, where we create the NSMutableArray from the capturedImages Swift array of UIImages:

```
@IBAction func stitch_TouchUpInside(_ sender: Any) {
    let image = StitchingWrapper.stitch(NSMutableArray(array: capturedImages, copyItems: true))
    if image != nil {
        PHPhotoLibrary.shared().performChanges({ // save stitching result to gallery
                PHAssetChangeRequest.creationRequestForAsset(from: image!)
        }, completionHandler: nil)
    }
}
```

Back on the Objective-C++ side, firstly we need to get the OpenCV cv::Mat objects from the UIImage*s input, like so:

```
+ (UIImage* _Nullable)stitch:(NSMutableArray*) images {
    using namespace cv;
        
    std::vector<Mat> imgs;
    
    for (UIImage* img in images) {
        Mat mat;
        UIImageToMat(img, mat);
        if ([img imageOrientation] == UIImageOrientationRight) {
            rotate(mat, mat, cv::ROTATE_90_CLOCKWISE);
        }
        cvtColor(mat, mat, cv::COLOR_BGRA2BGR);
        imgs.push_back(mat);
    }
```

Finally, we're ready to call the stitching function on the array of images, like so:

```
    Mat pano;
    Stitcher::Mode mode = Stitcher::PANORAMA;
    Ptr<Stitcher> stitcher = Stitcher::create(mode, false);
    try {
        Stitcher::Status status = stitcher->stitch(imgs, pano);
        if (status != Stitcher::OK) {
            NSLog(@"Can't stitch images, error code = %d", status);
            return NULL;
        }
    } catch (const cv::Exception& e) {
        NSLog(@"Error %s", e.what());
        return NULL;
    }
```

An example of an output panorama created with this code (note the use of cylindrical warping) is shown as follows:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/571d4d1e-0e79-4148-9b17-f65bd58924d4.png)

You may notice some changes in illumination between the four images,  while the edges have been blended. Dealing with varying illumination can be addressed in the OpenCV image stitching API using the cv::detail::ExposureCompensator base API. 

# Summary

In this chapter, we've learned about panorama creation. We've seen  some of the underlying theory and practice in panorama creation,  implemented in OpenCV's stitching module. We then turned our  focus to creating an iOS application that helps a user to capture images for panorama stitching with overlapping views. Lastly, we saw how to  invoke OpenCV code from a Swift application to run the stitching functions on the captures images, resulting in a finished panorama.

The next chapter will focus on selection strategies for OpenCV  algorithms given a problem at hand. We will see how to reason about a  computer vision problem and its solution offering in OpenCV, as well as  how to compare competing algorithms in order to make informed  selections.

# Chapter 9 Finding the Best OpenCV Algorithm for the Job

Any computer vision problem can be solved in different ways. Each way has its pros and cons and relative measures of success, depending on  the data, resources, or goals. Working with OpenCV, a computer vision  engineer has many algorithmic options on hand to solve a given task.  Making the right choice in an informed way is extremely important since  it can have a tremendous impact on the success of the entire solution,  and prevent you from being boxed into a rigid implementation. This  chapter will discuss some methods to follow when considering options in  OpenCV. We will discuss the areas in computer vision that OpenCV covers, ways to select between competing algorithms if more than one exists,  how to measure the success of an algorithm, and finally how to measure  success in a robust way with a pipeline.

The following topics will be covered in this chapter:

- Is it covered in OpenCV? Computer vision topics with algorithms available in OpenCV.
- Which algorithm to pick? Topics with multiple available solutions in OpenCV.
- How to know which algorithm is best? Establishing metrics for measuring algorithm success.
- Using a pipeline to test different algorithms on the same data.

## Is it covered in OpenCV?

When first tackling a computer vision problem, any engineer should first asks: should I implement a solution from scratch, from a paper or known method, or use an existing solution and fit it to my needs?

This question goes hand-in-hand with the offering of implementations in OpenCV. Luckily, OpenCV has very wide and extensive coverage of both canonical and specific computer vision tasks. On the other hand, not all OpenCV implementations are easily applied to a given problem. For example, while OpenCV offers some object recognition and classification capabilities, it is by far inferior to the state-of-the-art computer vision one would see in conferences and the literature. Over the last few years, and certainly in OpenCV v4.0, there's an effort to easily integrate deep convolutional neural networks with OpenCV APIs (through the core dnn module) so engineers can enjoy all the latest and greatest work.

We made an effort to list the current offering of algorithms in OpenCV v4.0, along with a subjective estimation of the coverage they give of the grand computer vision subject. We also note whether OpenCV provides GPU implementation coverage, and whether the topic is covered in the core modules or in the contrib modules. Contrib modules vary; some modules are very mature and offer documentation and tutorials (for example, tracking), while others are a black box implementation with very poor documentation (for example, xobjectdetect). Having core module implementation is a good sign there is going to be adequate documentation, examples, and robustness.

The following is a list of topics in computer vision with their level of offering in OpenCV:

| **Topic**                                 | **Coverage** | **OpenCV offering**                                          | **Core?**      | **GPU?** |
| ----------------------------------------- | ------------ | ------------------------------------------------------------ | -------------- | -------- |
| Image processing                          | Very good    | Linear and non-linear filtering, transformations, colorspaces, histograms, shape analysis, edge detection | Yes            | Good     |
| Feature detection                         | Very good    | Corner detection, key-point extraction, descriptor calculation | Yes + contrib  | Poor     |
| Segmentation                              | Mediocre     | Watershed, contour and connected component analysis, binarization and thresholding, GrabCut, foreground-background segmentation, superpixels | Yes + contrib  | Poor     |
| Image alignment, stitching, Stabilization | Good         | Panoramic stitching pipeline, Video stabilization pipeline, template matching, transform estimation, warping, seamless stitching | Yes + contrib  | Poor     |
| Structure from motion                     | Poor         | Camera pose estimation, essential and fundamental matrix estimation, integration with external SfM library | Yes + contrib  | None     |
| Motion estimation, optical flow, tracking | Good         | Optical flow algorithms, Kalman filter, object tracking framework, multi-target tracking | Mostly contrib | Poor     |
| Stereo and 3D reconstruction              | Good         | Stereo matching framework, triangulation, structured light scanning | Yes + contrib  | Good     |
| Camera calibration                        | Very good    | Calibration from several patterns, stereo rig calibration    | Yes + contrib  | None     |
| Object detection                          | Mediocre     | Cascade classifiers, QR code detector, face landmark detector, 3D object recognition, text detection | Yes + contrib  | Poor     |
| Object recognition, classification        | Poor         | Eigen and Fisher face recognition, bag-of-words              | Mostly contrib | None     |
| Computational photography                 | Mediocre     | Denoising, HDR, superresolution                              | Yes + contrib  | None     |

While OpenCV does a tremendous job with traditional computer vision algorithms, such as image processing, camera calibration, feature extraction, and other topics, it also has poor coverage of important topics such as SfM and object classification. In other topics, such as segmentation, it has a decent offering, but again falls short of state of the art, although that has moved almost exclusively to convolutional networks and can essentially be implemented with the dnn module.

In some topics, such as feature detection, extraction, and matching, as well as camera calibration, OpenCV is considered to be the most comprehensive, free, and usable library today, used in probably many thousands of applications. However, in the course of a computer vision project, engineers may consider decoupling from OpenCV after the prototyping phase since the library is heavy and adds significantly to the overhead in building and deploying (an acute problem for mobile applications). In those cases, OpenCV is a good crutch for prototyping, because of its wide offering, usefulness for testing, and choosing between different algorithms for the same task, for example, for calculating a 2D feature. Beyond prototyping, numerous other considerations become more important, such as the execution environment, stability and maintainability of the code, permissions and licensing, and more. At that stage, using OpenCV should satisfy the requirements of the product, including the considerations mentioned.

## Algorithm options in OpenCV

OpenCV has many algorithms covering the same subject. When implementing a new processing pipeline, sometimes there is more than one choice for a step in the pipeline. For example, in [Chapter 2](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/3229555f-ce7e-4330-9197-9cc4169f230f.xhtml), *Explore Structure from Motion with the SfM Module*, we made an arbitrary decision to use AKAZE features for finding landmarks between the images to estimate camera motion, and sparse 3D structure, however; there are many more kinds of 2D features available in OpenCV's features2D module. A more sensible mode of operation should have been to select the type of feature algorithm to use based on its performance, with respect to our needs. At the very least, we need to be aware of the different options.

Again, we looked to create a convenient way to see whether there are multiple options for the same task. We created a table where we list specific computer vision tasks that have multiple algorithm implementations in OpenCV. We also strived to mark whether algorithms have a common abstract API, and thus, easily and completely interchangeable within the code. While OpenCV offers the cv::Algorithm base class abstraction for most if not all of its algorithms, the abstraction is at a very high level and gives very little power to polymorphism and interchangeability. From our review, we exclude the machine learning algorithms (the ml module and the cv::StatsModel common API) since they are not proper computer vision algorithms, as well as low-level image processing algorithms, which do in fact have overlapping implementations (for example, the Hough detector family). We also exclude the GPU CUDA implementations that shadow several core topics such as object detection, background segmentation, 2D features, and more, since they are mostly replicas of the CPU implementations.

The following are topics with multiple implementations in OpenCV:

| **Topic**              | **Implementations**                                          | **Base API?** |
| ---------------------- | ------------------------------------------------------------ | ------------- |
| Optical flow           | video module: SparsePyrLKOpticalFlow, FarnebackOpticalFlow, DISOpticalFlow, VariationalRefinementoptflow contrib module: DualTVL1OpticalFlow, OpticalFlowPCAFlow | Yes           |
| Object tracking        | track contrib module: TrackerBoosting, TrackerCSRT, TrackerGOTURN, TrackerKCF, TrackerMedianFlow, TrackerMIL, TrackerMOSSE, TrackerTLDExternal: DetectionBasedTracker | Yes1          |
| Object detection       | objdetect module: CascadeClassifier, HOGDescriptor, QRCodeDetector,linemod contrib module: Detectoraruco contrib module: aruco::detectMarkers | No2           |
| 2D features            | OpenCV's most established common API.features2D module: AgastFeatureDetector, AKAZE, BRISK, FastFeatureDetector, GFTTDetector,KAZE, MSER, ORB,SimpleBlobDetectorxfeatures2D contrib module: BoostDesc, BriefDescriptorExtractor,DAISY, FREAK, HarrisLaplaceFeatureDetector, LATCH, LUCID, MSDDetector, SIFT, StarDetector, SURF, VGG | Yes           |
| Feature matching       | BFMatcher, FlannBasedMatcher                                 | Yes           |
| Background subtraction | video module: BackgroundSubtractorKNN, BackgroundSubtractorMOG2bgsegm contrib module: BackgroundSubtractorCNT, BackgroundSubtractorGMG, BackgroundSubtractorGSOC, BackgroundSubtractorLSBP, BackgroundSubtractorMOG | Yes           |
| Camera calibration     | calib3d module: calibrateCamera,calibrateCameraRO, stereoCalibratearuco contrib module: calibrateCameraArcuo, calibrateCameraCharucoccalib contrib module: omnidir::calibrate, omnidir::stereoCalibrate | No            |
| Stereo reconstruction  | calib3d module: StereoBM, StereoSGBMstereo contrib module: StereoBinaryBM, StereoBinarySGBMccalib contrib module: omnidir::stereoReconstruct | Partial3      |
| Pose estimation        | solveP3P, solvePnP, solvePnPRansac                           | No            |

1 Only for the classes in the track contrib module.
2 Some classes share functions with the same name, but no inherited abstract class.
3 Each module has a base within itself, but not shared across modules.

When approaching a problem with a few algorithmic options, it's important not to commit too early to one execution path. We may use the preceding table above to see options exist, and then explore them. Next, we will discuss how to select from a pool of options.

## Which algorithm is best?

Computer vision is a world of knowledge and a decades-long research pursuit. Unlike many other disciplines, computer vision is not strongly hierarchical or vertical, which means new solutions for given problems are not always better and may not be based on preceding work. Being an applied field, computer vision algorithms are created with attention to the following aspects, which may explain the non-vertical development:

- **Computation resources**: CPU, GPU, embedded system, memory footprint, network connectivity.
- **Data**: Size of images, number of images, number of image stream (cameras), data type, sequentiality, lighting conditions, types of scenes, and so on.
- **Performance requirements**: Real-time output or another timing constraint (for example, human perception), accuracy and precision.
- **Meta-algorithmic**: Algorithm simplicity (cross-reference Occam's Razor theorem), implementation system and external tools, availability of formal proof.

With every algorithm created in order to cater for perhaps a certain one of these considerations, one can never know for sure that it will outperform all others without properly testing some or all of them. Granted, testing all algorithms for a given problem is *unrealistic*, even if the implementations are indeed available, and OpenCV certainly has many implementations available, as we've seen in the last section. On the other hand, computer vision engineers will be remiss if they do not consider the possibility that their implementations are not optimal because of their algorithm choices. This in essence flows from the *no free lunch* theorem, which states, in broad strokes, that no single algorithm is the best one over the entire space of possible datasets.

It is, therefore, a very welcome practice to test a set of different algorithmic options before committing to the best one out of that set. But how do we find the *best one*? The word *best* implies that each one will be *better* (or *worse*) than the others, which in turn suggests there is an objective scale or measurement in which they are all scored and ranked in order. Obviously, there is not a single measure (**metric**) for all algorithms in all problems each problem will have its own. In many cases, the metric for success will form a measurement of **error**, deviation from a known **ground truth** value that was sourced from humans or other algorithms we can trust. In optimization, this is known as a **loss function**or cost function, a function that we wish to minimize (sometimes maximize) in order to find the best option that has the lowest score. Another prominent class of metrics cares less about the output performance (for example, error) and more about runtime timing, memory footprint, capacity and throughput, and so on.

The following is a partial list of metrics we may see in select computer vision problems:

| **Task**                                       | **Example metrics**                                          |
| ---------------------------------------------- | ------------------------------------------------------------ |
| Reconstruction, registration, feature matching | **Mean absolute error** (**MAE**), **mean squared error** (**MSE**), **root mean squared error** (**RMSE**),  **sum of squared distances** (**SSD**) |
| Object classification, recognition             | Accuracy, precision, recall, f1-score, **false-positive rate** (**FPR**) |
| Segmentation, object detection                 | **Intersection-over-Union** (**IoU**)                        |
| Feature detection                              | Repeatability, precision recall                              |

The why to find the best algorithm for a given task is either to set all the options at our disposal in a test scenario and measure their performance on the metrics of choice, or obtain someone else's measurements on a standard experiment or dataset. The highest ranking option should be picked, where the ranking is derived from a combination of the metrics (in the case of just a single metric, it's an easy task). Next, we will try our hand at such a task, and make an *informed* choice on the best algorithm.

## Example comparative performance test of algorithms

As an example, we will set up a scenario where we are required to align overlapping images, like what is done in panorama or aerial photo stitching. One important feature that we need to measure performance is to have a **ground truth**, a precise measurement of the true condition that we are trying to recover with our approximation method. Ground truth data can be obtained from datasets made available for researchers to test and compare their algorithms; indeed, many of these datasets exist and computer vision researchers use them all the time. One good resource for finding computer vision datasets is **Yet Another Computer Vision Index To Datasets**(**YACVID**), https://riemenschneider.hayko.at/vision/dataset/, which has been actively maintained for the past eight years and contains hundreds of links to datasets. The following is also a good resource for data: https://github.com/jbhuang0604/awesome-computer-vision#datasets.

We, however, will pick a different way to get ground truth, which is well practiced in computer vision literature. We will create a contrived situation within our parametric control, and create a benchmark that we can vary to test different aspects of our algorithms. For our example, we will take a single image and split it into two overlapping images, and apply some transformations to one of them. The fusing of the images with our algorithm will try to recreate the original fused image, but it will likely not do a perfect job. Choices we make in selecting the pieces in our system (for example, the type of 2D feature, the feature matching algorithm, and the transform recovery algorithm) will affect the final result, which we will measure and compare. Working with artificial ground truth data gives us a lot of control over the conditions and level in our trials.

Consider the following image and its two-way overlapping split:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/fa28e66c-a85b-41a1-963b-a2238d3ec1e2.png)

Image: https://pixabay.com/en/forest-forests-tucholski-poland-1973952/

The left image we keep untouched, while we perform artificial transformations on the right image to see how well our algorithm will be able to undo them. To keep things simple, we will only rotate the right image at several brackets, like so:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/78450888-4d25-4f15-89db-d66197a82d40.png)

We add a middle bracket for the *no rotation* case, in which the right image is only translated somewhat. This makes up our ground truth data, where we know exactly what transformation occurred and what the original input was.

Our goal is to measure the success of different 2D feature descriptor types in aligning images. One measure for our success can be the **Mean Squared Error (MSE)** over the pixels of the final re-stitched image. If the transformation recovery wasn't very well done, the pixels will not align perfectly, and thus we expect to see a high MSE. As the MSE approaches zero, we know the stitching was done well. We may also wish to know, for practical reasons, which feature is the most efficient, so we can also take a measurement of execution time. To this end, our algorithm can be very simple:

1. Split original image *left image* and *right image*.

2. For each of the feature types (

   SURF, SIFT, ORB, AKAZE, BRISK),

    

   do the following: 

   1. Find keypoints and features in the left image.

   2. For each rotation angle 

      [-90, -67, ..., 67, 90] 

      do the following:

      1. Rotate the right image by the rotation angle.
      2. Find keypoints and features in the rotated right image.
      3. Match keypoints between the rotated right image and the left image.
      4. Estimate a rigid 2D transform.
      5. Transform according to the estimation.
      6. Measure the **MSE** of the final result with the original unsplit image.
      7. Measure the overall **time** it takes to extract, compute, and match features, and perform the alignment.

As a quick optimization, we can cache the rotated images, and not calculate them for each feature type. The rest of the algorithm remains untouched. Additionally, to keep things fair in terms of timing, we should take care to have a similar number of keypoints extracted for each feature type (for example, 2,500 keypoints), which can be done by setting the threshold for the keypoint extraction functions.

Note the alignment execution pipeline is oblivious of the feature type, and works exactly the same given the matched keypoints. This is a very important feature for testing many options. With OpenCV's cv::Feature2D and cv::DescriptorMatcher common base API, it is possible to achieve this, since all features and matchers implement them. However, if we take a look at the table in the *Is it covered in OpenCV?* section, we can see that this may not be possible for all vision problem in OpenCV, so we may need to add our own instrumentation code to make this comparison possible.

In the accompanying code, we can find the Python implementation of this routine, which provides the following results. To test rotation invariance, we vary the angle and measure the reconstruction MSE:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/b0ceab17-8c4b-468a-8d16-4d38951d8b1b.png)

With the same experiments, we record the mean MSE across all experiments for a feature type, and also the mean execution time, shown as follows:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/d929290b-8db2-4ed9-9bd9-0d3cf46a1e6a.png)

**Results analysis**, we can clearly see some features performing better than others in terms of MSE, with respect to both the different rotation angles and overall, and we can also see a big variance in the timing. It seems AKAZE and SURF are the highest performers in terms of alignment success across the rotation angle domain, with an advantage for AKAZE in higher rotations (~60°). However, at very small angular variation (rotation angle close to 0°), SIFT achieves practically perfect reconstruction with MSE around zero, and it also does as well as if not better then, the others with rotations below 30°. ORB does very badly throughout the domain, and BRISK, while not as bad, rarely was able to beat any of the forerunners.

Considering timing, ORB and BRISK (which are essentially the same algorithm) are the clear winners, but they both are far behind the others in terms of reconstruction accuracy. AKAZE and SURF are the leaders with neck-and-neck timing performance.

Now, it is up to us as the application developers to rank the features according to the requirements of the project. With the data from this test we performed, it should be easy to make a decision. If we are looking for speed, we would choose BRISK, since it's the fastest and performs better than ORB. If we are looking for accuracy, we would choose AKAZE, since it's the best performer and is faster than SURF. Using SURF in itself is a problem, since the algorithm is not free and it is protected by patent, so we are lucky to find AKAZE as a free and adequate alternative.

This was a very rudimentary test, only looking at two simple measures (MSE and time) and only one varied parameter (rotation). In a real situation, we may wish to insert more complexity into the transformations, according to the requirements of our system. For example, we may use full perspective transformation, rather than just rigid rotation. Additionally, we may want to do a deeper statistical analysis of the results. In this test, we only ran the alignment process once for each rotation condition, which is not good for capturing a good measure of timing, since some of the algorithms may benefit from executing in succession (for example, loading static data to memory). If we have multiple executions, we can reason about the variance in the executions, and calculate the standard deviation or error to give our decision making process more information. Lastly, given enough data, we can perform statistical inference processes and hypothesis testing, such as a **t-test** or **analysis of variance** (**ANOVA**), to determine whether the minute differences between the conditions (for example, AKAZE and SURF) have **statistical significance** or are too noisy to tell apart.

## Summary

Choosing the best computer vision algorithm for the job is an  illusive process, which is the reason many engineers do not perform it.  While published survey work on different choices provides benchmark  performance, in many situations it doesn't model the particular system  requirements an engineer might encounter, and new tests must be  implemented. The major problem in testing algorithmic options is  instrumentation code, which is an added work for engineers, and not  always simple. OpenCV provides base APIs for algorithms in several  vision problem domains, but the cover age is not complete. On the other  hand, OpenCV has very extensive coverage of problems in computer vision, and is one of the premier frameworks to perform such tests.

Making an informed decision when picking an algorithm is a very  important aspect of vision engineering, with many elements to optimize  for, for example, speed, accuracy, simplicity, memory footprint, and  even availability. Each vision system project has particular  requirements that affect the weight each of these elements receives, and thus the final decision. With relatively simple OpenCV code, we saw how to gather data, chart it, and make an informed decision about a toy  problem.

In the next chapter we discuss the history of the OpenCV open source  project, as well as some common pitfalls when using OpenCV with  suggested solutions for them.

# Chapter 10 Avoiding Common Pitfalls in OpenCV

OpenCV has been around for more than 15 years now. It contains many  implementations that are outdated or unoptimized and are relics of the  past. An advanced OpenCV engineer should know how to avoid basic  mistakes in navigating the OpenCV APIs, and see their project to  algorithmic success.

In this chapter, we will review the historic development of OpenCV,  and the gradual increase in the framework and algorithmic offering,  alongside the development of computer vision at large. We will use this  knowledge to see how to figure out whether a newer alternative exists  within OpenCV for our algorithm of choice. Lastly, we will discuss how  to identify and avoid common problems or sub-optimal choices while  creating computer vision systems with OpenCV.

The following topics will be covered in this chapter:

- A historic review of OpenCV and the latest wave of computer vision research
- Checking the date at which an algorithm became available in OpenCV, and whether it's a sign that it is outdated
- Addressing pitfalls in building computer vision systems in OpenCV

## History of OpenCV from v1 to v4

OpenCV started as the brainchild of **Gray Bradsky**, once a computer vision engineer at **Intel**, around the early 2000s. Bradsky and a team of engineers, mostly from  Russia, developed the first versions of OpenCV internally at Intel  before making v0.9 of it **open source software** (**OSS**) in 2002. Bradsky then transitioned to **Willow Garage**, with the former founding members of OpenCV. Among them were Viktor  Eurkhimov, Sergey Molinov, Alexander Shishkov, and Vadim Pisarevsky (who eventually started the company **ItSeez**, which was acquired in 2016 by Intel), who began supporting the young library as an open source project.

Version 0.9 had a predominantly C API and already sported image data  manipulation functions and pixel access, image processing, filtering,  colorspace transformations, geometric and shape analysis (for example,  morphologic functions, Hough transforms, contour finding), motion  analysis, basic machine learning (K-means, HMM), camera pose estimation, basic linear algebra (SVD, Eigen decomposition), and more. Many of  these functions lasted through the ages, even into today's versions of  OpenCV. Version 1.0 was released in 2006, and it marked the beginning of the library as the dominant force in OSS computer vision. In late 2008, Bradsky and **Adrian Kaehler** published the best-selling *Learning OpenCV* book based on OpenCV v1.1pre1, which was a smashing worldwide success, and  served for years to come as the definitive guide to OpenCV's C API.

For its completeness, OpenCV v1 became a very popular framework for vision work in both academic and industrial  applications, especially in the robotics domain, although it was just a  small departure from v0.9 in terms of feature offering. After the  release of v1.0 (late 2006), the OpenCV project went into years of  hibernation, as the founding team was occupied with other projects and  the open source community wasn't as established as it became years  later. The project released v1.1pre1 in late 2008 with minor additions; however, the foundation of OpenCV as the most well-known vision library came with version 2.x, which introduced the very successful **C++ API**. Versions 2.x lasted *6 years* (2009-2015) as the stable branch of OpenCV, and the branch was  maintained even until very recently, in early 2018 (the last version,  2.4.13.6, was released in February 2018), almost *10 years*  later. Version 2.4, released in mid-2012, had a very stable and  successful API, lasted for three years, and also introduced a very wide  offering of features.



Versions 2.x introduced the **CMake** build system, which was also used at the time by the **MySQL** project, to align with its goal to be completely **cross-platform**. Apart from the new C++ API, v2.x also brought the concept of **modules** (in v2.2, circa 2011), which can be built, included, and linked separately, based on the project assembly necessity, forsaking v1.x's cv, cvaux, ml, and so on. The 2D feature  suit was extended, as well as the machine learning capabilities,  built-in face recognition cascade models, 3D reconstruction  functionality, and most importantly the coverage of **Python** bindings. Early investment in Python made OpenCV the best tool for  vision prototyping available at that time, and probably still today.  Version 2.4, released in mid-2012, continued development until 2018,  with v2.5 never released due to fears of breaking API changes and simply rebranded as v3.0 (ca. mid-2013). Version 2.4.x continued to bring more important features such as **Android** and **iOS** support, **CUDA** and **OpenCL** implementations, CPU optimizations (for example, SSE and other SIMD architectures), and an incredible amount of new algorithms.

Version 3.0, first released out of beta in late 2015, was first received with lukewarm adoption from the  community. They were looking for a stable API, since some APIs had  breaking changes and a drop-in replacement was impossible. The header  structure also changed (from opencv2/<module>/<module>.hpp to opencv2/<module>.hpp), making the transition even harder. Version 2.4.11+ (February 2015) had  instrumentation to bridge the API gap between the versions, and  documentation was installed to help developers transition to v3.0 (https://docs.opencv.org/3.4/db/dfa/tutorial_transition_guide.html). Version 2.x maintained a very strong hold, with many package management systems (for example, Ubuntu's apt) still serving it as the stable version of OpenCV, while version 3.x was advancing at a very fast pace.

After years of cohabitation and planning, version 2.4.x gave  way to version 3.x, which boasted a revamped API (many abstractions and  base classes introduced) and improved GPU support via the new **Transparent API (T-API)**, which allowed the use of GPU code interchangeably with regular CPU code. A separate repository for community-contributed code was established, opencv-contrib, removing it from the main code as a module in v2.4.x, with improved  build stability and timing. Another big change was the machine learning  support in OpenCV, which was greatly improved and revised from v2.4.  Version 3.x was also pushed for better Android support and optimizations for CPU architectures beyond Intel x86 (for example, ARM, NEON) via the OpenCV **HAL** (**Hardware Acceleration Layer**), which later merged into the core modules. The first emergence of deep  neural networks in OpenCV was recorded in v3.1 (December 2015) as a contrib module, and almost two years later in v3.3 (August 2017) was upgraded to a core module, opencv-dnn. The 3.x versions brought tremendous improvements to optimization and compatibility with GPU and CPU architectures, with support from Intel, Nvidia, AMD, and Google, and became OpenCV's hallmark as the optimized computer vision library.

Version 4.0 marks the mature state of OpenCV as the major open source project it is today. The old C API (of which many functions date back  to v0.9) was let go and instead **C++11** was made *mandatory*, which also rid the library of its cv::String and cv::Ptr hybrids. Version 4.0 keeps track of further optimization for CPUs and GPUs; however, the most interesting addition is the **Graph API (G-API) module**. G-API brings the spirit of the times to OpenCV, with support for  building compute graphs for computer vision, with heterogeneous  execution on CPU and GPU, following the very big success of Google's **TensorFlow** deep learning library and Facebook's **PyTorch**. With long-standing investment in deep learning and machine learning, Python and other languages, execution graphs,  cross-compatibility, and a wide offering of optimized algorithms, OpenCV is established as a forward-looking project with very strong community  support, which makes it fifteen years later the leading open computer  vision library in existence.

The history of this book series, *Mastering OpenCV*, is intertwined with the development history of OpenCV as the major  library for open source computer vision. The first edition, released in  2012, was based on the everlasting v2.4.x branch. This dominated the  OpenCV scene in 2009-2016. The second edition, released in 2017, hailed  the dominance of OpenCV v3.1+ in the community (started in mid-2016).  The third edition, the one you are reading now, welcomes OpenCV v4.0.0  into the fold, released in late October 2018.

## OpenCV and the data revolution in computer vision

OpenCV existed before the data revolution in computer vision. In the late 1990s, access to big amounts of data was not a  simple task for computer vision researchers. Fast access to the internet was not common, and even universities and big research institutes were  not strongly networked. The limited storage capacities of personal and  bigger institutional computers did not allow researchers and students to work with big amounts of data, let alone having the computational power (memory and CPU) required to do so. Thus, the research on large-scale  computer vision problems was restricted to a selected list of  laboratories worldwide, among them MIT's **Computer Science and Artificial Intelligence Lab** (**CSAIL**), the University of Oxford Robotics Research Group, **Carnegie Mellon** (**CMU**) Robotics Institute, and the **California Institute of Technology** (**CalTech**) Computational Vision Group. These laboratories also had the resources  to curate big amounts of data on their own to serve the work of local  scientists, and their compute clusters were powerful enough to work with that scale of data. 

However, the beginning of the 2000s brought a change to this  landscape. Fast internet connections enabled it to become a hub for  research and data exchange, and in parallel, compute and storage power  exponentially increased year over year. This democratization of large-scale computer vision work has brought the creation of seminal big datasets for computer vision work, such as **MNIST** (1998), **C****MU PIE** (2000), **CalTech 101** (2003), and **MIT's LabelMe** (2005). The release of these datasets also spurred algorithm research  around large-scale image classification, detection, and recognition.  Some of the most seminal work in computer vision was enabled directly or indirectly by these datasets, for example, **LeCun's** handwriting recognition (circa 1990), **Viola and Jones'** cascaded boosting face detector (2001), **Lowe's** SIFT (1999, 2004), **Dalal's** HoG people classifier (2005), and many more. 

The second half of the 2000s saw a sharp increase in data offerings, with many big datasets released, such as **CalTech 256** (2006), **ImageNet** (2009), **CIFAR-10** (2009), and **PASCAL VOC** (2010), all of which still play a vital role in today's research. With  the advent of deep neural networks around 2010-2012, and the momentous  winning of the ImageNet large-scale visual recognition (ILSVRC)  competition by **Krizhevsky and Hinton's AlexNet** (2012),  large datasets became the fashion, and the computer vision world had  changed. ImageNet itself has grown to monstrous proportions (more than  14 million photos), and other big datasets did too, such as **Microsoft's COCO** (2015, with 2.5 million photos), **OpenImages V4** (2017, with less than nine million photos), and **MIT's ADE20K** (2017, with nearly 500,000 object segmentation instances). This recent  trend pushed researchers to think on a larger scale, and today's machine learning that tackles such data will often have tens and hundreds of  millions of parameters (in a deep neural network), compared to dozens of parameters ten years ago.

OpenCV's early claim to fame was its built-in implementation of the  Viola and Jones face detection method, based on a cascade of boosted  classifiers, which was a reason for many to select OpenCV in their  research or practice. However, OpenCV did not target data-driven  computer vision at first. In v1.0, the only machine learning algorithms  were the cascaded boosting, hidden Markov model, and some unsupervised  methods (such as K-means clustering and expectation maximization). Much  of the focus was on image processing, geometric shape and morphological  analysis, and so on. Versions 2.x and 3.x added a great deal of standard machine learning capabilities to OpenCV; among them were decision  trees, randomized forests and gradient boosting trees, **support vector machines** (**SVM**), logistic regression, Naive Bayes classification, and more. As it  stands, OpenCV is not a data-driven machine learning library, and in  recent versions this becomes more obvious. The opencv_dnn  core module lets developers use models learned with external tools (for  example, TensorFlow) to run in an OpenCV environment, where OpenCV  provides the image preprocessing and post-processing. Nevertheless,  OpenCV plays a crucial role in data-driven pipelines, and plays a  meaningful role in the scene.

## Historic algorithms in OpenCV

When starting to work on an OpenCV project, one should be aware of  its historical past. OpenCV has existed for more than 15 years as an  open source project, and despite its very dedicated management team that aims to better the library and keep it relevant, some implementations  are more outdated than others. Some APIs are left for backward  compatibility with previous versions, and others are targeted at  specific algorithmic circumstances, all while newer algorithms are  added.

Any engineer looking to choose the best performing algorithm for his  work should have the tools to inquire about a specific algorithm to see *when* it was added and what are its *origins* (for example, a research paper). That is not to suggest that anything *new* is necessarily *better*, as some basic and older algorithms are excellent performers, and in  most cases there's a clear trade-off between various metrics. For  example, a data-driven deep neural network to perform image binarization (turning a color, or grayscale image to black-and-white) will likely  reach the highest *accuracy.* However, the **Otsu method** (1979) for adaptive binary thresholding is incredibly *fast* and performs quite well in many situations. The key is therefore to  know the requirements, as well as the details of the algorithm.

## How to check when an algorithm was added to OpenCV

One of the simplest things to do in order to learn more about an  OpenCV algorithm is to see when it was added to the source tree.  Luckily, OpenCV as an open source project has retained most of its  code's history, and changes were logged in various released versions.  There are several useful resources to access this information, as  follows:

- The OpenCV source repository: https://github.com/opencv/opencv
- The OpenCV change logs: https://github.com/opencv/opencv/wiki/ChangeLog
- The OpenCV attic: https://github.com/opencv/opencv_attic
- The OpenCV documentation: https://docs.opencv.org/master/index.html

Let's examine, for example, the algorithm in the cv::solvePnP(...) function, which is one of the most useful functions for object (or camera) pose estimation.  This function is heavily used in 3D reconstruction pipelines. We can  locate solvePnP in the opencv/modules/calib3d/src/solvepnp.cpp file. Using the search feature in GitHub, we can trace solvepnp.cpp back to its initial commit (https://github.com/opencv/opencv/commit/04461a53f1a484499ce81bcd4e25a714488cf600) on April 4, 2011.

There, we can see the original solvePnP function originally resided in calibrate3d.cpp, so we can trace that function back as well. However, we soon discover  that there is not much history for that file, as it originated from the  initial commit to the new OpenCV repository in May 2010. A search in the attic repository doesn't reveal anything beyond what exists in the  original repository. The oldest version of solvePnP we have is from May 11, 2010 (https://github.com/opencv/opencv_attic/blob/8173f5ababf09218cc4838e5ac7a70328696a48d/opencv/modules/calib3d/src/calibration.cpp) and it looks like this:

```
void cv::solvePnP( const Mat& opoints, const Mat& ipoints,
                   const Mat& cameraMatrix, const Mat& distCoeffs,
                   Mat& rvec, Mat& tvec, bool useExtrinsicGuess )
{
    CV_Assert(opoints.isContinuous() && opoints.depth() == CV_32F &&
              ((opoints.rows == 1 && opoints.channels() == 3) ||
               opoints.cols*opoints.channels() == 3) &&
              ipoints.isContinuous() && ipoints.depth() == CV_32F &&
              ((ipoints.rows == 1 && ipoints.channels() == 2) ||
              ipoints.cols*ipoints.channels() == 2));
    
    rvec.create(3, 1, CV_64F);
    tvec.create(3, 1, CV_64F);
    CvMat _objectPoints = opoints, _imagePoints = ipoints;
    CvMat _cameraMatrix = cameraMatrix, _distCoeffs = distCoeffs;
    CvMat _rvec = rvec, _tvec = tvec;
    cvFindExtrinsicCameraParams2(&_objectPoints, &_imagePoints, &_cameraMatrix,
                                 &_distCoeffs, &_rvec, &_tvec, useExtrinsicGuess );
}
```

We can clearly see it is a simple wrapper around the old C API's cvFindExtrinsicCameraParams2. The code for this C API function exists in calibration.cpp (https://github.com/opencv/opencv/blob/8f15a609afc3c08ea0a5561ca26f1cf182414ca2/modules/calib3d/src/calibration.cpp#L1043), and we can verify it as it has not changed since May 2010. The newer version of solvePnP (latest commit in November 2018) adds much more functionality, adding another function (allowing the use of **RANdom SAmple Consensus** (**RANSAC**)) and several specialty PnP algorithms such as EPnP, P3P, AP3P, DLS, UPnP, and also retaining the old C API (cvFindExtrinsicCameraParams2) method when supplying the SOLVEPNP_ITERATIVE flag to the function. The old C function, upon inspection, seems to solve the pose estimation problem by either finding a **homography**, in the case of planar objects, or using the **DLT method**, and then performing an iterative refinement. 

As per usual, it'd be a mistake to directly assume  the old C method is inferior to the other methods. However, the newer  methods are indeed methods suggested decades after the DLT method (which dates back to the 1970s). For example, the UPnP method was proposed in  just *2013* by Penate-Sanchez et al. (2013). Again, without  careful examination of the particular data at hand and a comparative  study, we cannot conclude which algorithm performs best with respect to  the requirements (speed, accuracy, memory, and so on), although we can  conclude that computer vision research has certainly advanced in *40 years* from the 1970s to the 2010s. Penate-Sanchez et al. actually show in  their paper that UPnP performs much better than DLT, in terms of both  speed and accuracy, based on empirical studies they carried out with  real and simulated data. Please refer to [Chapter 9](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/1d19dccc-6cf4-471f-ab43-207a43ee3a6f.xhtml), *Finding the Best OpenCV Algorithm for the Job**,* for tips on how to compare algorithm options.

In-depth inspection of the OpenCV code should be a routine job for  serious computer vision engineers. It not only reveals potential  optimizations and guides choices by focusing on newer methods, but it  also may teach much about the algorithms themselves.

## Common pitfalls and suggested solutions

OpenCV is very feature rich and provides multiple solutions and paths to resolve a visual-understanding problem. With this great power also  comes hard work, choosing and crafting the best processing pipeline for  the project requirements. Having multiple options means that probably  finding the exact best performing solution is next to impossible, as  many pieces are interchangeable and testing *all* the possible  options is out of our reach. This problem's exponential complexity is  compounded by the input data; more unknown variance in the incoming data will make our algorithm choices even more unstable. In other words,  working with OpenCV, or any other computer vision library, is still a  matter of experience and art. A priori intuition as to the success of  one or another route to a solution is something computer vision  engineers develop with years of experience, and for the most part there  are no shortcuts.

There is, however, the option of learning from someone else's  experience. If you've purchased this book, it likely means you are  looking to do just so. In this section, we have prepared a partial list  of problems that we encountered in our years of work as computer vision  engineers. We also look to propose solutions for these problems, like we used in our own work. The list focuses on problems arising from  computer vision engineering; however, any engineer should also be aware  of common problems in *general purpose software and system engineering*, which we do not enumerate here. In practice, no system implementation  is without some problems, bugs, or under-optimizations, and even after  following our list, one will probably find there is much more left to  do.

The primary common pitfall in any engineering field is **making assumptions instead of assertions**. For any engineer, if there's an option to measure something, it should  be measured, even by approximation, establishment of lower and upper  bounds, or measuring a different highly correlated phenomenon. For some  examples on which metrics can be used for measurement in OpenCV, refer  to [Chapter 9](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/1d19dccc-6cf4-471f-ab43-207a43ee3a6f.xhtml), *Finding the Best OpenCV Algorithm for the Job*. The best made decisions are the informed ones, based on hard data and  visibility; however, that is often not the privilege of an engineer.  Some projects require a fast and cold start that forces an engineer to  rapidly build up a solution from scratch, without much data or  intuition. In such cases, the following advice can save a lot of grief:

- **Not comparing algorithm options**: 

  One pitfall engineers often make is choosing algorithms categorically based on what they encounter first, something they've done in the past and  seemed to work, or something that has a nice tutorial (someone else's  experience). This is called the

   

  anchoring

   

  or

   

  focalism

   

  cognitive bias

  , a well-known problem in decision making theory. Reiterating the words  from the last chapter, the choice of algorithm can have tremendous  impact on the results of the entire pipeline and project, in terms of  accuracy, speed, resources, and otherwise. Making uninformed decisions  when selecting algorithms is not a good idea.

  - **Solution**: OpenCV has many ways to assist in testing different options seamlessly, through common base APIs (such as Feature2D, DescriptorMatcher, SparseOpticalFlow, and more) or common function signatures (such as solvePnP and solvePnPRansac). High-level programming languages, such as Python, have even more  flexibility in interchanging algorithms; however, this is also possible  in C++ beyond polymorphism, with some instrumentation code. After  establishing a pipeline, see how you can interchange some of the  algorithms (for example, feature type or matcher type, thresholding  technique) or their parameters (for example, threshold values,  algorithm flags) and measure the effect on the final result. Strictly  changing parameters is often called **hyperparameter tuning**, which is standard practice in machine learning.

- Not unit testing homegrown solutions or algorithms

  :

   

  It is often a programmer's fallacy to believe their work is bug-free, and  that they've covered all edge cases. It is far better to err on the side of caution when it comes to computer vision algorithms, since in many  cases the input space is vastly unknown, as it is incredibly highly  dimensional. Unit tests are excellent tools to make sure functionality  doesn't break on unexpected input, invalid data, or edge cases (for  example, an empty image) and has a graceful degradation. 

  - **Solution**: Establish unit tests for any meaningful  function in your code, and make sure to cover the important parts. For  example, any function that either reads or writes image data is a good  candidate for a unit test. The unit test is a simple piece of code that  usually invokes the function a number of times with different arguments, testing the function's ability (or inability) to handle the input.  Working in C++, there are many options for a test framework; one such  framework is part of the Boost C++ package, Boost.Test (https://www.boost.org/doc/libs/1_66_0/libs/test/doc/html/index.html). Here is an example:

```
#define BOOST_TEST_MODULE binarization test
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE( binarization_test )
{
    // On empty input should return empty output
    BOOST_TEST(binarization_function(cv::Mat()).empty())
    // On 3-channel color input should return 1-channel output
    cv::Mat input = cv::imread("test_image.png");
    BOOST_TEST(binarization_function(input).channels() == 1)
}
```

After compiling this file, it creates an executable that will perform the tests and exit with a status of 0 if all tests passed or 1 if any of them failed. It is common to mix this approach with **CMake's** **CTest** (https://cmake.org/cmake/help/latest/manual/ctest.1.html) feature (via ADD_TEST in the CMakeLists.txt files), which facilitates building tests for many parts of the code and running them all upon command.

- Not checking data ranges

  :

   

  A common problem in computer vision programming is to assume a range for the  data, for example a range of [0, 1] for floating-point pixels (

  float

  , 

  CV_32F

  ) or [0, 255] for byte pixels (

  unsigned char

  ,

  CV_8U

  ). There really are no guarantees that these assumptions hold in any  situation, since the memory block can hold any value. The problems that  arise from these errors are mostly value saturation, when trying to  write a value bigger than the representation; for example, writing 325  into a byte that can hold [0, 255] will saturate to 255, losing a great  deal of precision. Other potential problems are differences between  expected and actual data, for example, expecting a depth image in the  range of [0, 2048] (for example, two meters in millimeters) only to see  the actual range is [0, 1], meaning 

  it was normalized  somehow. This can lead to underperformance in the algorithm, or a  complete breakdown (imagine dividing the [0, 1] range by 2048 again).

  - **Solution**: Check the input data range and make sure  it is what you expect. If the range is not within acceptable bounds, you may throw an out_of_range exception (a standard library class, visit https://en.cppreference.com/w/cpp/error/out_of_range for more details). You can also consider using CV_ASSERT to check the range, which will trigger a cv::error exception on failure.

- **Data types, channels, conversion, and rounding errors**: 

  One of the most vexing problems in OpenCV's 

  cv::Mat

  data structure 

  is that it doesn't carry data type information on its variable type. A

  cv::Mat

  can hold any type of data (

  float

  ,

  uchar

  ,

  int

  ,

  short

  , and so on) in any size, and a receiving function cannot know what data  is inside the array without inspection or convention. The problem is  also compounded by the number of channels, as an array can hold any  number of them arbitrarily (for example, a

  cv::Mat

  can hold

  CV_8UC1

  or

  CV_8UC3

  ). Failing to have a known data type can lead to runtime exceptions from  OpenCV functions that don't expect such data, and therefore to potential crashing of the entire application. Problems with handling multiple  data types on the same input

  cv::Mat

  may lead to other issues of conversion. For example, if we know an incoming array holds

  CV_32F

  (by checking

  input.type() == CV_32F

  ), we may

  input.convertTo(out, CV_8U)

  to "normalize" it to a

  uchar

   character; however, if the

  float

  data is in the [0, 1] range, the output conversion will have all 0s and 1s in a [0, 255] image, which may be a problem.

  - **Solution**: Prefer cv::Mat_<> types (for example, cv::Mat_<float>) over cv::Mat to also carry the data type, establish very clear conventions on variable naming (for example cv::Mat image_8uc1), test to make sure the types you expect are the types you get, or create a "normalization" scheme to turn any unexpected input type to the type  you would like into work with in your function. Using try .. catch blocks is also a good practice when data type uncertainty is feared.

- Colorspace-originating problems: RGB versus perceptual (HSV, L*a*b*) versus technical (YUV)

  : Colorspaces are a way to encode color information in numeric values in a pixel  array (image). However, there are a number of problems with this  encoding. The foremost problem is that any colorspace eventually becomes a series of numbers stored in the array, and OpenCV does not keep track of colorspace information in 

  cv::Mat

  (for example, an  array may hold 3-byte RGB or 3-byte HSV, and the variable user cannot  tell the difference). This is not a good thing, because we tend to  think 

  we can do any kind of numeric manipulation on numeric

  data and it will make sense. However, in some colorspaces, certain  manipulations need to be cognizant of the colorspace. For example, in  the very useful

  HSV (Hue, Saturation, Value)

  colorspace, one must remember the

  H (Hue)

  is in fact a measure of

  degrees

  [0,360] that usually is compressed to [0,180] to fit in a

  uchar

   character. There is, therefore, no sense in putting a value of 200 in the H  channel, as it violates the colorspace definition and leads to  unexpected problems. Same goes for linear operations. If for example, we wish to dim an image by 50%, in RGB we simply divide all channels by  two; however, in HSV (or L*a*b*, Luv, and so on) one must only perform  the division on the

  V (Value)

  or

  L (Luminance)

  channels.

  

  The problem becomes much worse when working with non-byte images, such as  YUV420 or RGB555 (16-bit colorspaces). These images store pixel values  on the 

  bit

  level, not the byte level, compounding  data for more than one pixel or one channel in the same byte. For  example, an RGB555 pixel is stored in two bytes (16 bits): one bit  unused, then five bits for red, five bits for green, and five bits for  blue. All kinds of numeric operations (for example, arithmetics) in that case fail, and may cause irreparable corruption to the data.

  - **Solution**: Always know the colorspace of the data you process. When reading images from files using cv::imread, you may assume they are read in **BGR** order (standard OpenCV pixel data storage). When no colorspace  information is available, you may rely on heuristics or test the input.  In general, you should be wary of images with only two channels, as they are more than likely a bit-packed colorspace. Images with four channels are usually **ARGB** or **RGBA**, adding an **alpha channel**, and again introduce some uncertainty. Testing for perceptual  colorspaces can be done visually, by displaying the channels to the  screen. The worst of the bit-packing problem comes from working with  image files, memory blocks from external libraries, or sources. Within  OpenCV, most of the work is done on single-channel grayscale or BGR  data, but when it comes to saving to the file, or preparing an image  memory block for use in a different library, then it is important to  keep track of colorspace conversions. Remember cv::imwrite expects *BGR* data, and not any other format.

- Accuracy versus speed versus resources (CPU, memory) trade-offs and optimization

  :

   

  Most of the problems in computer vision have trade-offs between their  computation and resource efficiency. Some algorithms are fast because  they cache in memory crucial data with a fast lookup efficiency; others  may be fast because of a rough approximation they make on the input or  output that reduces accuracy. In most cases, one fetching trait comes at the expense of another. Not paying attention to these trade-offs, or  paying too much attention to them, can become a problem. 

  A common pitfall for engineers is around matters of

  optimization

  . There is under-or

  over-optimization

  ,

  premature optimization

  , unnecessary optimization, and more. When looking to optimize an  algorithm, there's a tendency to treat all optimizations as equals, when in fact there is usually just one culprit (code line or method) causing most of the inefficiency. 

  Dealing with algorithmic tradeoff or optimization is mostly a problem of research and development 

  time

  , rather than

  result

  . Engineers may spend too much or not enough time in optimization, or optimize at the wrong time. 

  - **Solution**: Know the algorithms before or while  employing them. If you choose an algorithm, make sure you have an  understanding of its the complexity (runtime and resource) by testing  it, or at least by looking at the OpenCV documentation pages. For  example, when matching image features, one should know the brute-force  matcher BFMatcher is often a few orders of magnitude slower than the approximate FLANN-based matcher FlannBasedMatcher, especially if preloading and caching the features is possible. 

## Summary

OpenCV is becoming a mature computer vision library, more than 15  years in the making. During that time, it saw many revolutions happen,  both in the computer vision world and in the OpenCV community.

In this chapter, we reviewed OpenCV's past through a practical lens  of understanding how to work with it better. We focused on one  particular good practice, inspecting the historical OpenCV code to find  the origins of an algorithm, in order to make better choices. To cope  with the abundance of functionality and features, we also proposed  solutions to some common pitfalls in developing computer vision  applications with OpenCV.

# Other Books You May Enjoy

If you enjoyed this book, you may be interested in these other books by Packt:

[![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/ea033480-52bf-42bd-b870-a21eac6e2938.png)](https://www.packtpub.com/application-development/learn-opencv-4-building-projects-second-edition)

**Learn OpenCV 4 By Building Projects - Second Edition**
 David Millán Escrivá, Vinícius G. Mendonça, Prateek Joshi

ISBN: 9781789341225

- Install OpenCV 4 on your operating system
- Create CMake scripts to compile your C++ application
- Understand basic image matrix formats and filters
- Explore segmentation and feature extraction techniques
- Remove backgrounds from static scenes to identify moving objects for surveillance
- Employ various techniques to track objects in a live video
- Work with new OpenCV functions for text detection and recognition with Tesseract
- Get acquainted with important deep learning tools for image classification

[![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/f9b84fb3-1915-4cae-b677-0fac47a76226.png)](https://www.packtpub.com/application-development/hands-gpu-accelerated-computer-vision-opencv-and-cuda)

**Hands-On GPU-Accelerated Computer Vision with OpenCV and CUDA**
 Bhaumik Vaidya

ISBN: 9781789348293

- Understand how to access GPU device properties and capabilities from CUDA programs
- Learn how to accelerate searching and sorting algorithms
- Detect shapes such as lines and circles in images
- Explore object tracking and detection with algorithms
- Process videos using different video analysis techniques in Jetson TX1
- Access GPU device properties from the PyCUDA program
- Understand how kernel execution works