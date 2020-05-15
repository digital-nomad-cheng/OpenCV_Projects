
#include <iostream>
#include "opencv2/opencv.hpp"
#include <dirent.h>
#include "inpainting.hpp"

namespace colorconstant
{
#define MAX_RETINEX_SCALES    8
#define MIN_GAUSSIAN_SCALE   16
#define MAX_GAUSSIAN_SCALE  250
#define SCALE_WIDTH         150
#define ENTRY_WIDTH           4


#define gint int
#define gfloat float
#define gdouble double
#define guchar unsigned char
#define gboolean bool


#define g_try_malloc malloc
#define g_free free

typedef struct
{
  gint     scale;
  gint     nscales;
  gint     scales_mode;
  gfloat   cvar;
} RetinexParams;

typedef enum
{
  filter_uniform,
  filter_low,
  filter_high
} FilterMode;

/*
  Definit comment sont repartis les
  differents filtres en fonction de
  l'echelle (~= ecart type de la gaussienne)
 */
#define RETINEX_UNIFORM 0
#define RETINEX_LOW     1
#define RETINEX_HIGH    2


#define CLAMP(x, low, high)  ((x) > (low) ? (x) < (high) ? (x) : (high) : (low))

static gfloat RetinexScales[MAX_RETINEX_SCALES];

typedef struct
{
  gint    N;
  gfloat  sigma;
  gdouble B;
  gdouble b[4];
} gauss3_coefs;

/*
 * MSRCR = MultiScale Retinex with Color Restoration
 */
static void     MSRCR                       (guchar       *src,
                                             gint          width,
                                             gint          height,
                                             gint          bytes,
                                             gboolean      preview_mode);


/*
 * Private variables.
 */
static RetinexParams rvals =
{
  250,             /* Scale */
  3,               /* Scales */
  RETINEX_UNIFORM, /* Echelles reparties uniformement */
  2.0              /* A voir */
};

#define byte uchar
const float YCbCrYRF = 0.299F;              // RGB转YCbCr的系数(浮点类型）
const float YCbCrYGF = 0.587F;
const float YCbCrYBF = 0.114F;
const float YCbCrCbRF = -0.168736F;
const float YCbCrCbGF = -0.331264F;
const float YCbCrCbBF = 0.500000F;
const float YCbCrCrRF = 0.500000F;
const float YCbCrCrGF = -0.418688F;
const float YCbCrCrBF = -0.081312F;

const float RGBRYF = 1.00000F;            // YCbCr转RGB的系数(浮点类型）
const float RGBRCbF = 0.0000F;
const float RGBRCrF = 1.40200F;
const float RGBGYF = 1.00000F;
const float RGBGCbF = -0.34414F;
const float RGBGCrF = -0.71414F;
const float RGBBYF = 1.00000F;
const float RGBBCbF = 1.77200F;
const float RGBBCrF = 0.00000F;

const int Shift = 20;
const int HalfShiftValue = 1 << (Shift - 1);

const int YCbCrYRI = (int)(YCbCrYRF * (1 << Shift) + 0.5);         // RGB转YCbCr的系数(整数类型）
const int YCbCrYGI = (int)(YCbCrYGF * (1 << Shift) + 0.5);
const int YCbCrYBI = (int)(YCbCrYBF * (1 << Shift) + 0.5);
const int YCbCrCbRI = (int)(YCbCrCbRF * (1 << Shift) + 0.5);
const int YCbCrCbGI = (int)(YCbCrCbGF * (1 << Shift) + 0.5);
const int YCbCrCbBI = (int)(YCbCrCbBF * (1 << Shift) + 0.5);
const int YCbCrCrRI = (int)(YCbCrCrRF * (1 << Shift) + 0.5);
const int YCbCrCrGI = (int)(YCbCrCrGF * (1 << Shift) + 0.5);
const int YCbCrCrBI = (int)(YCbCrCrBF * (1 << Shift) + 0.5);

const int RGBRYI = (int)(RGBRYF * (1 << Shift) + 0.5);              // YCbCr转RGB的系数(整数类型）
const int RGBRCbI = (int)(RGBRCbF * (1 << Shift) + 0.5);
const int RGBRCrI = (int)(RGBRCrF * (1 << Shift) + 0.5);
const int RGBGYI = (int)(RGBGYF * (1 << Shift) + 0.5);
const int RGBGCbI = (int)(RGBGCbF * (1 << Shift) + 0.5);
const int RGBGCrI = (int)(RGBGCrF * (1 << Shift) + 0.5);
const int RGBBYI = (int)(RGBBYF * (1 << Shift) + 0.5);
const int RGBBCbI = (int)(RGBBCbF * (1 << Shift) + 0.5);
const int RGBBCrI = (int)(RGBBCrF * (1 << Shift) + 0.5);
void ToYCbCr(byte* From, byte* To, int Length = 1)
{
    if (Length < 1) return;
    byte* End = From + Length * 3;
    int Red, Green, Blue;
    // int Y, Cb, Cr;
    while (From != End)
    {
        Blue = *From; Green = *(From + 1); Red = *(From + 2);
        // 无需判断是否存在溢出，因为测试过整个RGB空间的所有颜色值，无颜色存在溢出
        *To = (byte)((YCbCrYRI * Red + YCbCrYGI * Green + YCbCrYBI * Blue + HalfShiftValue) >> Shift);
        *(To + 1) = (byte)( 128 + ( (YCbCrCbRI * Red + YCbCrCbGI * Green + YCbCrCbBI * Blue + HalfShiftValue) >> Shift));
        *(To + 2) = (byte) (  128 + ( (YCbCrCrRI * Red + YCbCrCrGI * Green + YCbCrCrBI * Blue + HalfShiftValue) >> Shift));
       // *To = (byte)Y;          // 不要把直接计算的代码放在这里，会降低速度，
        //*(To + 1) = (byte)Cb;
        //*(To + 2) = (byte)Cr;
        From += 3;
        To += 3;
    }
}

cv::Mat combineImages(std::vector<cv::Mat>& imgs,//@parameter1:需要显示的图像组
                  int col,//parameter2:显示的列数
                  int row, //parameter3:显示的行数
                  bool hasMargin){//parameter4:是否设置边框
    int imgAmount = imgs.size();//获取需要显示的图像数量
    int width = imgs[0].cols;//本函数默认需要显示的图像大小相同
    int height = imgs[0].rows;//获取图像宽高
    int newWidth, newHeight;//新图像宽高
    if (!hasMargin){
        newWidth = col*imgs[0].cols;//无边框，新图像宽/高=原图像宽/高*列/行数
        newHeight = row*imgs[0].rows;
    }
    else{
        newWidth = (col + 1) * 20 + col*width;//有边框，要将上边框的尺寸，这里设置边框为20px
        newHeight = (row + 1) * 20 + row*height;
    }

    cv::Mat newImage(newHeight, newWidth, CV_8UC3, cvScalar(255, 255, 255));//显示创建设定尺寸的新的大图像；色深八位三通道；填充为白色


    int x, y,imgCount;//x列号，y行号，imgCount图片序号
    if (hasMargin){//有边框
        imgCount = 0;
        x = 0; y = 0;
        while (imgCount < imgAmount){
            cv::Mat imageROI = newImage(cv::Rect(x*width + (x + 1) * 20, y*height + (y + 1) * 20, width, height));//创建感兴趣区域
            imgs[imgCount].copyTo(imageROI);//将图像复制到大图中
            imgCount++;
            if (x == (col - 1)){
                x = 0;
                y++;
            }
            else{
                x++;
            }//移动行列号到下一个位置
        }
    }
    else{//无边框
        imgCount = 0;
        x = 0; y = 0;
        while (imgCount < imgAmount){
            cv::Mat imageROI = newImage(cv::Rect(x*width, y*height, width, height));
            imgs[imgCount].copyTo(imageROI);
            imgCount++;
            if (x == (col - 1)){
                x = 0;
                y++;
            }
            else{
                x++;
            }
        }
    }
    return newImage;//返回新的组合图像
};

/*
 * calculate scale values for desired distribution.
 */
static void
retinex_scales_distribution(gfloat* scales, gint nscales, gint mode, gint s)
{
  if (nscales == 1)
    { /* For one filter we choose the median scale */
      scales[0] = (gint) s / 2;
    }
  else if (nscales == 2)
    { /* For two filters whe choose the median and maximum scale */
      scales[0] = (gint) s / 2;
      scales[1] = (gint) s;
    }
  else
    {
      gfloat size_step = (gfloat) s / (gfloat) nscales;
      gint   i;

      switch(mode)
        {
        case RETINEX_UNIFORM:
          for(i = 0; i < nscales; ++i)
            scales[i] = 2. + (gfloat) i * size_step;
          break;

        case RETINEX_LOW:
          size_step = (gfloat) log(s - 2.0) / (gfloat) nscales;
          for (i = 0; i < nscales; ++i)
            scales[i] = 2. + pow (10, (i * size_step) / log (10));
          break;

        case RETINEX_HIGH:
          size_step = (gfloat) log(s - 2.0) / (gfloat) nscales;
          for (i = 0; i < nscales; ++i)
            scales[i] = s - pow (10, (i * size_step) / log (10));
          break;

        default:
          break;
        }
    }
}

/*
 * Calculate the coefficients for the recursive filter algorithm
 * Fast Computation of gaussian blurring.
 */
static void
compute_coefs3 (gauss3_coefs *c, gfloat sigma)
{
  /*
   * Papers:  "Recursive Implementation of the gaussian filter.",
   *          Ian T. Young , Lucas J. Van Vliet, Signal Processing 44, Elsevier 1995.
   * formula: 11b       computation of q
   *          8c        computation of b0..b1
   *          10        alpha is normalization constant B
   */
  gfloat q, q2, q3;

  q = 0;

  if (sigma >= 2.5)
    {
      q = 0.98711 * sigma - 0.96330;
    }
  else if ((sigma >= 0.5) && (sigma < 2.5))
    {
      q = 3.97156 - 4.14554 * (gfloat) sqrt ((double) 1 - 0.26891 * sigma);
    }
  else
    {
      q = 0.1147705018520355224609375;
    }

  q2 = q * q;
  q3 = q * q2;
  c->b[0] = (1.57825+(2.44413*q)+(1.4281 *q2)+(0.422205*q3));
  c->b[1] = (        (2.44413*q)+(2.85619*q2)+(1.26661 *q3));
  c->b[2] = (                   -((1.4281*q2)+(1.26661 *q3)));
  c->b[3] = (                                 (0.422205*q3));
  c->B = 1.0-((c->b[1]+c->b[2]+c->b[3])/c->b[0]);
  c->sigma = sigma;
  c->N = 3;

/*
  g_printerr ("q %f\n", q);
  g_printerr ("q2 %f\n", q2);
  g_printerr ("q3 %f\n", q3);
  g_printerr ("c->b[0] %f\n", c->b[0]);
  g_printerr ("c->b[1] %f\n", c->b[1]);
  g_printerr ("c->b[2] %f\n", c->b[2]);
  g_printerr ("c->b[3] %f\n", c->b[3]);
  g_printerr ("c->B %f\n", c->B);
  g_printerr ("c->sigma %f\n", c->sigma);
  g_printerr ("c->N %d\n", c->N);
*/
}

static void
gausssmooth (gfloat *in, gfloat *out, gint size, gint rowstride, gauss3_coefs *c)
{
  /*
   * Papers:  "Recursive Implementation of the gaussian filter.",
   *          Ian T. Young , Lucas J. Van Vliet, Signal Processing 44, Elsevier 1995.
   * formula: 9a        forward filter
   *          9b        backward filter
   *          fig7      algorithm
   */
  gint i,n, bufsize;
  gfloat *w1,*w2;

  /* forward pass */
  bufsize = size+3;
  size -= 1;
  w1 = (gfloat *) g_try_malloc (bufsize * sizeof (gfloat));
  w2 = (gfloat *) g_try_malloc (bufsize * sizeof (gfloat));
  w1[0] = in[0];
  w1[1] = in[0];
  w1[2] = in[0];
  for ( i = 0 , n=3; i <= size ; i++, n++)
    {
      w1[n] = (gfloat)(c->B*in[i*rowstride] +
                       ((c->b[1]*w1[n-1] +
                         c->b[2]*w1[n-2] +
                         c->b[3]*w1[n-3] ) / c->b[0]));
    }

  /* backward pass */
  w2[size+1]= w1[size+3];
  w2[size+2]= w1[size+3];
  w2[size+3]= w1[size+3];
  for (i = size, n = i; i >= 0; i--, n--)
    {
      w2[n]= out[i * rowstride] = (gfloat)(c->B*w1[n] +
                                           ((c->b[1]*w2[n+1] +
                                             c->b[2]*w2[n+2] +
                                             c->b[3]*w2[n+3] ) / c->b[0]));
    }

  g_free (w1);
  g_free (w2);
}

/*
 * Calculate the average and variance in one go.
 */
static void
compute_mean_var (gfloat *src, gfloat *mean, gfloat *var, gint size, gint bytes)
{
  gfloat vsquared;
  gint i,j;
  gfloat *psrc;

  vsquared = 0;
  *mean = 0;
  for (i = 0; i < size; i+= bytes)
    {
       psrc = src+i;
       for (j = 0 ; j < 3 ; j++)
         {
            *mean += psrc[j];
            vsquared += psrc[j] * psrc[j];
         }
    }

  *mean /= (gfloat) size; /* mean */
  vsquared /= (gfloat) size; /* mean (x^2) */
  *var = ( vsquared - (*mean * *mean) );
  *var = sqrt(*var); /* var */
}

/*
 * This function is the heart of the algo.
 * (a)  Filterings at several scales and sumarize the results.
 * (b)  Calculation of the final values.
 */
static void
MSRCR (guchar *src, gint width, gint height, gint bytes, gboolean preview_mode)
{

  gint          scale,row,col;
  gint          i,j;
  gint          size;
  gint          pos;
  gint          channel;
  guchar       *psrc = NULL;            /* backup pointer for src buffer */
  gfloat       *dst  = NULL;            /* float buffer for algorithm */
  gfloat       *pdst = NULL;            /* backup pointer for float buffer */
  gfloat       *in, *out;
  gint          channelsize;            /* Float memory cache for one channel */
  gfloat        weight;
  gauss3_coefs  coef;
  gfloat        mean, var;
  gfloat        mini, range, maxi;
  gfloat        alpha;
  gfloat        gain;
  gfloat        offset;
  gdouble       max_preview = 0.0;

  if (!preview_mode)
    {
      max_preview = 3 * rvals.nscales;
    }

  /* Allocate all the memory needed for algorithm*/
  size = width * height * bytes;
  dst = (gfloat *)g_try_malloc (size * sizeof (gfloat));
  if (dst == NULL)
    {
      printf("Failed to allocate memory");
      return;
    }
  memset (dst, 0, size * sizeof (gfloat));

  channelsize  = (width * height);
  in  = (gfloat *) g_try_malloc (channelsize * sizeof (gfloat));
  if (in == NULL)
    {
      g_free (dst);
      printf ("Failed to allocate memory");
      return; /* do some clever stuff */
    }

  out  = (gfloat *) g_try_malloc (channelsize * sizeof (gfloat));
  if (out == NULL)
    {
      g_free (in);
      g_free (dst);
      printf ("Failed to allocate memory");
      return; /* do some clever stuff */
    }


  /*
     Calculate the scales of filtering according to the
     number of filter and their distribution.
   */

  retinex_scales_distribution (RetinexScales,
                               rvals.nscales, rvals.scales_mode, rvals.scale);

  /*
      Filtering according to the various scales.
      Summerize the results of the various filters according to a
      specific weight(here equivalent for all).
  */
  weight = 1./ (gfloat) rvals.nscales;

  /*
    The recursive filtering algorithm needs different coefficients according
    to the selected scale (~ = standard deviation of Gaussian).
   */
  pos = 0;
  for (channel = 0; channel < 3; channel++)
    {
      for (i = 0, pos = channel; i < channelsize ; i++, pos += bytes)
         {
            /* 0-255 => 1-256 */
            in[i] = (gfloat)(src[pos] + 1.0);
         }
      for (scale = 0; scale < rvals.nscales; scale++)
        {
          compute_coefs3 (&coef, RetinexScales[scale]);
          /*
           *  Filtering (smoothing) Gaussian recursive.
           *
           *  Filter rows first
           */
          for (row=0 ;row < height; row++)
            {
              pos =  row * width;
              gausssmooth (in + pos, out + pos, width, 1, &coef);
            }

          memcpy(in,  out, channelsize * sizeof(gfloat));
          memset(out, 0  , channelsize * sizeof(gfloat));

          /*
           *  Filtering (smoothing) Gaussian recursive.
           *
           *  Second columns
           */
          for (col=0; col < width; col++)
            {
              pos = col;
              gausssmooth(in + pos, out + pos, height, width, &coef);
            }

          /*
             Summarize the filtered values.
             In fact one calculates a ratio between the original values and the filtered values.
           */
          for (i = 0, pos = channel; i < channelsize; i++, pos += bytes)
            {
              dst[pos] += weight * (log (src[pos] + 1.) - log (out[i]));
            }

//           if (!preview_mode)
//             gimp_progress_update ((channel * rvals.nscales + scale) /
//                                   max_preview);
        }
    }
  g_free(in);
  g_free(out);

  /*
      Final calculation with original value and cumulated filter values.
      The parameters gain, alpha and offset are constants.
  */
  /* Ci(x,y)=log[a Ii(x,y)]-log[ Ei=1-s Ii(x,y)] */

  alpha  = 128.;
  gain   = 1.;
  offset = 0.;

  for (i = 0; i < size; i += bytes)
    {
      gfloat logl;

      psrc = src+i;
      pdst = dst+i;

      logl = log((gfloat)psrc[0] + (gfloat)psrc[1] + (gfloat)psrc[2] + 3.);

      pdst[0] = gain * ((log(alpha * (psrc[0]+1.)) - logl) * pdst[0]) + offset;
      pdst[1] = gain * ((log(alpha * (psrc[1]+1.)) - logl) * pdst[1]) + offset;
      pdst[2] = gain * ((log(alpha * (psrc[2]+1.)) - logl) * pdst[2]) + offset;
    }

/*  if (!preview_mode)
    gimp_progress_update ((2.0 + (rvals.nscales * 3)) /
                          ((rvals.nscales * 3) + 3));*/

  /*
      Adapt the dynamics of the colors according to the statistics of the first and second order.
      The use of the variance makes it possible to control the degree of saturation of the colors.
  */
  pdst = dst;

  compute_mean_var (pdst, &mean, &var, size, bytes);
  mini = mean - rvals.cvar*var;
  maxi = mean + rvals.cvar*var;
  range = maxi - mini;

  if (!range)
    range = 1.0;

  for (i = 0; i < size; i+= bytes)
    {
      psrc = src + i;
      pdst = dst + i;

      for (j = 0 ; j < 3 ; j++)
        {
          gfloat c = 255 * ( pdst[j] - mini ) / range;

          psrc[j] = (guchar) CLAMP (c, 0, 255);
        }
    }

  g_free (dst);
}

void color_balance2(cv::Mat *img)
{
    int histo[256] = { 0 };//直方图统计每个像素值的数目
    int num_of_pixels = img->cols*img->rows;
    //统计每个像素值的数目
    for (int y = 0; y < img->rows; ++y)
    {
        uchar *data = (uchar*)(img->data + y*img->step);//定义的大小和图像尺寸一致
        for (int x = 0; x < img->cols; ++x)
        {
            histo[data[x]] += 1;
        }
    }
 
    //统计当前像素值和之前像素值的总数
    for (int i = 1; i < 256; ++i)
        histo[i] = histo[i] + histo[i - 1];
 
    double s = 0.0265;//此参数可以调整，最好在0.1以下(0=<s<=1)
 
    int vmin = 0;
 
    //统计像素点数目小于num_of_pixels*s / 2的数目，s为控制比率
    while (histo[vmin + 1] <= cvRound(num_of_pixels*s / 2))
        vmin = vmin + 1;
 
    int vmax = 255 - 1;
 
    //统计像素点数目大于num_of_pixels*(1 - s / 2)的数目，s为控制比率
    while (histo[vmax - 1] > cvRound(num_of_pixels*(1 - s / 2)))
    {
        vmax = vmax - 1;
    }
 
    if (vmax < 255 - 1)
        vmax = vmax + 1;
 
    //处理图像中像素值大于vmin和小于vmax的像素，
    //即处理偏亮和偏暗的区域
    for (int y = 0; y < img->rows; ++y)
    {
 
        uchar *data = (uchar*)(img->data + y*img->step);
        for (int x = 0; x < img->cols; ++x)
        {
            if (data[x] < vmin)
                data[x] = vmin;
            if (data[x] > vmax)
                data[x] = vmax;
        }
    }
 
    //对其他的像素进行处理（拉伸），其实可以合并到上一步，简化时间复杂度，这里分开只是为了让过程更清楚
    for (int y = 0; y < img->rows; ++y)
    {
 
        uchar *data = (uchar*)(img->data + y*img->step);
        for (int x = 0; x < img->cols; ++x)
        {
            data[x] = cvRound((data[x] - vmin)*255.0 / (vmax - vmin));
        }
    }
}

cv::Mat GrayWorld(cv::Mat& src) {
    std::vector <cv::Mat> bgr;
  cv::split(src, bgr);
  double B = 0;
  double G = 0;
  double R = 0;
  int row = src.rows;
  int col = src.cols;
    cv::Mat dst(row, col, CV_8UC3);
  for (int i = 0; i < row; i++) {
    for (int j = 0; j < col; j++) {
        B += 1.0 * src.at<cv::Vec3b>(i, j)[0];
        G += 1.0 * src.at<cv::Vec3b>(i, j)[1];
        R += 1.0 * src.at<cv::Vec3b>(i, j)[2];
    }
  }
  B /= (row * col);
  G /= (row * col);
  R /= (row * col);
  printf("%.5f %.5f %.5f\n", B, G, R);
  double GrayValue = (B + G + R) / 3;
  printf("%.5f\n", GrayValue);
  double kr = GrayValue / R;
  double kg = GrayValue / G;
  double kb = GrayValue / B;
  printf("%.5f %.5f %.5f\n", kb, kg, kr);
  for (int i = 0; i < row; i++) {
    for (int j = 0; j < col; j++) {
        dst.at<cv::Vec3b>(i, j)[0] = (int)(kb * src.at<cv::Vec3b>(i, j)[0]) > 255 ? 255 : (int)(kb * src.at<cv::Vec3b>(i, j)[0]);
        dst.at<cv::Vec3b>(i, j)[1] = (int)(kg * src.at<cv::Vec3b>(i, j)[1]) > 255 ? 255 : (int)(kg * src.at<cv::Vec3b>(i, j)[1]);
        dst.at<cv::Vec3b>(i, j)[2] = (int)(kr * src.at<cv::Vec3b>(i, j)[2]) > 255 ? 255 : (int)(kr * src.at<cv::Vec3b>(i, j)[2]);
    }
  }
  return dst;
}

void color_balance(IplImage *img)
{
    int histo[256] = { 0 };//直方图统计每个像素值的数目
    int num_of_pixels = img->width*img->height;
    //统计每个像素值的数目
    for (int y = 0; y < img->height; ++y)
    {
        uchar *data = (uchar*)(img->imageData + y*img->widthStep);//定义的大小和图像尺寸一致
        for (int x = 0; x < img->width; ++x)
        {
            histo[data[x]] += 1;
        }
    }
 
    //统计当前像素值和之前像素值的总数
    for (int i = 1; i < 256; ++i)
        histo[i] = histo[i] + histo[i - 1];
 
    double s = 0.0265;//此参数可以调整，最好在0.1以下(0=<s<=1)
 
    int vmin = 0;
 
    //统计像素点数目小于num_of_pixels*s / 2的数目，s为控制比率
    while (histo[vmin + 1] <= cvRound(num_of_pixels*s / 2))
        vmin = vmin + 1;
 
    int vmax = 255 - 1;
 
    //统计像素点数目大于num_of_pixels*(1 - s / 2)的数目，s为控制比率
    while (histo[vmax - 1] > cvRound(num_of_pixels*(1 - s / 2)))
    {
        vmax = vmax - 1;
    }
 
    if (vmax < 255 - 1)
        vmax = vmax + 1;
 
    //处理图像中像素值大于vmin和小于vmax的像素，
    //即处理偏亮和偏暗的区域
    for (int y = 0; y < img->height; ++y)
    {
 
        uchar *data = (uchar*)(img->imageData + y*img->widthStep);
        for (int x = 0; x < img->width; ++x)
        {
            if (data[x] < vmin)
                data[x] = vmin;
            if (data[x] > vmax)
                data[x] = vmax;
        }
    }
 
    //对其他的像素进行处理（拉伸），其实可以合并到上一步，简化时间复杂度，这里分开只是为了让过程更清楚
    for (int y = 0; y < img->height; ++y)
    {
 
        uchar *data = (uchar*)(img->imageData + y*img->widthStep);
        for (int x = 0; x < img->width; ++x)
        {
            data[x] = cvRound((data[x] - vmin)*255.0 / (vmax - vmin));
        }
    }
}

void showGrayWorld(const char *imgpath) {
    cv::Mat img = cv::imread(imgpath);
    
    // check if image was loaded
    if (img.data) {
        std::cout << "Image loaded" << std::endl;
    }
    else {
        std::cout << "Image not loaded" << std::endl;
    }
    
    char src[50] = "src";
    char dst[50] = "dst";
    cv::imshow(strncat(src, imgpath, 20), img);
    cv::imshow(strncat(dst, imgpath, 20), GrayWorld(img));
    // wait for a key press
    
    char path[256];
    strcpy(path, "./images/dst");
    strncat (path, imgpath, 20);
    imwrite( path, GrayWorld(img));
}

void showMSR(const char *imgpath) {
    cv::Mat img = cv::imread(imgpath);
    
    // check if image was loaded
    if (img.data) {
        std::cout << "Image loaded" << std::endl;
    }
    else {
        std::cout << "Image not loaded" << std::endl;
    }
    
    char src[50] = "src";
    char dst[50] = "dst";
    cv::imshow(strncat(src, imgpath, 20), img);
    // create a window and show the image
    MSRCR (img.data, img.cols, img.rows, 3, false);
    cv::imshow(strncat(dst, imgpath, 20), img);
    
    char path[256];
    strcpy(path, "./images/dst");
    strncat (path, imgpath, 20);
    imwrite( path, img);
    // wait for a key press
}


void showbalace(const char *imgpath) {
    IplImage *srcImg = cvLoadImage(imgpath);//读取图片
        IplImage *dstImg = cvCreateImage(cvGetSize(srcImg), 8, 3);
        IplImage *redCh = cvCreateImage(cvGetSize(srcImg), 8, 1);//R通道
        IplImage *greenCh = cvCreateImage(cvGetSize(srcImg), 8, 1);//G通道
        IplImage *blueCh = cvCreateImage(cvGetSize(srcImg), 8, 1);//B通道
        cvSplit(srcImg, blueCh, greenCh, redCh, NULL);//把原图拆分RGB通道
        color_balance(redCh);//对R通道进行色彩平衡
        color_balance(greenCh);//对G通道进行色彩平衡
        color_balance(blueCh);//对B通道进行色彩平衡
        cvMerge(blueCh, greenCh, redCh, NULL, dstImg);//合并操作后的通道，为最终结果
        
        //显示操作
//    char src[50] = "src";
//    char dst[50] = "dst";
//        cvNamedWindow(strncat(src, imgpath, 20), CV_WINDOW_AUTOSIZE);
//        cvShowImage(src, srcImg);
//
//        cvNamedWindow(strncat(dst, imgpath, 20), CV_WINDOW_AUTOSIZE);
//        cvShowImage(dst, dstImg);
    char path[256];
    strcpy(path, "./images/dst");
    strncat (path, imgpath, 20);
    cvSaveImage(path,dstImg);
}

template<typename T>
inline T sign(T const &input){
    return input >= 0 ? 1 : -1;
}
//动态阈值算法
void showDynamicThresholdMethod(const char *imgpath) {
    cv::Mat input = cv::imread(imgpath);
    cv::Mat tmp = input.clone();
    std::vector<cv::Mat> imgs(2);
    imgs[0] = input.clone();
    if (!tmp.data) {
        return;
    }
    if(input.channels() == 4){
       cv::cvtColor(input, tmp, CV_BGRA2BGR); //去掉alpha通道
    }
//    cv::cvtColor(tmp, input, CV_BGR2YCrCb); //转换到YCrCb的色彩空间，我查过openCV的转换公式，和楼主的如出一辙
                                             //转换后的范围介于0～255
    ToYCbCr(tmp.data, input.data, input.total());
//    cv::Scalar a(0,-128,-128);
//    input = input + a;
    cv::Scalar const global_mean = cv::mean(input);
    double dr = 0;
    double db = 0;
    double ymax = 0;
    for(int i = 0; i != input.rows; ++i){
       uchar *ptr = input.ptr<uchar>(i);
       for(int j = 0; j != input.cols; ++j){
           ymax = fmax(ymax, *ptr++);//to first cr
           
           dr += std::abs(*ptr++ - global_mean[1]);  //to cb
//           std::cout<<(int)*ptr<<" ";
           db += std::abs(*ptr++ - global_mean[2]); //to next cr
       }
    }
    
//    std::cout<<std::endl;
    dr /= input.total();
    db /= input.total();

    //公式（3）与（4）的判断条件
    double const cr_left_criteria = 1.5 * global_mean[1] + dr * sign(global_mean[1]);
    double const cr_right_criteria = 1.5 * dr;
    double const cb_left_criteria = global_mean[2] + db * sign(global_mean[2]);
    double const cb_right_criteria = 1.5 * db;

    std::cout<<"cr criteria = "<<cr_left_criteria<<", "<<cr_right_criteria<<std::endl;
    std::cout<<"cb criteria = "<<cb_left_criteria<<", "<<cb_right_criteria<<std::endl;
    size_t satisfy_number = 0;
    std::vector<std::vector<double> > Y(input.rows,std::vector<double>(input.cols));
    double *Yhistogram = new double[256];
    memset(Yhistogram, 0, 256*sizeof(double));
    for(int i = 0; i != input.rows; ++i){
      uchar const *ptr = input.ptr<uchar>(i);
      for(int j = 0; j != input.cols; ++j){
          uchar const y = *ptr++;
          uchar const cr = *ptr++;
          uchar const cb = *ptr++;
          if(((cr - cb_left_criteria) < cb_right_criteria) && ((cb - cr_left_criteria) < cr_right_criteria)){
              ++satisfy_number;
              Y[i][j] = y;
              Yhistogram[y]++;
          }
      }
    }
    
    double Yhistogramsum = 0;
    double Ymin = 0;
    for (int i = 256 - 1; i >= 0; i--) {
        Yhistogramsum += Yhistogram[i];
        if (Yhistogramsum > 0.05 * satisfy_number) {
            Ymin = i;
            break;
        }
    }
    
    double Raver = 0, Gaver = 0, Baver = 0;
    double averSum = 0;
    for (int i = 0; i < Y.size(); i++) {
        for (int j = 0; j < Y[i].size(); j++) {
            if (Y[i][j] >= Ymin) {
     
                int r = tmp.at<cv::Vec3b>(i, j)[0];
                int g = tmp.at<cv::Vec3b>(i, j)[1];
                int b = tmp.at<cv::Vec3b>(i, j)[2];
                Raver += r;
                Gaver += g;
                Baver += b;
                averSum++;
            }
        }
    }
    Raver /= averSum;
    Gaver /= averSum;
    Baver /= averSum;
    
//    ymax /= 15.0;
    double Rgain = ymax / Raver, Ggain = ymax / Gaver, Bgain = ymax / Baver;
    for (int i = 0; i < tmp.rows; i++) {
      for (int j = 0; j < tmp.cols; j++) {
          tmp.at<cv::Vec3b>(i, j)[0] = (int)(Rgain * tmp.at<cv::Vec3b>(i, j)[0]) > 255 ? 255 : (int)(Rgain * tmp.at<cv::Vec3b>(i, j)[0]);
          tmp.at<cv::Vec3b>(i, j)[1] = (int)(Ggain * tmp.at<cv::Vec3b>(i, j)[1]) > 255 ? 255 : (int)(Ggain * tmp.at<cv::Vec3b>(i, j)[1]);
          tmp.at<cv::Vec3b>(i, j)[2] = (int)(Bgain * tmp.at<cv::Vec3b>(i, j)[2]) > 255 ? 255 : (int)(Bgain * tmp.at<cv::Vec3b>(i, j)[2]);
//          std::cout<<tmp.at<cv::Vec3b>(i, j)<<" ";
      }
    }
    
    char path[256];
    strcpy(path, "./images/dst");
    strncat (path, imgpath, 20);
    imwrite( path, tmp);
    
    delete[] Yhistogram;
    imgs[1] = tmp;
    cv::Mat m = combineImages(imgs, 2, 1, false);
    cv::imshow(imgpath, m);
    std::cout<<"satisfy_number = "<<satisfy_number<<std::endl;
}

