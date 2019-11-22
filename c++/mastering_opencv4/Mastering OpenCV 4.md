

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

## Accessing the webcam

To access a computer's webcam or camera device, you can simply call the open() function on a cv::VideoCapture object (OpenCV's method of accessing your camera device), and pass 0 as the default camera ID number. Some computers have multiple cameras attached, or they do not work with a default camera of 0, so it is common practice to allow the user to pass the desired camera  number as a command-line argument, in case they want to try camera 1, 2, or -1, for example. We will also try to set the camera resolution to 640 x 480 using cv::VideoCapture::set() to run faster on high-resolution cameras.

Depending on your  camera model, driver, or system, OpenCV might not change the properties  of your camera. It is not important for this project, so don't worry if  it does not work with your webcam.

You can put this code in the main() function of your main.cpp file:

```
auto cameraNumber = 0; 
if (argc> 1) 
cameraNumber = atoi(argv[1]); 

// Get access to the camera. 
cv::VideoCapture camera; 
camera.open(cameraNumber); 
if (!camera.isOpened()) { 
   std::cerr<<"ERROR: Could not access the camera or video!"<< std::endl; 
   exit(1); 
} 

// Try to set the camera resolution. 
camera.set(cv::CV_CAP_PROP_FRAME_WIDTH, 640); 
camera.set(cv::CV_CAP_PROP_FRAME_HEIGHT, 480);
```

After the webcam has been initialized, you can grab the current camera image as a cv::Mat object (OpenCV's image container). You can grab each camera frame by using the C++ streaming operator from your cv::VideoCapture object in a cv::Mat object, just like if you were getting input from a console.

OpenCV makes it very easy to capture frames from a video file (such as an AVI or MP4 file)  or network stream instead of a webcam. Instead of passing an integer  such as camera.open(0), pass a string such as camera.open("my_video.avi") and then grab frames just like it was a webcam. The source code provided with this book has an initCamera() function that opens a webcam, video file, or network stream.

## Main camera processing loop for a desktop app

If you want to display a GUI window on the screen using OpenCV, you call the cv::namedWindow() function and then the cv::imshow() function for each image, but you must also call cv::waitKey() once per frame, otherwise your windows will not update at all! Calling cv::waitKey(0) waits forever until the user hits a key in the window, but a positive number such as waitKey(20) or higher will wait for at least that many milliseconds.

Put this main loop in the main.cpp file, as the basis of your real-time camera app:

```
while (true) { 
    // Grab the next camera frame. 
    cv::Mat cameraFrame; 
    camera >> cameraFrame; 
    if (cameraFrame.empty()) { 
        std::cerr<<"ERROR: Couldn't grab a camera frame."<< 
        std::endl; 
        exit(1); 
    } 
    // Create a blank output image, that we will draw onto. 
    cv::Mat displayedFrame(cameraFrame.size(), cv::CV_8UC3); 

    // Run the cartoonifier filter on the camera frame. 
    cartoonifyImage(cameraFrame, displayedFrame); 

    // Display the processed image onto the screen. 
    imshow("Cartoonifier", displayedFrame); 

    // IMPORTANT: Wait for atleast 20 milliseconds, 
    // so that the image can be displayed on the screen! 
    // Also checks if a key was pressed in the GUI window. 
    // Note that it should be a "char" to support Linux. 
    auto keypress = cv::waitKey(20); // Needed to see anything! 
    if (keypress == 27) { // Escape Key 
       // Quit the program! 
       break; 
    } 
 }//end while
```

## Generating a black and white sketch

To obtain a sketch (black and white drawing) of the camera frame, we will use an edge detection filter, whereas to  obtain a color painting, we will use an edge preserving filter  (bilateral filter) to further smooth the flat regions while keeping  edges intact. By overlaying the sketch drawing on top of the color  painting, we obtain a cartoon effect, as shown earlier in the screenshot of the final app.

There are many different edge detection  filters, such as Sobel, Scharr, and Laplacian filters, or a Canny edge  detector. We will use a Laplacian edge filter since it produces edges  that look most similar to hand sketches compared to Sobel or Scharr, and is quite consistent compared to a Canny edge detector, which produces  very clean line drawings but is affected more by random noise in the  camera frames, and therefore the line drawings would often change  drastically between frames.

Nevertheless, we still need to reduce the  noise in the image before we use a Laplacian edge filter. We will use a  median filter because it is good at removing noise while keeping edges  sharp, but is not as slow as a bilateral filter. Since Laplacian filters use grayscale images, we must convert from OpenCV's default BGR format  to grayscale. In your empty cartoon.cpp file, put this code at the top so you can access OpenCV and STD C++ templates without typing cv:: and std:: everywhere:

```
// Include OpenCV's C++ Interface 
 #include <opencv2/opencv.hpp> 

 using namespace cv; 
 using namespace std;
```

Put this and all remaining code in a cartoonifyImage() function in your cartoon.cpp file:

```
Mat gray; 
 cvtColor(srcColor, gray, CV_BGR2GRAY); 
 const int MEDIAN_BLUR_FILTER_SIZE = 7; 
 medianBlur(gray, gray, MEDIAN_BLUR_FILTER_SIZE); 
 Mat edges; 
 const int LAPLACIAN_FILTER_SIZE = 5; 
 Laplacian(gray, edges, CV_8U, LAPLACIAN_FILTER_SIZE);
```

The Laplacian filter produces edges with  varying brightness, so to make the edges look more like a sketch, we  apply a binary threshold to make the edges either white or black:

```
Mat mask; 
 const int EDGES_THRESHOLD = 80; 
 threshold(edges, mask, EDGES_THRESHOLD, 255, THRESH_BINARY_INV);
```

In the following diagram, you see  the original image (to the left) and the generated edge mask (to the  right), which looks similar to a sketch drawing. After we generate a  color painting (explained later), we also put this edge mask on top to  have black line drawings:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/4dcc0951-4099-4a05-acfe-a6565c207408.png)

## Generating a color painting and a cartoon

A strong  bilateral filter smooths flat regions while keeping edges sharp, and  therefore is great as an automatic cartoonifier or painting filter,  except that it is extremely slow (that is, measured in seconds or even  minutes, rather than milliseconds!). Therefore, we will use some tricks  to obtain a nice cartoonifier, while still running at an acceptable  speed. The most important trick we can use is that we can perform  bilateral filtering at a lower resolution and it will still have a  similar effect as a full resolution, but run much faster. Let's reduce the total number of pixels by four (for example, half width and half height):

```
Size size = srcColor.size(); 
Size smallSize; 
smallSize.width = size.width/2; 
smallSize.height = size.height/2; 
Mat smallImg = Mat(smallSize, CV_8UC3); 
resize(srcColor, smallImg, smallSize, 0,0, INTER_LINEAR);
```

Rather than applying a large bilateral  filter, we will apply many small bilateral filters, to produce a strong  cartoon effect in less time. We will truncate the filter (refer to the  following diagram) so that instead of performing a whole filter (for  example, a filter size of 21 x 21, when the bell curve is 21 pixels  wide), it just uses the minimum filter size needed for a convincing  result (for example, with a filter size of just 9 x 9 even if the bell  curve is 21 pixels wide). This truncated filter will apply the major  part of the filter (gray area) without wasting time on the minor part of the filter (white area under the curve), so it will run several times  faster:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/1c36205a-1c7b-446e-b674-626a44bfa09f.png)

Therefore, we have four parameters that  control the bilateral filter: color strength, positional strength, size, and repetition count. We need a temp Mat since the bilateralFilter() function can't overwrite its input (referred to as **in-place processing**), but we can apply one filter storing a temp Mat and another filter storing back the input:

```
Mat tmp = Mat(smallSize, CV_8UC3); 
auto repetitions = 7; // Repetitions for strong cartoon effect. 
for (auto i=0; i<repetitions; i++) { 
    auto ksize = 9; // Filter size. Has large effect on speed. 
    double sigmaColor = 9; // Filter color strength. 
    double sigmaSpace = 7; // Spatial strength. Affects speed. 
    bilateralFilter(smallImg, tmp, ksize, sigmaColor, sigmaSpace); 
    bilateralFilter(tmp, smallImg, ksize, sigmaColor, sigmaSpace); 
}
```

Remember that this was applied to the  shrunken image, so we need to expand the image back to the original  size. Then, we can overlay the edge mask that we found earlier. To  overlay the edge mask sketch onto the bilateral  filter painting (left-hand side of the following image), we can start  with a black background and copy the painting pixels that aren't edges  in the sketch mask:

```
Mat bigImg; 
 resize(smallImg, bigImg, size, 0,0, INTER_LINEAR); 
 dst.setTo(0); 
 bigImg.copyTo(dst, mask);
```

The result is a cartoon version of the original photo, as shown on the right-hand side of the following image, where the *sketch* mask is overlaid on the painting:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/812bd780-36ce-470d-b69b-8e72fb960a53.png)



# Generating an evil mode using edge filters

Cartoons and comics always have both good  and bad characters. With the right combination of edge filters, a scary  image can be generated from the most innocent looking people! The trick  is to use a small edge filter that will find many edges all over the  image, then merge the edges using a small median filter.

We will perform this on a grayscale image  with some noise reduction, so the preceding code for converting the  original image to grayscale and applying a 7 x 7 median filter should  still be used (the first image in the following diagram shows the output of the grayscale median blur). Instead of following it with a Laplacian filter and Binary threshold, we can get a scarier look if we apply a 3 x 3 Scharr gradient filter along *x* and *y* (second image in the diagram), then a binary threshold with a very low cutoff (third image in the diagram), and a 3 x 3 median blur, producing the final *evil* mask (fourth image in the diagram):

