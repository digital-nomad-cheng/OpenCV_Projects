## Chapter 01 

1. isdigit

   ```c
   int isdigit ( int c );
   ```

   Check if character is decimal digit.

2. atoi

   ```c
   int atoi (const char * str);
   ```

   Convert string to integer

3. std::err: standar error stream

4. cv::Laplician() use gray images

5. cv::Scarr() edge detection

6. memset

7. ```
   void * memset ( void * ptr, int value, size_t num );
   ```

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