 //动态阈值算法
cv::Mat dynamicThresholdMethod(cv::Mat& input) {
    cv::Mat tmp = input.clone();
    if(input.channels() == 4){
       cv::cvtColor(input, tmp, CV_BGRA2BGR); //去掉alpha通道
    }
    ToYCbCr(tmp.data, input.data, input.total());
    cv::Scalar const global_mean = cv::mean(input);
    double dr = 0;
    double db = 0;
    double ymax = 0;
    for(int i = 0; i != input.rows; ++i){
       uchar *ptr = input.ptr<uchar>(i);
       for(int j = 0; j != input.cols; ++j){
           ymax = fmax(ymax, *ptr++);//to first cr
           dr += std::abs(*ptr++ - global_mean[1]);  //to cb
           db += std::abs(*ptr++ - global_mean[2]); //to next cr
       }
    }
    
    dr /= input.total();
    db /= input.total();

    //公式（3）与（4）的判断条件
    double const cr_left_criteria = 1.5 * global_mean[1] + dr * sign(global_mean[1]);
    double const cr_right_criteria = 1.5 * dr;
    double const cb_left_criteria = global_mean[2] + db * sign(global_mean[2]);
    double const cb_right_criteria = 1.5 * db;

    size_t satisfy_number = 0;
    std::vector<std::vector<double> > Y(input.rows,std::vector<double>(input.cols));
    double *Yhistogram = new double[256];
    memset(Yhistogram, 0, 256*sizeof(double));
    for(int i = 0; i != input.rows; ++i){
      uchar const *ptr = input.ptr<uchar>(i);
      for(int j = 0; j != input.cols; ++j){
          uchar const y = *ptr++;
          uchar const cr = *ptr++;
          uchar const cb = *ptr++;
          if(((cr - cb_left_criteria) < cb_right_criteria) && ((cb - cr_left_criteria) < cr_right_criteria)){
              ++satisfy_number;
              Y[i][j] = y;
              Yhistogram[y]++;
          }
      }
    }
    
    double Yhistogramsum = 0;
    double Ymin = 0;
    for (int i = 256 - 1; i >= 0; i--) {
        Yhistogramsum += Yhistogram[i];
        if (Yhistogramsum > 0.05 * satisfy_number) {
            Ymin = i;
            break;
        }
    }
    
    double Raver = 0, Gaver = 0, Baver = 0;
    double averSum = 0;
    for (int i = 0; i < Y.size(); i++) {
        for (int j = 0; j < Y[i].size(); j++) {
            if (Y[i][j] > Ymin) {
     
                int r = tmp.at<cv::Vec3b>(i, j)[0];
                int g = tmp.at<cv::Vec3b>(i, j)[1];
                int b = tmp.at<cv::Vec3b>(i, j)[2];
                Raver += r;
                Gaver += g;
                Baver += b;
                averSum++;
            }
        }
    }
    Raver /= averSum;
    Gaver /= averSum;
    Baver /= averSum;
    
//    ymax /= 15.0;
    double Rgain = ymax / Raver, Ggain = ymax / Gaver, Bgain = ymax / Baver;
    for (int i = 0; i < tmp.rows; i++) {
      for (int j = 0; j < tmp.cols; j++) {
          tmp.at<cv::Vec3b>(i, j)[0] = (int)(Rgain * tmp.at<cv::Vec3b>(i, j)[0]) > 255 ? 255 : (int)(Rgain * tmp.at<cv::Vec3b>(i, j)[0]);
          tmp.at<cv::Vec3b>(i, j)[1] = (int)(Ggain * tmp.at<cv::Vec3b>(i, j)[1]) > 255 ? 255 : (int)(Ggain * tmp.at<cv::Vec3b>(i, j)[1]);
          tmp.at<cv::Vec3b>(i, j)[2] = (int)(Bgain * tmp.at<cv::Vec3b>(i, j)[2]) > 255 ? 255 : (int)(Bgain * tmp.at<cv::Vec3b>(i, j)[2]);
//          std::cout<<tmp.at<cv::Vec3b>(i, j)<<" ";
      }
    }
    
    
    return tmp;
}

cv::Mat MSRCR(cv::Mat& img) {
  cv::Mat a = img.clone();
  MSRCR (a.data, img.cols, img.rows, 3, false);
  return a;
}

int endsWith(const char *str, const char *suffix)
{
    if (!str || !suffix)
        return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr)
        return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}