```
Mat gray;
 cvtColor(srcColor, gray, CV_BGR2GRAY);
 const int MEDIAN_BLUR_FILTER_SIZE = 7;
 medianBlur(gray, gray, MEDIAN_BLUR_FILTER_SIZE);
 Mat edges, edges2;
 Scharr(srcGray, edges, CV_8U, 1, 0);
 Scharr(srcGray, edges2, CV_8U, 1, 0, -1);
 edges += edges2;
 // Combine the x & y edges together.
 const int EVIL_EDGE_THRESHOLD = 12
 threshold(edges, mask, EVIL_EDGE_THRESHOLD, 255,
 THRESH_BINARY_INV);
 medianBlur(mask, mask, 3)
```

The following diagram shows the evil effect applied in the fourth image:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/0b25aa5d-c6dc-43d5-aeed-9b1a9c76826e.png)

Now that we have an *evil* mask, we can overlay this mask onto the *cartoonified* painting image as we did with the regular *sketch* edge mask. The final result is shown on the right-hand side of the following diagram:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/894dd521-2f6c-4afe-a1ad-0a482ee5924f.png)

## Generating an alien mode using skin detection

Now that we have a *sketch* mode, a *cartoon* mode (*painting* + *sketch* mask), and an *evil* mode (*painting* + *evil* mask), for fun, let's try something more complex: an *alien* mode, by detecting the skin regions of the face and then changing the skin color to green.

## Skin detection algorithm

There are many different techniques used for detecting skin regions, from simple color thresholds using **RGB** (short for **Red-Green-Blue**) or **HSV** (short for **Hue-Saturation-Brightness**) values, or color histogram calculation and re-projection, to complex machine  learning algorithms of mixture models that need camera calibration in  the **CIELab** color space, offline training  with many sample faces, and so on. But even the complex methods don't  necessarily work robustly across various camera and lighting conditions  and skin types. Since we want our skin detection to run on an embedded  device, without any calibration or training, and we are just using skin  detection for a fun image filter; it is sufficient for us to use a  simple skin detection method. However, the color responses from the tiny camera sensor in the Raspberry Pi Camera Module tend to vary  significantly, and we want to support skin detection for people of any  skin color but without any calibration, so we need something more robust than simple color thresholds.

For example, a simple HSV skin detector can  treat any pixel as skin if its hue color is fairly red, saturation is  fairly high but not extremely high, and its brightness is not too dark  or extremely bright. But cameras in mobile phones or Raspberry Pi Camera Modules often have bad white balancing; therefore, a person's skin  might look slightly blue instead of red, for instance, and this would be a major problem for simple HSV thresholding.

A more robust solution is to perform face detection with a Haar or LBP cascade classifier (shown in [Chapter 5](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/86c5b037-c45d-4622-9131-078fca1cf397.xhtml), *Face Detection and Recognition with the DNN Module*), then look at the range of colors for the pixels in the middle of the  detected face, since you know that those pixels should be skin pixels of the actual person. You could then scan the whole image or nearby region for pixels of a similar color as the center of the face. This has the  advantage that it is very likely to find at least some of the true skin  region of any detected person, no matter what their skin color is or  even if their skin appears somewhat blueish or reddish in the camera  image.

Unfortunately, face detection using cascade  classifiers is quite slow on current embedded devices, so that method  might be less ideal for some real-time embedded applications. On the  other hand, we can take advantage of the fact that for mobile apps and  some embedded systems, it can be expected that the user will be facing  the camera directly from a very close distance, so it can be reasonable  to ask the user to place their face at a specific location and distance, rather than try to detect the location and size of their face. This is  the basis of many mobile phone apps, where the app asks the user to  place their face at a certain position or perhaps to manually drag  points on the screen to show where the corners of their face are in a  photo. So, let's simply draw the outline of a face in the center of the  screen, and ask the user to move their face to the position and size shown.

## Showing the user where to put their face

When the *alien* mode is first  started, we will draw the face outline on top of the camera frame so the user knows where to put their face. We will draw a big ellipse covering 70% of the image height, with a fixed aspect ratio of 0.72, so that the face will not become too skinny or fat depending on the aspect ratio of the camera:

```
// Draw the color face onto a black background.
 Mat faceOutline = Mat::zeros(size, CV_8UC3);
 Scalar color = CV_RGB(255,255,0); // Yellow.
 auto thickness = 4;
 
 // Use 70% of the screen height as the face height.
 auto sw = size.width;
 auto sh = size.height;
 int faceH = sh/2 * 70/100; // "faceH" is radius of the ellipse.
 
 // Scale the width to be the same nice shape for any screen width.
 int faceW = faceH * 72/100;
 // Draw the face outline.
 ellipse(faceOutline, Point(sw/2, sh/2), Size(faceW, faceH),
 0, 0, 360, color, thickness, CV_AA);
```

To make it more obvious that it is a face,  let's also draw two eye outlines. Rather than drawing an eye as an  ellipse, we can give it a bit more realism (refer to the following  image) by drawing a truncated ellipse for the top of the eye and a  truncated ellipse for the bottom of the eye, because we can specify the  start and end angles when drawing with the ellipse() function:

```
// Draw the eye outlines, as 2 arcs per eye.
 int eyeW = faceW * 23/100;
 int eyeH = faceH * 11/100;
 int eyeX = faceW * 48/100;
 int eyeY = faceH * 13/100;
 Size eyeSize = Size(eyeW, eyeH);
 
 // Set the angle and shift for the eye half ellipses.
 auto eyeA = 15; // angle in degrees.
 auto eyeYshift = 11;
 
 // Draw the top of the right eye.
 ellipse(faceOutline, Point(sw/2 - eyeX, sh/2 -eyeY),
 eyeSize, 0, 180+eyeA, 360-eyeA, color, thickness, CV_AA);
 
 // Draw the bottom of the right eye.
 ellipse(faceOutline, Point(sw/2 - eyeX, sh/2 - eyeY-eyeYshift),
 eyeSize, 0, 0+eyeA, 180-eyeA, color, thickness, CV_AA);
 
 // Draw the top of the left eye.
 ellipse(faceOutline, Point(sw/2 + eyeX, sh/2 - eyeY),
 eyeSize, 0, 180+eyeA, 360-eyeA, color, thickness, CV_AA);
 
 // Draw the bottom of the left eye.
 ellipse(faceOutline, Point(sw/2 + eyeX, sh/2 - eyeY-eyeYshift),
 eyeSize, 0, 0+eyeA, 180-eyeA, color, thickness, CV_AA);
```

We can do the same to draw the bottom lip of the mouth:

```
// Draw the bottom lip of the mouth.
 int mouthY = faceH * 48/100;
 int mouthW = faceW * 45/100;
 int mouthH = faceH * 6/100;
 ellipse(faceOutline, Point(sw/2, sh/2 + mouthY), Size(mouthW,
 mouthH), 0, 0, 180, color, thickness, CV_AA);
```

To make it even more obvious that the user should put their face where shown, let's write a message on the screen!

```
// Draw anti-aliased text.
 int fontFace = FONT_HERSHEY_COMPLEX;
 float fontScale = 1.0f;
 int fontThickness = 2;
 char *szMsg = "Put your face here";
 putText(faceOutline, szMsg, Point(sw * 23/100, sh * 10/100),
 fontFace, fontScale, color, fontThickness, CV_AA);
```

Now that we have the face outline drawn, we  can overlay it onto the displayed image by using alpha blending to  combine the cartoonified image with this drawn outline:

```
addWeighted(dst, 1.0, faceOutline, 0.7, 0, dst, CV_8UC3);
```

This results in the outline in the following image, showing the user where to put their face, so we don't have to  detect the face location:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/d6bcca80-cfee-412d-9671-066166364c78.png)

## Implementation of the skin color changer

Rather than detecting the skin color and then the region with that skin color, we can use OpenCV's floodFill() function, which is similar to the bucket fill tool in most image editing  software. We know that the regions in the middle of the screen should be skin pixels (since we asked the user to put their face in the middle),  so to change the whole face to have green skin, we can just apply a  green flood fill on the center pixel, which will always color some parts of the face green. In reality, the color, saturation, and brightness  are likely to be different in different parts of the face, so a flood  fill will rarely cover all the skin pixels of a face unless the  threshold is so low that it also covers unwanted pixels outside of the  face. So instead of applying a single flood fill in the center of the  image, let's apply a flood fill on six different points around the face  that should be skin pixels.

A nice feature of OpenCV's floodFill() is that it can draw the flood fill in an external image rather than modify the input image. So, this feature can give us a mask image for  adjusting the color of the skin pixels without necessarily changing the  brightness or saturation, producing a more realistic image than if all  the skin pixels became an identical green pixel (losing significant face detail).

Skin color changing does not work so well in the RGB color space, because you want to allow brightness to vary in  the face but not allow skin color to vary much, and RGB does not  separate brightness from the color. One solution is to use the HSV color space since it separates the brightness from the color (hue) as well as the colorfulness (Saturation). Unfortunately, HSV wraps the hue value  around red, and since the skin is mostly red, it means that you need to  work both with *hue < 10%* and *hue > 90%* since these are both red. So, instead we will use the **Y'CrCb** color space (the variant of YUV that is in OpenCV), since it separates  brightness from color and only has a single range of values for typical  skin color rather than two. Note that most cameras, images, and videos  actually use some type of YUV as their color space before conversion to  RGB, so in many cases you can get a YUV image free without converting it yourself.

