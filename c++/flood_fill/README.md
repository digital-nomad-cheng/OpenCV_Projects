flood_fill is an hole filling algorithm
In opencv flood fill comes with the following paramers:
`int floodFill(InputOutputArray image, InputOutputArray mask, Point seedPoint, Scalar newVal, Rect* rect=0, Scalar loDiff=Scalar(), Scalar upDiff=Scalar(), int flags=4 )`
+ `image`: Input/output 1- or 3-channel, 8-bit, or floating-point image. It is modified by the function unless the FLOODFILL_MASK_ONLY flag is set in the second variant of the function.
+ `mask`: single cahnnel 8-bit image, 2-pixels taller and 2-pixels wider than `image`. Normally should be initialized to 0 before passed in. If the value of a pixel in `mask` is non-zero then flood fill won't go across it`(x,y) in origin image corresponds to (x+1, y+1) in mask`. On output, pixels in the mask coresponding to filled pixels in the image are set to 1 or to the value specified in `flags`.
+ seedPoint: start point
+ newVal: new value of repainted pixels in image
+ loDiff:
+ upDiff: 
+ rect: output paramer set by the function to the minimum bounding rectangle of the repainted domain
+ `flags`: first 0-8 bits contain a connecitvity type, 4 for (left, right, top, down pixels) and 8 for (surrounding pixels); 8-16 bits contain a value between 1 and 255 to fill the mask(default value is 1). Two additional options occupy heigher bits:
	+ FLOODFILL_PIXED_RANGE: if set, the difference between the current pixel and seed pixel is considered. Otherwise, the difference between neighbor pixels is considered(which means the range is floating beacause the referencing point is changing)
	+ FLOODFILL_MASK_ONLY: if set does not change the image(newVal is ignored)

	example: `4 | (255 << 8) | FLOODFILL_PIXEL_RANGE | FLOODFILL_MASK_ONLY`

## reference
1. https://docs.opencv.org/2.4/modules/imgproc/doc/miscellaneous_transformations.html