double getColorCastFactor(cv::Mat input)
{
    cv::Mat tmp(input);
    if(input.channels() == 4){
      cv::cvtColor(input, tmp, CV_BGRA2BGR); //去掉alpha通道
    }
    cv::cvtColor(tmp, tmp, CV_BGR2Lab);

    int SumA = 0, SumB = 0;
    int Width = tmp.cols;
    int Height = tmp.rows;
    double MsqA = 0, MsqB = 0, AvgA, AvgB;
    int A, B;

    int *HistA = new int[256];
    int *HistB = new int[256];

    for (int i = 0; i < tmp.rows; i++) {
      for (int j = 0; j < tmp.cols; j++) {
          A = tmp.at<cv::Vec3b>(i, j)[1];
          B = tmp.at<cv::Vec3b>(i, j)[2];
          SumA += A;
          SumB += B;
          HistA[A]++;
          HistB[B]++;
//          std::cout<<tmp.at<cv::Vec3b>(i, j)<<" ";
      }
    }

    AvgA = (double)SumA / (Width * Height) - 128;                  // 必须归一化到[-128，,127]范围内
    AvgB = (double)SumB / (Width * Height) - 128;
    for (int Y = 0; Y < 256; Y++)
    {
       MsqA += (double)(abs(Y - AvgA - 128) * HistA[Y]) / (Width * Height);           // 用方差的方式结果有问题
       MsqB += (double)(abs(Y - AvgB - 128) * HistB[Y]) / (Width * Height);
    }

    delete []HistA;
    delete []HistB;
    return sqrt(AvgA * AvgA + AvgB * AvgB) / sqrt(MsqA * MsqA + MsqB * MsqB);
}

}//namesapce colorconstant