Since we want our alien mode to look like a  cartoon, we will apply the alien filter after the image has already been cartoonified. In other words, we have access to the shrunken color  image produced by the bilateral filter, and access to the full-sized  edge mask. Skin detection often works better at low resolutions, since  it is the equivalent of analyzing the average value of each  high-resolution pixel's neighbors (or the low-frequency signal instead  of the high-frequency noisy signal). So, let's work at the same shrunken scale as the bilateral filter (half-width and half-height). Let's  convert the painting image to YUV:

```
Mat yuv = Mat(smallSize, CV_8UC3);
 cvtColor(smallImg, yuv, CV_BGR2YCrCb);
```

We also need to shrink the edge mask so it is on the same scale as the painting image. There is a complication with OpenCV's floodFill() function, when storing to a separate mask image, in that the mask should have a  one-pixel border around the whole image, so if the input image is *W x H* pixels in size, then the separate mask image should be *(W+2) x (H+2)* pixels in size. But the floodFill() function also allows us to initialize the mask with edges that the flood fill  algorithm will ensure it does not cross. Let's use this feature, in the  hope that it helps prevent the flood fill from extending outside of the  face. So, we need to provide two mask images: one is the edge mask of *W x H* in size, and the other image is the exact same edge mask but *(W+2) x (H+2)* in size because it should include a border around the image. It is possible to have multiple cv::Mat objects (or headers) referencing the same data, or even to have a cv::Mat object that references a sub-region of another cv::Mat image. So, instead of allocating two separate images and copying the edge mask pixels across, let's allocate a single mask image including the border, and create an extra cv::Mat header of *W x H* (which just references the region of interest in the flood fill mask without  the border). In other words, there is just one array of pixels of size *(W+2) x (H+2)* but two cv::Mat objects, where one is referencing the whole *(W+2) x (H+2)* image and the other is referencing the *W x H* region in the middle of that image:

```
auto sw = smallSize.width;
auto sh = smallSize.height;
Mat mask, maskPlusBorder;
maskPlusBorder = Mat::zeros(sh+2, sw+2, CV_8UC1);
mask = maskPlusBorder(Rect(1,1,sw,sh));
// mask is now in maskPlusBorder.
resize(edges, mask, smallSize); // Put edges in both of them.
```

The edge mask (shown on the left of the  following diagram) is full of both strong and weak edges, but we only  want strong edges, so we will apply a binary threshold (resulting in the middle image in the following diagram). To join some gaps between  edges, we will then combine the morphological operators dilate() and erode() to remove some gaps (also referred to as the close operator), resulting in the image on the right:

```
const int EDGES_THRESHOLD = 80;
 threshold(mask, mask, EDGES_THRESHOLD, 255, THRESH_BINARY);
 dilate(mask, mask, Mat());
 erode(mask, mask, Mat());
```





We can see the result of applying thresholding and morphological  operation in the following image, first image is the input edge map,  second the thresholding filter, and last image is the dilate and erode  morphological filters:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/351d8dc8-379f-48b0-87e2-25d3e88ecb0b.png)

As mentioned earlier, we want to apply flood fills in numerous points around the face, to make sure we include the  various colors and shades of the whole face. Let's choose six points  around the nose, cheeks, and forehead, as shown on the left-hand side of the following screenshot. Note that these values are dependent on the  face outline being drawn earlier:

```
auto const NUM_SKIN_POINTS = 6;
Point skinPts[NUM_SKIN_POINTS];
skinPts[0] = Point(sw/2, sh/2 - sh/6);
skinPts[1] = Point(sw/2 - sw/11, sh/2 - sh/6);
skinPts[2] = Point(sw/2 + sw/11, sh/2 - sh/6);
skinPts[3] = Point(sw/2, sh/2 + sh/16);
skinPts[4] = Point(sw/2 - sw/9, sh/2 + sh/16);
skinPts[5] = Point(sw/2 + sw/9, sh/2 + sh/16);
```

Now, we just need to find some good lower  and upper bounds for the flood fill. Remember that this is being  performed in the Y'CrCb color space, so we basically decide how much the brightness can vary, how much the red component can vary, and how much  the blue component can vary. We want to allow the brightness to vary a  lot, to include shadows as well as highlights and reflections, but we  don't want the colors to vary much at all:

```
const int LOWER_Y = 60;
 const int UPPER_Y = 80;
 const int LOWER_Cr = 25;
 const int UPPER_Cr = 15;
 const int LOWER_Cb = 20;
 const int UPPER_Cb = 15;
 Scalar lowerDiff = Scalar(LOWER_Y, LOWER_Cr, LOWER_Cb);
 Scalar upperDiff = Scalar(UPPER_Y, UPPER_Cr, UPPER_Cb);
```

We will use the floodFill() function with its default flags, except that we want to store to an external mask, so we must specify FLOODFILL_MASK_ONLY:

```
const int CONNECTED_COMPONENTS = 4; // To fill diagonally, use 8.
const int flags = CONNECTED_COMPONENTS | FLOODFILL_FIXED_RANGE
| FLOODFILL_MASK_ONLY; 
Mat edgeMask = mask.clone(); // Keep a copy of the edge mask.
// "maskPlusBorder" is initialized with edges to block floodFill().
for (int i = 0; i < NUM_SKIN_POINTS; i++) {
  floodFill(yuv, maskPlusBorder, skinPts[i], Scalar(), NULL,
  lowerDiff, upperDiff, flags);
}
```

The following image on the left-hand side  shows the six flood fill locations (shown as circles), and the  right-hand side of the image shows the external mask that is generated,  where the skin is shown as gray and edges are shown as white. Note that  the right-hand image was modified for this book so that skin pixels (of  value 1) are clearly visible:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/9e8f68aa-c6fe-4973-8e78-7f48e4b8251a.png)

The mask image (shown on the right-hand side of the preceding image) now contains the following:

- Pixels of value 255 for the edge pixels
- Pixels of value 1 for the skin regions
- Pixels of value 0 for the rest

Meanwhile, edgeMask just contains edge pixels (as value 255). So to get just the skin pixels, we can remove the edges from it:

```
mask -= edgeMask;
```

The mask variable now just  contains 1s for skin pixels and 0s for non-skin pixels. To change the  skin color and brightness of the original image, we can use the cv::add() function with the skin mask to increase the green component in the original BGR image:

```
auto Red = 0;
auto Green = 70;
auto Blue = 0;
add(smallImgBGR, CV_RGB(Red, Green, Blue), smallImgBGR, mask);
```

The following diagram shows the original  image on the left and the final alien cartoon image on the right, where  at least six parts of the face will now be green!

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/5d7c5870-d7f8-4f64-af89-04118fc91711.png)

Notice that we have made the skin look green but also brighter (to look like an alien that glows in the dark). If  you want to just change the skin color without making it brighter, you  can use other color changing methods, such as adding 70 to green while subtracting 70 from red and blue, or convert to the HSV color space using cvtColor(src, dst, "CV_BGR2HSV_FULL") and adjust the hue and saturation.

## Reducing the random pepper noise from the sketch image

Most of the tiny cameras in smartphones,  Raspberry Pi Camera Modules, and some webcams have significant image  noise. This is normally acceptable, but it has a big effect on our 5 x 5 Laplacian edge filter. The edge mask (shown as the sketch mode) will  often have thousands of small blobs of black pixels called **pepper noise**, made of several black pixels next to each other on a white background.  We are already using a median filter, which is usually strong enough to  remove pepper noise, but in our case it may not be strong enough. Our  edge mask is mostly a pure white background (value of 255) with some  black edges (value of 0) and the dots of noise (also value of 0). We  could use a standard closing morphological operator, but it will remove a lot of edges. So instead, we will apply a custom filter that removes  small black regions that are surrounded completely by white pixels. This will remove a lot of noise while having little effect on actual edges.

We will scan the image for black pixels, and at each black pixel, we'll check the border of the 5 x 5 square around  it to see if all the 5 x 5 border pixels are white. If they are all  white, then we know we have a small island of black noise, so then we  fill the whole block with white pixels to remove the black island. For  simplicity in our 5 x 5 filter, we will ignore the two border pixels  around the image and leave them as they are.

The following diagram shows the original image from an Android tablet on the left side, with a sketch  mode in the center, showing small black dots of pepper noise and the  result of our pepper noise removal shown on the right-hand side, where  the skin looks cleaner:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/7211fa5b-df44-4c7e-b272-a850cbaf53cc.png)

The following code can be named the removePepperNoise() function to edit the image in place for simplicity:

```
void removePepperNoise(Mat &mask)
{
    for (int y=2; y<mask.rows-2; y++) {
    // Get access to each of the 5 rows near this pixel.
    uchar *pUp2 = mask.ptr(y-2);
    uchar *pUp1 = mask.ptr(y-1);
    uchar *pThis = mask.ptr(y);
    uchar *pDown1 = mask.ptr(y+1);
    uchar *pDown2 = mask.ptr(y+2);
 
    // Skip the first (and last) 2 pixels on each row.
    pThis += 2;
    pUp1 += 2;
    pUp2 += 2;
    pDown1 += 2;
    pDown2 += 2;
    for (auto x=2; x<mask.cols-2; x++) {
       uchar value = *pThis; // Get pixel value (0 or 255).
       // Check if it's a black pixel surrounded bywhite
       // pixels (ie: whether it is an "island" of black).
       if (value == 0) {
          bool above, left, below, right, surroundings;
          above = *(pUp2 - 2) && *(pUp2 - 1) && *(pUp2) && *(pUp2 + 1) 
            && *(pUp2 + 2);
          left = *(pUp1 - 2) && *(pThis - 2) && *(pDown1 - 2);
          below = *(pDown2 - 2) && *(pDown2 - 1) && (pDown2) &&
            (pDown2 + 1) && *(pDown2 + 2);
          right = *(pUp1 + 2) && *(pThis + 2) && *(pDown1 + 2);
          surroundings = above && left && below && right;
          if (surroundings == true) {
             // Fill the whole 5x5 block as white. Since we
             // knowthe 5x5 borders are already white, we just
             // need tofill the 3x3 inner region.
             *(pUp1 - 1) = 255;
             *(pUp1 + 0) = 255;
             *(pUp1 + 1) = 255;
             *(pThis - 1) = 255;
             *(pThis + 0) = 255;
             *(pThis + 1) = 255;
             *(pDown1 - 1) = 255;
             *(pDown1 + 0) = 255;
             *(pDown1 + 1) = 255;
             // Since we just covered the whole 5x5 block with
             // white, we know the next 2 pixels won't be
             // black,so skip the next 2 pixels on the right.
             pThis += 2;
             pUp1 += 2;
             pUp2 += 2;
             pDown1 += 2;
             pDown2 += 2;
         }
       }
       // Move to the next pixel on the right.
       pThis++;
       pUp1++;
       pUp2++;
       pDown1++;
       pDown2++;
       }
    }
 }
```

That's all! Run the app in the different modes until you are ready to port it to the embedded device!

## Porting from desktop to an embedded device

Now that the program works on the desktop,  we can make an embedded system from it. The details given here are  specific to Raspberry Pi, but similar steps apply when developing for  other embedded Linux systems such as BeagleBone, ODROID, Olimex, Jetson, and so on.

There are several different options for  running our code on an embedded system, each with some advantages and  disadvantages in different scenarios.

There are two common methods for compiling the code for an embedded device:

- Copy the source code from the desktop onto the device and compile it directly on board the device. This is often referred to as **native compilation** since we are compiling our code natively on the same system that it will eventually run on.
- Compile all the code on the desktop but using special methods to  generate code for the device, and then you copy the final executable  program onto the device. This is often referred to as **cross-compilation** since you need a special compiler that knows how to generate code for other types of CPUs.

Cross-compilation is  often significantly harder to configure than native compilation,  especially if you are using many shared libraries, but since your  desktop is usually a lot faster than your embedded device,  cross-compilation is often much faster at compiling large projects. If  you expect to be compiling your project hundreds of times, in order to  work on it for months, and your device is quite slow compared to your  desktops, such as the Raspberry Pi 1 or Raspberry Pi Zero, which are  very slow compared to a desktop, then cross-compilation is a good idea.  But in most cases, especially for small, simple projects, you should  just stick with native compilation since it is easier.

Note that all the libraries used by your  project will also need to be compiled for the device, so you will need  to compile OpenCV for your device. Natively compiling OpenCV on a  Raspberry Pi 1 can take hours, whereas cross-compiling OpenCV on a  desktop might take just 15 minutes. But you usually only need to compile OpenCV once and then you'll have it for all your projects, so it is  still worth sticking with native compilation of your project (including  the native compilation of OpenCV) in most cases.

There are also several options for how to run the code on an embedded system:

- Use the same input and output methods you used on the desktop, such  as the same video files, USB webcam, or keyboard as input, and display  text or graphics on an HDMI monitor in the same way you were doing on  the desktop.
- Use special devices for input and output. For example, instead of  sitting at a desk using a USB webcam and keyboard as input and  displaying the output on a desktop monitor, you could use the special  Raspberry Pi Camera Module for video input, use custom GPIO push buttons or sensors for input, and use a 7-inch MIPI DSI screen or GPIO LED  lights as the output, and then by powering it all with a common **portable USB charger**, you can be wearing the whole computer platform in your backpack or attaching it on your bicycle!
- Another option is to stream data in or out of the embedded device to other computers, or even use one device to stream out the camera data  and one device to use that data. For example, you can use the GStreamer  framework to configure the Raspberry Pi to stream H.264 compressed video from its camera module to the Ethernet network or through Wi-Fi, so  that a powerful PC or server rack on the local network or the Amazon AWS cloud computing services can process the video stream somewhere else.  This method allows a small and cheap camera device to be used in a  complex project requiring large processing resources located somewhere  else.

If you do wish to perform computer vision on board the device, be aware that some low-cost embedded devices such as  Raspberry Pi 1, Raspberry Pi Zero, and BeagleBone Black have  significantly less computing power than desktops or even cheap netbooks  or smartphones, perhaps 10-50 times slower than your desktop, so  depending on your application you might need a powerful embedded device  or stream video to a separate computer, as mentioned previously. If you  don't need much computing power (for example, you only need to process  one frame every 2 seconds, or you only need to use 160 x 120 image  resolution), then a Raspberry Pi Zero running some computer vision on  board might be fast enough for your requirements. But many computer  vision systems need far more computing power, and so if you want to  perform computer vision on board the device, you will often want to use a much faster device with a CPU in the range of 2 GHz, such as a  Raspberry Pi 3, ODROID-XU4, or Jetson TK1.

## Equipment setup to develop code for an embedded device

Let's begin by keeping it as simple as  possible, by using a USB keyboard and mouse and an HDMI monitor just  like our desktop system, compiling the code natively on the device, and  running our code on the device. Our first step will be to copy the code  onto the device, install the build tools, and compile OpenCV and our  source code on the embedded system.

Many embedded devices such as Raspberry Pi  have an HDMI port and at least one USB port. Therefore, the easiest way  to start using an embedded device is to plug in an HDMI monitor and USB  keyboard and mouse for the device, to configure settings and see the  output, while doing the code development and testing using your desktop  machine. If you have a spare HDMI monitor, plug that into the device,  but if you don't have a spare HDMI monitor, you might consider buying a  small HDMI screen just for your embedded device.

Also, if you don't have a spare USB keyboard and mouse, you might consider buying a wireless keyboard and mouse that has a single USB wireless dongle, so you only use up a single USB port  for both the keyboard and mouse. Many embedded devices use a 5V power  supply, but they usually need more power (electrical current) than a  desktop or laptop will provide in its USB port. So, you should obtain  either a separate 5V USB charger (at least 1.5 amps, ideally 2.5 amps)  or a portable USB battery charger that can provide at least 1.5 amps of  output current. Your device might only use 0.5 amps most of the time,  but there will be occasional times when it needs over 1 amp, so it's  important to use a power supply that is rated for at least 1.5 amps or  more, otherwise your device will occasionally reboot, or some hardware  could behave strangely at important times, or the filesystem could  become corrupt and you lose your files! A 1 amp supply might be good  enough if you don't use cameras or accessories, but 2.0-2.5 amps is  safer.

For example, the following photographs show a convenient setup containing a Raspberry Pi 3, a good quality 8 GB  micro-SD card for $10 (http://ebay.to/2ayp6Bo), a 5-inch HDMI resistive touchscreen for $30-$45 (http://bit.ly/2aHQO2G), a wireless USB keyboard and mouse for $30 (http://ebay.to/2aN2oXi), a **5V 2.5 A** power supply for $5 (https://amzn.to/2UafanD), a USB webcam such as the very fast **PS3 Eye** for just $5 (http://ebay.to/2aVWCUS), a Raspberry Pi Camera Module v1 or v2 for $15-$30 (http://bit.ly/2aF9PxD), and an Ethernet cable for $2 (http://ebay.to/2aznnjd), connecting the Raspberry Pi to the same LAN network as your development PC or laptop. Notice that this HDMI screen is designed specifically for the Raspberry Pi, since the screen plugs directly into the Raspberry Pi below it, and has an HDMI male-to-male adapter (shown in the right-hand photo) for the Raspberry Pi so you don't need an HDMI cable, whereas  other screens may require an HDMI cable (https://amzn.to/2Rvet6H), or MIPI DSI or SPI cable.

Also note that some screens and touch panels need configuration before they will work, whereas most HDMI screens  should work without any configuration:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/e27b2a0e-d6f5-4adb-a2ab-d3d14fe88515.png)

Notice the black USB webcam (on the far left of the LCD), the Raspberry Pi Camera Module (green and black board  sitting on the top-left corner of the LCD), Raspberry Pi board  (underneath the LCD), HDMI adapter (connecting the LCD to the Raspberry  Pi underneath it), a blue Ethernet cable (plugged into a router), a  small USB wireless keyboard and mouse dongle, and a micro-USB power  cable (plugged into a **5V 2.5A** power supply).



The following steps are specific to  Raspberry Pi, so if you are using a different embedded device or you  want a different type of setup, search the web about how to set up your  board. To set up an Raspberry Pi 1, 2, or 3 (including their variants  such as Raspberry Pi Zero, Raspberry Pi 2B, 3B, and so on, and Raspberry Pi 1A+ if you plug in a USB Ethernet dongle), follow these steps:

1. Get a fairly new, good quality micro-SD card of at least 8 GB. If  you use a cheap micro-SD card or an old micro-SD card that you already  used many times before and it has degraded in quality, it might not be  reliable enough to boot the Raspberry Pi, so if you have trouble booting the Raspberry Pi, you should try a good quality Class 10 micro-SD card  (such as SanDisk Ultra or better) that says it handles at least 45 Mbps  or can handle 4K video.