int main(int argc, const char * argv[]) {
    cv::Mat imageR = cv::imread("/Users/tongwang/Downloads/ex1.jpg");
    cv::Mat mask = cv::imread("/Users/tongwang/Downloads/ex1.jpg", cv::IMREAD_GRAYSCALE);
    cv::threshold(mask, mask, 128, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
//    cv::Mat imageR = cv::imread("/Users/tongwang/Downloads/ex2.jpg");
//    cv::Mat mask = cv::imread("/Users/tongwang/Downloads/mask23.jpg", cv::IMREAD_GRAYSCALE);
//    cv::Mat src, maskr, img, dmask, ddmask;
//        cv::Point2i dsize = cv::Point2i(800, 600);
//        const float ls = std::max(/**/ std::min( /*...*/
//                std::max(imageR.rows, imageR.cols)/float(dsize.x),
//                std::min(imageR.rows, imageR.cols)/float(dsize.y)
//                                                   ), 1.0f /**/);
//    
//    //    cv::resize(<#InputArray src#>, <#OutputArray dst#>, <#Size dsize#>)
//        cv::resize (mask, maskr, cv::Size(maskr.size().width/ls, maskr.size().height/ls), 0, 0, cv::INTER_NEAREST);
//        cv::resize (imageR,  src,  cv::Size(maskr.size().width/ls, maskr.size().height/ls),  0, 0,    cv::INTER_AREA);
    cv::Mat H(5, 5, CV_64F);
    for(int i = 0; i < H.rows; i++)
        for(int j = 0; j < H.cols; j++)
            if(i<j)
                H.at<double>(i,j)=1./(i+j+1);
            else
                H.at<double>(i,j)=i+j;
    std::cout<<H;
    
    cv::imshow("1", imageR);
    cv::imshow("2", mask);
    cv::Mat img_lab;
    cv::cvtColor(imageR, img_lab, cv::COLOR_BGR2Lab);
    cv::Mat dst;
    cv::xphoto::inpaint(img_lab, mask, dst, cv::xphoto::INPAINT_SHIFTMAP);
    cv::cvtColor(dst, dst, cv::COLOR_Lab2BGR);
    cv::imshow("shiftmap", dst);
    cv::waitKey();
}




int main2(int argc, const char * argv[]) {
    // load an image
#if 1
    
    struct dirent *dirp;
    
    DIR* dir = opendir("./");
    
    while ((dirp = readdir(dir)) != nullptr) {
        if (dirp->d_type == DT_REG) {
            // 文件
            printf("%s\n", dirp->d_name);
            if (colorconstant::endsWith(dirp->d_name, ".jpg")) {
//                colorconstant::showGrayWorld(dirp->d_name);
                colorconstant::showbalace(dirp->d_name);
            }
            
            
        } else if (dirp->d_type == DT_DIR) {
            // 文件夹
        }
    }
    
    closedir(dir);
    // make sure you change the path!
//    cv::Mat img = cv::imread("/Users/tongwang/Downloads/user_dataset/imgs/0023347.jpg");
    
    cv::waitKey(0);
    cv::destroyAllWindows();
    

#else
    
    struct dirent *dirp;
    
    DIR* dir = opendir("./");
    
    while ((dirp = readdir(dir)) != nullptr) {
        if (dirp->d_type == DT_REG) {
            // 文件
            printf("%s\n", dirp->d_name);
            if (colorconstant::endsWith(dirp->d_name, ".jpg")) {
//                colorconstant::showMSR(dirp->d_name);
                colorconstant::showDynamicThresholdMethod(dirp->d_name);
            }
            
            
        } else if (dirp->d_type == DT_DIR) {
            // 文件夹
        }
    }
    
    closedir(dir);
    
    cvWaitKey(0);
#endif
    return 0;
}