1. Download and burn the latest **Raspbian IMG** (not NOOBS) to the micro-SD card. Note that burning an IMG is different to simply copying the file to SD. Visit https://www.raspberrypi.org/documentation/installation/installing-images/ and follow the instructions for your desktop's OS to burn Raspbian to a  micro-SD card. Be aware that you will lose any files that were  previously on the card.
2. Plug a USB keyboard, mouse, and HDMI display into the Raspberry Pi, so you can easily run some commands and see the output.
3. Plug the Raspberry Pi into a 5V USB power supply with at least 1.5  A, ideally 2.5 A or higher. Computer USB ports aren't powerful enough.
4. You should see many pages of text scrolling while it is booting up Raspbian Linux, then it should be ready after 1 or 2 minutes.
5. If, after booting, it's just showing a black console screen with some text (such as if you downloaded **Raspbian Lite**), you are at the text-only login prompt. Log in by typing pi as the username and then hit *Enter*. Then, type raspberry as the password and hit *Enter* again.
6. Or if it booted to the graphical display, click on the black **Terminal** icon at the top to open a shell (Command Prompt).
7. Initialize some settings in your Raspberry Pi: 
   - Type sudo raspi-config and hit *Enter* (see the following screenshot).
   - First, run Expand Filesystem and then finish and reboot your device, so the Raspberry Pi can use the whole micro-SD card.
   - If you use a normal (US) keyboard, not a British keyboard, in Internationalization Options, change to Generic 104-key keyboard, Other, English (US), and then for the AltGr and similar questions, just hit *Enter* unless you are using a special keyboard.
   - In Enable Camera, enable the Raspberry Pi Camera Module.
   - In Overclock Options, set to Raspberry Pi 2 or similar to the device runs faster (but generates more heat).
   - In Advanced Options, enable the SSH server.
   - In Advanced Options, if you are using Raspberry Pi 2 or 3, change Memory Split to 256 MB so the GPU has plenty of RAM for video processing. For Raspberry Pi 1 or Zero, use 64 MB or the default.
   - Finish, then reboot the device.

1. (Optional): Delete Wolfram to save 600 MB of space on your SD card:

```
sudo apt-get purge -y wolfram-engine
```

It can be reinstalled using sudo apt-get install wolfram-engine.

To see the remaining space on your SD card, run df -h | head -2:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/c09cd443-2373-4427-bf7a-95926b0200c2.png)

1. Assuming you plugged the Raspberry Pi into your internet router, it  should already have internet access. So, update your Raspberry Pi to the latest Raspberry Pi firmware, software locations, OS, and software. **Warning**: many Raspberry Pi tutorials say you should run sudo rpi-update; however, in recent years, it's no longer a good idea to run rpi-update since it can give you an unstable system or firmware. The following  instructions will update your Raspberry Pi to have stable software and  firmware (note that these commands might take up to one hour):

```
sudo apt-get -y update
sudo apt-get -y upgrade
sudo apt-get -y dist-upgrade
sudo reboot
```

1. Find the IP address of the device:

```
hostname -I
```

1. Try accessing the device from your desktop. For example, assume the device's IP address is 192.168.2.101. Enter this on a Linux desktop:

```
ssh-X pi@192.168.2.101
```

1. Or, do this on a Windows desktop:
   1. Download, install, and run PuTTY
   2. Then in PuTTY, connect to the IP address (192.168.2.101), as the user pi with the password raspberry
2. Optionally, if you want your Command Prompt to be a different color  than the commands and show the error value after each command, use this:

```
nano ~/.bashrc
```

1. Add this line to the bottom:

```
PS1="[e[0;44m]u@h: w ($?) $[e[0m] "
```

1. Save the file (hit *Ctrl* + *X*, then hit *Y*, and then hit *Enter*).
2. Start using the new settings:

```
source ~/.bashrc
```

1. To prevent the screensaver/screen blank power saving feature in Raspbian from turning off your screen on idle, use this:

```
sudo nano /etc/lightdm/lightdm.conf
```

1. And follow these steps: 
   1. Look for the line that says #xserver-command=X (jump to line 87 by pressing *Alt* + *G* and then typing 87 and hitting *Enter*).
   2. Change it to xserver-command=X -s 0 dpms.
   3. Save the file (hit *Ctrl* + *X,* then hit *Y,* then hit *Enter*).
2. Finally, reboot the Raspberry Pi:

```
sudo reboot
```

You should be ready to start developing on the device now!

## Installing OpenCV on an embedded device

There is a very easy way to install OpenCV and all its dependencies on a Debian-based embedded device such as Raspberry Pi:

```
sudo apt-get install libopencv-dev
```

However, that might install an old version of OpenCV from one or two years ago.

To install the latest version of OpenCV on  an embedded device such as Raspberry Pi, we need to build OpenCV from  the source code. First, we install a compiler and build system, then  libraries for OpenCV to use, and finally OpenCV itself. Note that the  steps for compiling OpenCV from source on Linux are the same whether you are compiling for desktop or for embedded systems. A Linux script, install_opencv_from_source.sh, is provided with this book; it is recommended you copy the file onto  your Raspberry Pi (for example, with a USB flash stick) and run the  script to download, build, and install OpenCV, including potential  multi-core CPU and **ARM NEON SIMD** optimizations (depending on hardware support):

```
chmod +x install_opencv_from_source.sh
 ./install_opencv_from_source.sh
```

The script will  stop if there is an error, for example, if you don't have internet  access or a dependency package conflicts with something else you already installed. If the script stops with an error, try using info on the web to solve that error, then run the script again. The script will quickly check all the previous steps and then continue from where it finished  last time. Note that it will take between 20 minutes and 12 hours  depending on your hardware and software!

It's highly recommended to build and run a  few OpenCV samples every time you install OpenCV, so when you have  problems building your own code, at least you will know whether the  problem is the OpenCV installation or a problem with your code.

Let's try to build the simple edge sample program. If we try the same Linux command to build it from OpenCV 2, we get a build error:

```
cd ~/opencv-4.*/samples/cpp
 g++ edge.cpp -lopencv_core -lopencv_imgproc -lopencv_highgui
 -o edge
 /usr/bin/ld: /tmp/ccDqLWSz.o: undefined reference to symbol '_ZN2cv6imreadERKNS_6StringEi'
 /usr/local/lib/libopencv_imgcodecs.so.4..: error adding symbols: DSO missing from command line
 collect2: error: ld returned 1 exit status
```

The second to last line of that error  message tells us that a library was missing from the command line, so we simply need to add -lopencv_imgcodecs in our command next to the other OpenCV libraries we linked to. Now, you know how to fix the  problem anytime you are compiling an OpenCV 3 program and you see that  error message. So, let's do it correctly:

```
cd ~/opencv-4.*/samples/cpp
 g++ edge.cpp -lopencv_core -lopencv_imgproc -lopencv_highgui
 -lopencv_imgcodecs -o edge
```

It worked! So, now you can run the program:

```
./edge
```

Hit *Ctrl* + *C* on your keyboard to quit the program. Note that the edge program might crash if you try running the command in an SSH Terminal and you  don't redirect the window to display on the device's LCD screen. So, if  you are using SSH to remotely run the program, add DISPLAY=:0 before your command:

```
DISPLAY=:0 ./edge
```

You should also plug a USB webcam into the device and test that it works:

```
g++ starter_video.cpp -lopencv_core -lopencv_imgproc
 -lopencv_highgui -lopencv_imgcodecs -lopencv_videoio \
 -o starter_video
 DISPLAY=:0 ./starter_video 0
```

Note: if you don't have a USB webcam, you can test using a video file:

```
DISPLAY=:0 ./starter_video ../data/768x576.avi
```

Now that OpenCV is successfully installed on your device, you can run the Cartoonifier applications we developed  earlier. Copy the Cartoonifier folder onto the device (for example, by using a USB flash stick, or using scp to copy files over the network). Then, build the code just like you did for the desktop:

```
cd ~/Cartoonifier
 export OpenCV_DIR="~/opencv-3.1.0/build"
 mkdir build
 cd build
 cmake -D OpenCV_DIR=$OpenCV_DIR ..
 make
```

And run it:

```
DISPLAY=:0 ./Cartoonifier
```

And if all is fine, we will see a window with our application running as follows:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/7e425558-2663-41d6-bcb6-381a649947ff.png)

## Using the Raspberry Pi Camera Module

While using a USB webcam on Raspberry Pi has the convenience of supporting identical behavior and code on the  desktop as on an embedded device, you might consider using one of the  official Raspberry Pi Camera Modules (referred to as the **Raspberry Pi Cams**). They have some advantages and disadvantages over USB webcams.

The Raspberry Pi Cams use the special MIPI  CSI camera format, designed for smartphone cameras to use less power.  They have a smaller physical size, faster bandwidth, higher resolutions, higher frame rates, and reduced latency compared to USB. Most USB 2.0  webcams can only deliver 640 x 480 or 1280 x 720 30 FPS video since USB  2.0 is too slow for anything higher (except for some expensive USB  webcams that perform onboard video compression) and USB 3.0 is still too expensive. However, smartphone cameras (including the Raspberry Pi  Cams) can often deliver 1920 x 1080 30 FPS or even Ultra HD/4K  resolutions. The Raspberry Pi Cam v1 can, in fact, deliver upto 2592 x  1944 15 FPS or 1920 x 1080 30 FPS video even on a $5 Raspberry Pi Zero,  thanks to the use of MIPI CSI for the camera and compatible video  processing ISP and GPU hardware inside the Raspberry Pi. The Raspberry  Pi Cams also support 640 x 480 in 90 FPS mode (such as for slow-motion  capture), and this is quite useful for real-time computer vision so you  can see very small movements in each frame, rather than large movements  that are harder to analyze.

However, the Raspberry Pi Cam is a plain circuit board that is *highly sensitive* to electrical interference, static electricity, or physical damage (simply touching the small, flat orange cable with your finger can cause video  interference or even permanently damage your camera!). The big flat  white cable is far less sensitive but it is still very sensitive to  electrical noise or physical damage. The Raspberry Pi Cam comes with a  very short 15 cm cable. It's possible to buy third-party cables on eBay  with lengths between 5 cm and 1 m, but cables 50 cm or longer are less  reliable, whereas USB webcams can use 2 m to 5 m cables and can be  plugged into USB hubs or active extension cables for longer distances.

There are currently several different  Raspberry Pi Cam models, notably the NoIR version that doesn't have an  internal infrared filter; therefore, a NoIR camera can easily see in the dark (if you have an invisible infrared light source), or see infrared  lasers or signals far clearer than regular cameras that include an  infrared filter inside them. There are also two different versions of  Raspberry Pi Cam: Raspberry Pi Cam v1.3 and Raspberry Pi Cam v2.1, where v2.1 uses a wider angle lens with a Sony 8 megapixel sensor instead of a 5 megapixel **OmniVision** sensor, has better support  for motion in low lighting conditions, and adds support for 3240 x 2464  video at 15 FPS and potentially up to 120 FPS video at 720p. However,  USB webcams come in thousands of different shapes and versions, making  it easy to find specialized webcams such as waterproof or  industrial-grade webcams, rather than requiring you to create your own  custom housing for a Raspberry Pi Cam.

IP cameras are also another option for a camera interface that can  allow 1080p or higher resolution videos with Raspberry Pi, and IP  cameras support not just very long cables, but potentially even work  anywhere in the world using the internet. But IP cameras aren't quite as easy to interface with OpenCV as USB webcams or Raspberry Pi Cams.

In the past, Raspberry Pi Cams and the official drivers weren't  directly compatible with OpenCV; you often used custom drivers and  modified your code in order to grab frames from Raspberry Pi Cams, but  it's now possible to access a Raspberry Pi Cam in OpenCV in the exact  same way as a USB webcam! Thanks to recent improvements in the v4l2  drivers, once you load the v4l2 driver, the Raspberry Pi Cam will appear as a /dev/video0 or /dev/video1 file like a regular USB webcam. So, traditional OpenCV webcam code such as cv::VideoCapture(0) will be able to use it just like a webcam.

## Installing the Raspberry Pi Camera Module driver

First, let's temporarily load the v4l2 driver for the Raspberry Pi Cam to make sure our camera is plugged in correctly:

```
sudo modprobe bcm2835-v4l2
```

If the command failed (if it printed an error message to the console, it froze, or the command returned a number besides 0), then perhaps your camera is not plugged in correctly. Shut down and  then unplug power from your Raspberry Pi and try attaching the flat  white cable again, looking at photos on the web to make sure it's  plugged in the correct way around. If it is the correct way around, it's possible the cable wasn't fully inserted before you closed the locking  tab on the Raspberry Pi. Also, check whether you forgot to click Enable Camera when configuring your Raspberry Pi earlier, using the sudoraspi-config command.

If the command worked (if the command returned 0 and no error was printed to the console), then we can make sure the  v4l2 driver for the Raspberry Pi Cam is always loaded on bootup by  adding it to the bottom of the /etc/modules file:

```
sudo nano /etc/modules
 # Load the Raspberry Pi Camera Module v4l2 driver on bootup:
 bcm2835-v4l2
```

After you save the file and reboot your Raspberry Pi, you should be able to run ls /dev/video* to see a list of cameras available on your Raspberry Pi. If the Raspberry  Pi Cam is the only camera plugged into your board, you should see it as  the default camera (/dev/video0), or if you also have a USB webcam plugged in, then it will be either /dev/video0 or /dev/video1.

Let's test the Raspberry Pi Cam using the starter_video sample program we compiled earlier:

```
cd ~/opencv-4.*/samples/cpp
 DISPLAY=:0 ./starter_video 0
```

If it's showing the wrong camera, try DISPLAY=:0 ./starter_video 1.

Now that we know the Raspberry Pi Cam is working in OpenCV, let's try Cartoonifier:

```
cd ~/Cartoonifier
 DISPLAY=:0 ./Cartoonifier 0
```

Or, use DISPLAY=:0 ./Cartoonifier 1 for the other camera.

## Making Cartoonifier run in fullscreen

In embedded systems, you often want your  application to be fullscreen and hide the Linux GUI and menu. OpenCV  offers an easy method to set the fullscreen window property, but make  sure you created the window using the NORMAL flag:

```
// Create a fullscreen GUI window for display on the screen.
 namedWindow(windowName, WINDOW_NORMAL);
 setWindowProperty(windowName, PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
```

## Hiding the mouse cursor

You might notice the mouse cursor is shown  on top of your window even though you don't want to use a mouse in your  embedded system. To hide the mouse cursor, you can use the xdotool command to move it to the bottom-right corner pixel, so it's not noticeable,  but is still available if you want to occasionally plug in your mouse to debug the device. Install xdotool and create a short Linux script to run it with Cartoonifier:

```
sudo apt-get install -y xdotool
 cd ~/Cartoonifier/build
```

After installing xdotool, now is the time to create the script, create a new file with your favorite editor with the name runCartoonifier.sh and the following content:

```
 #!/bin/sh
 # Move the mouse cursor to the screen's bottom-right pixel.
 xdotoolmousemove 3000 3000
 # Run Cartoonifier with any arguments given.
 /home/pi/Cartoonifier/build/Cartoonifier "$@"
```

Finally, make your script executable:

```
chmod +x runCartoonifier.sh
```

Try running your script to make sure it works:

```
DISPLAY=:0 ./runCartoonifier.sh
```

## Running Cartoonifier automatically after bootup

Often, when you build an embedded device,  you want your application to be executed automatically after the device  has booted up, rather than requiring the user to manually run your  application. To automatically run our application after the device has  fully booted up and logged in to the graphical desktop, create an autostart folder with a file in it with these contents, including the full path to your script or application:

```
mkdir ~/.config/autostart
 nano ~/.config/autostart/Cartoonifier.desktop
 [Desktop Entry]
 Type=Application
 Exec=/home/pi/Cartoonifier/build/runCartoonifier.sh
 X-GNOME-Autostart-enabled=true
```

Now, whenever you turn the device on or reboot it, Cartoonifier will begin running!

## Speed comparison of Cartoonifier on desktop versus embedded

You will notice that the code runs much  slower on Raspberry Pi than on your desktop! By far the two easiest ways to run it faster are to use a faster device or use a smaller camera  resolution. The following table shows some frame rates, **frames per seconds** (**FPS**), for both the s*ketch* and *paint* modes of Cartoonifier on a desktop, Raspberry Pi 1, Raspberry Pi 2, Raspberry Pi 3, and Jetson TK1. Note that the speeds don't have any custom  optimizations and only run on a single CPU core, and the timings include the time for rendering images to the screen. The USB webcam used is the fast PS3 Eye webcam running at 640 x 480 since it is the fastest  low-cost webcam on the market.

It's worth mentioning that Cartoonifier is  only using a single CPU core, but all the devices listed have four CPU  cores except for Raspberry Pi 1, which has a single core, and many x86  computers have hyperthreading to give roughly eight CPU cores. So, if  you wrote your code to efficiently make use of multiple CPU cores (or  GPU), the speeds might be 1.5 to 3 times faster than the single-threaded figures shown:

| **Computer**      | **Sketch mode** | **Paint mode**             |
| ----------------- | --------------- | -------------------------- |
| Intel Core i7 PC  | 20 FPS          | 2.7 FPS                    |
| Jetson TK1ARM CPU | 16 FPS          | 2.3 FPS                    |
| Raspberry Pi 3    | 4.3 FPS         | 0.32 FPS (3 seconds/frame) |
| Raspberry Pi 2    | 3.2 FPS         | 0.28 FPS (4 seconds/frame) |
| Raspberry Pi Zero | 2.5 FPS         | 0.21 FPS (5 seconds/frame) |
| Raspberry Pi 1    | 1.9 FPS         | 0.12 FPS (8 seconds/frame) |

 

Notice that Raspberry Pi is extremely slow at running the code, especially the *paint* mode, so we will try simply changing the camera and the resolution of the camera.

## Changing the camera and camera resolution

The following table shows how the speed of the *sketch* mode compares on Raspberry Pi 2 using different types of cameras and different camera resolutions:

| **Hardware**                         | **640 x 480 resolution** | **320 x 240 resolution** |
| ------------------------------------ | ------------------------ | ------------------------ |
| Raspberry Pi 2 with Raspberry Pi Cam | 3.8 FPS                  | 12.9 FPS                 |
| Raspberry Pi 2 with PS3 Eye webcam   | 3.2 FPS                  | 11.7 FPS                 |
| Raspberry Pi 2 with unbranded webcam | 1.8 FPS                  | 7.4 FPS                  |

 

As you can see, when using the Raspberry Pi  Cam in 320 x 240, it seems we have a good enough solution to have some  fun, even if it's not in the 20-30 FPS range that we would prefer.

## Power draw of Cartoonifier running on desktop versus embedded system

We've seen that various embedded devices are slower than desktops, from the Raspberry Pi 1 being roughly 20 times  slower than a desktop, up to Jetson TK1 being roughly 1.5 times slower  than a desktop. But for some tasks, low speed is acceptable if it means  there will also be significantly lower battery draw, allowing for small  batteries or low year-round electricity costs for a server, or low heat  generation.

Raspberry Pi has different models even for  the same processor, such as Raspberry Pi 1B, Zero, and 1A+, which all  run at similar speeds but have significantly different power draws. MIPI CSI cameras such as the Raspberry Pi Cam also use less electricity than webcams. The following table shows how much electrical power is used by different hardware running the same Cartoonifier code. Power  measurements of Raspberry Pi were performed as shown in the following  photo using a simple USB current monitor (for example, J7-T Safety  Tester (http://bit.ly/2aSZa6H) for $5) and a DMM multimeter for the other devices:



![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/59bebd50-6d37-4cf6-afec-bfa24bbba598.png)

**Idle power** measures power when the computer is running but no major applications are being used, whereas **Cartoonifier power** measures power when Cartoonifier is running. **Efficiency** is Cartoonifier power/Cartoonifier speed in a 640 x 480 *sketch* mode:

| **Hardware**                   | **Idle power** | **Cartoonifier power** | **Efficiency**          |
| ------------------------------ | -------------- | ---------------------- | ----------------------- |
| Raspberry Pi Zero with PS3 Eye | 1.2 Watts      | 1.8 Watts              | 1.4 Frames per Watt     |
| Raspberry Pi 1A+ with PS3 Eye  | **1.1 Watts**  | **1.5 Watts**          | 1.1 Frames per Watt     |
| Raspberry Pi 1B with PS3 Eye   | 2.4 Watts      | 3.2 Watts              | 0.5 Frames per Watt     |
| Raspberry Pi 2B with PS3 Eye   | 1.8 Watts      | 2.2 Watts              | 1.4 Frames per Watt     |
| Raspberry Pi 3B with PS3 Eye   | 2.0 Watts      | 2.5 Watts              | 1.7 Frames per Watt     |
| Jetson TK1 with PS3 Eye        | 2.8 Watts      | 4.3 Watts              | **3.7 Frames per Watt** |
| Core i7 laptop with PS3 Eye    | 14.0 Watts     | 39.0 Watts             | 0.5 Frames per Watt     |

 

We can see that Raspberry Pi 1A+ uses the  least power, but the most power efficient options are Jetson TK1 and  Raspberry Pi 3B. Interestingly, the original Raspberry Pi (Raspberry Pi  1B) has roughly the same efficiency as an x86 laptop. All later  Raspberry Pis are significantly more power efficient than the original  (Raspberry Pi 1B).

**Disclaimer**: The author is a former employee of NVIDIA, which produced the Jetson TK1,  but the results and conclusions are believed to be authentic.

Let's also look at the power draw of different cameras that work with Raspberry Pi:

| **Hardware**                                 | **Idle power** | **Cartoonifier power** | **Efficiency**          |
| -------------------------------------------- | -------------- | ---------------------- | ----------------------- |
| Raspberry Pi Zero with PS3 Eye               | 1.2 Watts      | 1.8 Watts              | 1.4 Frames per Watt     |
| Raspberry Pi Zero with Raspberry Pi Cam v1.3 | 0.6 Watts      | 1.5 Watts              | 2.1 Frames per Watt     |
| Raspberry Pi Zero with Raspberry Pi Cam v2.1 | **0.55 Watts** | **1.3 Watts**          | **2.4 Frames per Watt** |

 

We see that Raspberry Pi Cam v2.1 is  slightly more power efficient than Raspberry Pi Cam v1.3 and  significantly more power efficient than a USB webcam.

## Streaming video from Raspberry Pi to a powerful computer

Thanks to the hardware-accelerated video  encoders in all modern ARM devices, including Raspberry Pi, a valid  alternative to performing computer vision on board an embedded device is to use the device to just capture video and stream it across a network  in real time to a PC or server rack. All Raspberry Pi models contain the same video encoder hardware, so an Raspberry Pi 1A+ or Raspberry Pi  Zero with a Pi Cam is quite a good option for a low-cost, low-power  portable video streaming server. Raspberry Pi 3 adds Wi-Fi for  additional portable functionality.

There are numerous ways live camera video  can be streamed from a Raspberry Pi, such as using the official  Raspberry Pi V4L2 camera driver to allow the Raspberry Pi Cam to appear  like a webcam, then using GStreamer, liveMedia, netcat, or VLC to stream the video across a network. However, these methods often introduce one  or two seconds of latency and often require customizing the OpenCV  client code or learning how to use GStreamer efficiently. So instead,  the following section will show how to perform both the camera capture  and network streaming using an alternative camera driver named **UV4L**:

1. Install UV4L on the Raspberry Pi by following the instructions at http://www.linux-projects.org/uv4l/installation/:

```
curl http://www.linux-projects.org/listing/uv4l_repo/lrkey.asc
 sudo apt-key add -
 sudo su
 echo "# UV4L camera streaming repo:">> /etc/apt/sources.list
 echo "deb http://www.linux-
 projects.org/listing/uv4l_repo/raspbian/jessie main">>
 /etc/apt/sources.list
 exit
 sudo apt-get update
 sudo apt-get install uv4l uv4l-raspicam uv4l-server
```

1. Run the UV4L streaming server manually (on the Raspberry Pi) to check that it works:

```
sudo killall uv4l
 sudo LD_PRELOAD=/usr/lib/uv4l/uv4lext/armv6l/libuv4lext.so
 uv4l -v7 -f --sched-rr --mem-lock --auto-video_nr
 --driverraspicam --encoding mjpeg
 --width 640 --height 480 --framerate15
```

1. Test the camera's network stream from your desktop, following these steps to check all is working fine: 
   - Install VLC Media Player.
   - Navigate to Media | Open Network Stream and enter http://192.168.2.111:8080/stream/video.mjpeg.
   - Adjust the URL to the IP address of your Raspberry Pi. Run hostname -I on Raspberry Pi to find its IP address.
2. Run the UV4L server automatically on bootup:

```
sudo apt-get install uv4l-raspicam-extras
```

1. Edit any UV4L server settings you want in uv4l-raspicam.conf, such as resolution and frame rate to customize the streaming:

```
sudo nano /etc/uv4l/uv4l-raspicam.conf
 drop-bad-frames = yes
 nopreview = yes
 width = 640
 height = 480
 framerate = 24
```

You will need to reboot to make all changes take effect.

1. Tell OpenCV to use our network stream as if it was a webcam. As long as your installation of OpenCV can use FFMPEG internally, OpenCV will  be able to grab frames from an MJPEG network stream just like a webcam:

```
./Cartoonifier http://192.168.2.101:8080/stream/video.mjpeg
```

Your Raspberry Pi is now using UV4L to stream the live 640 x 480 24 FPS video to a PC that is running Cartoonifier in *sketch* mode, achieving roughly 19 FPS (with 0.4 seconds of latency). Notice this is  almost the same speed as using the PS3 Eye webcam directly on the PC (20 FPS)!

Note that when you are streaming the video  to OpenCV, it won't be able to set the camera resolution; you need to  adjust the UV4L server settings to change the camera resolution. Also  note that instead of streaming MJPEG, we could have streamed H.264  video, which uses a lower bandwidth, but some computer vision algorithms don't handle video compression such as H.264 very well, so MJPEG will  cause fewer algorithm problems than H.264.

If you have both the official Raspberry Pi V4L2 driver and the UV4L driver installed, they will both be available as cameras 0 and 1 (devices /dev/video0 and /dev/video1), but you can only use one camera driver at a time.

## Customizing your embedded system!

Now that you have created a whole embedded  Cartoonifier system, and you know the basics of how it works and which  parts do what, you should customize it! Make the video full screen,  change the GUI, change the application behavior and workflow, change the Cartoonifier filter constants or the skin detector algorithm, replace  the Cartoonifier code with your own project ideas, or stream the video  to the cloud and process it there!

You can improve the skin detection algorithm in many ways, such as using a more complex skin detection algorithm  (for example, using trained Gaussian models from many recent CVPR or  ICCV conference papers at [http://www.cvpapers.com](http://www.cvpapers.com/)), or add face detection (see the *Face detection* section of [Chapter 5](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/86c5b037-c45d-4622-9131-078fca1cf397.xhtml), *Face Detection and Recognition with the DNN Module*) to the skin detector, so it detects where the user's face is, rather  than asking the user to put their face in the center of the screen. Be  aware that face detection may take many seconds on some devices or  high-resolution cameras, so they may be limited in their current  real-time uses. But embedded system platforms are getting faster every  year, so this may be less of a problem over time.

The most significant way to speed up  embedded computer vision applications is to reduce the camera resolution absolutely as much as possible (for example, 0.5 megapixels instead of 5 megapixels), allocate and free images as rarely as possible, and  perform image format conversions as rarely as possible. In some cases,  there might be some optimized image processing or math libraries, or an  optimized version of OpenCV from the CPU vendor of your device (for  example, Broadcom, NVIDIA Tegra, Texas Instruments OMAP, or Samsung  Exynos), or for your CPU family (for example, ARM Cortex-A9).

## Summary

This chapter has shown several different  types of image processing filters that can be used to generate various  cartoon effects, from a plain sketch mode that looks like a pencil  drawing, a paint mode that looks like a color painting, to a cartoon  mode that overlays the *sketch* mode on top of the paint mode to  appear like a cartoon. It also shows that other fun effects can be  obtained, such as the evil mode, which greatly enhanced noisy edges and  the alien mode, which changed the skin of a face to appear bright green.

There are many commercial smartphone apps  that add similar fun effects on the user's face, such as cartoon filters and skin color changes. There are also professional tools using similar concepts, such as skin-smoothing video post-processing tools that  attempt to beautify women's faces by smoothing their skin while keeping  the edges and non-skin regions sharp, in order to make their faces  appear younger.

This chapter shows how to port the  application from a desktop to an embedded system by following the  recommended guidelines of developing a working desktop version first,  and then porting it to an embedded system and creating a user interface  that is suitable for the embedded application. The image processing code is shared between the two projects so that the reader can modify the  cartoon filters for the desktop application, and easily see those  modifications in the embedded system as well.

Remember that this book includes an OpenCV installation script for Linux and full source code for all projects discussed.

In the next chapter, we are going to learn how to use **multiple view stereo** (**MVS**) and **structure from motion** (**SfM**) for 3D reconstruction, and how to export the final result in OpenMVG format.

# Chapter 02 Explore Structure from Motion with the SfM Module

# Chapter 03 Face Landmark and Pose with the Face Module

# Chapter 04 Number Plate Recognition with Deep Convolutional Networks

# Chapter 05 Face Detection and Recognition with the DNN Module

# Chapter 06 Introduction to Web Computer Vision with OpenCV.js

# Chapter 07 Android Camera Calibration and AR Using ArUco Module



# Chapter 8 iOS Panoramas with the Stitching Module

Panoramic imaging has existed since the early days of photography. In those ancient times, roughly 150 years ago, it was called the art of **panography**, carefully putting together individual images using tape or glue to  recreate a panoramic view. With the advancement of computer vision,  panorama stitching became a handy tool in almost all digital cameras and mobile devices. Nowadays, creating panoramas is as simple as swiping  the device or camera across the view, the stitching calculations happen  immediately, and the final expanded scene is available for viewing. In  this chapter, we will implement a modest panoramic image stitching  application on the iPhone using OpenCV's precompiled library for iOS. We will first examine a little of the math and theory behind image  stitching, choose the relevant OpenCV functions to implement it, and  finally integrate it into an iOS app with a basic UI.

The following topics will be covered in this chapter:

- Introduction to the concept of image stitching and panorama building
- OpenCV's image stitching module and its functions
- Building a Swift iOS application UI for panorama capturing
- Integrating OpenCV component written in Objective C++ with the Swift application

## Technical requirements

The following technologies and installations are required to recreate the contents of this chapter:

- macOSX machine (for example, MacBook, iMac) running macOS High Sierra v10.13+
- iPhone 6+ running iOS v11+
- Xcode v9+
- CocoaPods v1.5+: https://cocoapods.org/
- OpenCV v4.0 installed via CocoaPods

Build instructions for the preceding components, as well as the code  to implement the concepts presented in this chapter, will be provided in the accompanying code repository.

The code for this chapter can be accessed via GitHub: https://github.com/PacktPublishing/Mastering-OpenCV-4-Third-Edition/tree/master/Chapter_08.

## Panoramic image stitching methods

Panoramas are essentially multiple images fused together into a  single image. The process of panorama creation from multiple images  involves many steps; some are common to other computer vision tasks,  such as the following:

- Extracting 2D features
- Matching pairs of images based on their features
- Transforming or warping images to a communal frame 
- Using (blending) the seams between the images for the pleasing continuous effect of a larger image

Some of these basic operations are also commonplace in **Structure-from-Motion** (**SfM**), **3D reconstruction** , **visual odometry**, and **simultaneous localization and mapping** (**SLAM**). We've already discussed some of these in [Chapter 2](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/3229555f-ce7e-4330-9197-9cc4169f230f.xhtml), *Explore Structure from Motion with the SfM Module* and [Chapter 7](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/9a5f3e9a-3065-494b-a893-2139e504925a.xhtml), *Android Camera Calibration and AR Using the ArUco Module*. The following is a rough image of the panorama creation process:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/99415d7a-f1d5-4fcd-9506-57e08f93cb42.png)

In this section, we will briefly review feature matching, camera pose estimation, and image warping. In reality, panorama stitching has  multiple pathways and classes, depending on the type of input and  required output. For example, if the camera has a fisheye lens (with an  extremely high degree view angle) a special process is needed.

## Feature extraction and robust matching for panoramas

We create panoramas from overlapping images. In the overlapping region, we look for common visual features that **register** (align) the two images together. In SfM or SLAM, we do this on a frame-by-frame basis, looking for matching features in a real-time video sequence  where the overlap between frames is extremely high. However, in  panoramas we get frames with a big motion component between them, where  the overlap might be as low as just 10%-20% of the image. At first, we  extract image features, such as the **scale invariant feature transform (SIFT)**, **speeded up robust features** (**SURF**), **oriented BRIEF** (**ORB**), or another kind of feature, and then match them between the images in  the panorama. Note the SIFT and SURF features are protected by patents  and cannot be used for commercial purposes. ORB is a considered a free  alternative, but not as robust.

The following image shows extracted features and their matching:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/1e56a991-ecca-4474-8d6a-713ef5fa96bf.png)

## Affine constraint

For a robust and meaningful pairwise matching, we often apply a geometric constraint. One such constraint can be an **affine transform**, a transform that allows only for scale, rotation, and translation. In  2D, an affine transform can be modeled in a 2 x 3 matrix:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/997365e9-c351-409a-83c9-164e9c622095.png)

To impose the constraint, we look for an affine transform $\hat{M}$ minimizes the distance (error) between matching points from the left $X_i^L$ and right $X_i^R$ images.

## Random sample consensus (RANSAC)

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

## Homography constraint

While affine transforms are useful for stitching scanned documents (for example, from a flatbed scanner), they cannot be used for stitching photo panoramas.  For stitching photos, we can employ the same process to find a **homography**, a transform between one plane and another, instead of an affine  transform, which has eight degrees of freedom, and is represented in a 3 x 3 matrix as follows:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/fb58daf4-0a86-4cc4-8890-838715adcbeb.png)

Once a proper matching has been found, we can find an ordering of the images to sequence them for the panorama, essentially to understand how the images relate to one another. In most cases, in panoramas the  assumption is that the photographer (camera) is standing still and only  rotating on its axis, sweeping from left to right, for example.  Therefore, the goal is to recover the rotation component between the  camera poses. Homographies can be decomposed to recover rotation, if we  regard the input as purely rotational: ![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/8ff4d431-fad9-4373-8bd1-2f4a3a601d42.png). If we assume the homography was originally composed from the camera intrinsic (calibration), matrix *K,* and a 3 x 3 rotation matrix *R*, we can recover *R* if we know *K*. The intrinsic matrix can be calculated by camera calibration ahead of  time, or can be estimated during the panorama creation process.

## Bundle Adjustment

When a transformation has been achieved *locally* between all photo *pairs*, we can further optimize our solution in a *global* step. This is called the process of **bundle adjustment**, and is widely constructed as a global optimization of all the  reconstruction parameters (camera or image transforms). Global bundle  adjustment is best performed if all the matched points between images  are put in the same coordinate frame, for example, a 3D space, and there are constraints that span more than two images. For example, if a  feature point appears in more than two images in the panorama, it can be useful for *global* optimization, since it involves registering three or more views. 

The goal in most bundle adjustment methods is to minimize the **reconstruction error**. This means, looking to bring the approximate parameters of the views,  for example, camera or image transforms, to values such that the  re-projected 2D points back on the original views will align with  minimal error. This can be expressed mathematically like so:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/0fa28b19-fdb3-4997-b4ec-8f5403fa2852.png)

Where we look for the best camera or image transforms *T*, such that the distance between original point *Xi* and reprojected point *Proj(Tj, Xi)* is minimal. The binary variable *vij* marks whether point *i* can be seen in image *j*, and can contribute to the error. These kinds of optimization problems can be solved with **iterative non-linear least squares** solvers, such as **Levenberg-Marquardt**, since the previous *Proj* function is usually non-linear.

## Warping images for panorama creation

Given that we know the homographies between images, we can apply  their inverse to project all the images on the same plane. However, a  direct warping using the homography ends up with a stretched-out look  if, for example, all the images are projected on the plane of the first  image. In the following image, we can see a stitching of 4 images using *concatenated* homography (perspective) warping, meaning all the images are registered to the plane of the first image, which illustrates the ungainly  stretching:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/2b8a4bb2-30d5-402c-8eab-3d76541bfeb3.png)

To cope with this problem, we think of the panorama as looking at the images from inside a cylinder, where the images are projected on the  wall of the cylinder, and we rotate the camera at the center. To achieve this effect, we first need to warp the images to **cylindrical coordinates**, as if the round wall of the cylinder was undone and flattened to a  rectangle. The following diagram explains the process of cylindrical  warping:

![img](https://learning.oreilly.com/library/view/mastering-opencv-4/9781789533576/assets/4c458080-4517-47c0-a905-04ee9d3872c0.png)

To wrap the image in cylindrical coordinates, we first apply the  inverse of the intrinsic matrix to get the pixel in normalized  coordinates. We now assume the pixel is a point on the surface of the  cylinder, which is parameterized by the height *h* and the angle *θ*. Height *h* essentially corresponds to the *y* coordinate, while the *x* and *z* (which are perpendicular to one another with regards to *y*) exist on a unit circle and therefore correspond to sin*θ* and cos*θ,* respectively. To get the warped image in the same pixel size as the original image, we can apply the intrinsic matrix *K* again; however, we can change the focal length parameter *f*, for example, to affect the output resolution of our panorama.

In the cylindrical warping model, the relationship between the images becomes purely translational, and in fact governed by a single  parameter: *θ**.* To stitch the images in the same plane, we simply need to find the *θ*s, just a single degree of freedom, which is simple compared to finding  eight parameters for the homography between every two consecutive  images. One major drawback of the cylindrical method is that we assume  the camera's rotational axis motion is perfectly aligned with its up  axis, as well as static in its place, which is almost never the case  with handheld cameras. Still, cylindrical panoramas produce highly  pleasing results. Another option for warping is **spherical coordinates**, which allow for more options in stitching the images in both *x* and *y* axes.

## Project overview

This project will include two major parts as follows:

- iOS application to support capturing the panorama
- OpenCV Objective-C++ code for creating the panorama from the images and integrating into the application

The iOS code will mostly be concerned with building the UI,  accessing the camera, and capturing images. Then, we will focus on  getting the images to OpenCV data structures and running the image  stitching functions from the stitch module.

## Setting up an iOS OpenCV project with CocoaPods 

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

## iOS UI for panorama capture

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

## OpenCV stitching in an Objective-C++ wrapper

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

## Summary

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