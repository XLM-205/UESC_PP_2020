#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <omp.h>

#define CONSOLE_PAUSE (void)getchar();

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../Dependencies/stb_image.h"			//Image Reading
#include "../Dependencies/stb_image_write.h"	//Image Writing	

#include <opencv2/opencv.hpp>

typedef unsigned char uint8R;
typedef unsigned long long int uint64R;

//define MAKE_BMP
#define MINIMAL_VERBOSE							//Comment this line to supress minimal system messages, such as batch progress and time consumption of fast mode


#ifdef MAKE_BMP
#define OUTPUT		"O.bmp"
#define	SAVE_IMAGE(outPath, width, heigth, channels, data) stbi_write_bmp(outPath, width, heigth, channels, data)
#else
#define OUTPUT		"O.png"
#define	SAVE_IMAGE(outPath, width, heigth, channels, data) stbi_write_png(outPath, width, heigth, channels, data, 0)
#endif

//Size of the 'progress bar'
#define BAR_SIZE		 40
//Character codes for the printBar() and printBard()
#ifdef __linux__	//Flag generally set by the g++ compiler. Using this because the Linux terminal does NOT support extended ASCII
#define BLOCK_EMPTY		'_' //32  //176
#define BLOCK_FILLED	'#'	//219
#define CONSOLE_CLEAR system("clear");
#else
#define BLOCK_EMPTY		'_' //176
#define BLOCK_FILLED	219
//Windows exclusive solution for system("cls")
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#define CONSOLE_CLEAR {static const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE); std::cout.flush(); COORD rst = {0,0}; SetConsoleCursorPosition(hOut, rst);}
#endif

//Flags
bool g_isDefaultName = true;				//If 1, the user didn't provided an output name. Use the input name for output with 'BIN_' and 'HIST_' for the files. Default is TRUE (1)
bool g_errIsRelative = false;				//Error calculation. If 0, use absolute error. If 1, use relative. Default is 0
bool g_isThrCustom = false;					//If true, the amount of threads was set to something lower than the maximum
int g_initialL0 = 127;						//Initial threshold. Default is 127
int g_errMargin = 5;						//Error margin. Default is 5
int g_avThreads = omp_get_max_threads();	//Available threads the program can use. Default is the maximum available
int g_qSize = 120;							//Queue size for parallel video binarization. Default is 120

//Console 'UI'
void printBar(const char *label, int squares, int min, int max, int val, int fillBar)
{
	if (max < min)
	{
		max = min + 1;
	}
	if (val < min)
	{
		val = min;
	}
	else if (val > max)
	{
		val = max;
	}
	double perc = (double)val / max;
	int target = (int)(perc * squares);
	printf("%s [", label);
	if (fillBar)
	{
		for (int i = 0; i <= target; i++)
		{
			printf("%c", BLOCK_FILLED);
		}
		for (int i = target; i < squares; i++)
		{
			printf("%c", BLOCK_EMPTY);
		}
	}
	else
	{
		for (int i = 0; i <= target; i++)
		{
			printf("%c", BLOCK_EMPTY);
		}
		printf("%c", BLOCK_FILLED);
		for (int i = target + 1; i < squares; i++)
		{
			printf("%c", BLOCK_EMPTY);
		}
	}
	printf("] (%5d - %6.2f%%)\n", val, perc * 100);
}
void printBard(const char *label, int squares, double min, double max, double val, int fillBar)
{
	if (max < min)
	{
		max = min + 1;
	}
	if (val < min)
	{
		val = min;
	}
	else if (val > max)
	{
		val = max;
	}
	double perc = (double)val / max;
	int target = (int)(perc * squares);
	printf("%s [", label);
	if (fillBar)
	{
		for (int i = 0; i <= target; i++)
		{
			printf("%c", BLOCK_FILLED);
		}
		for (int i = target; i < squares; i++)
		{
			printf("%c", BLOCK_EMPTY);
		}
	}
	else
	{
		for (int i = 0; i <= target; i++)
		{
			printf("%c", BLOCK_EMPTY);
		}
		printf("%c", BLOCK_FILLED);
		for (int i = target + 1; i < squares; i++)
		{
			printf("%c", BLOCK_EMPTY);
		}
	}
	printf("] (%5lf - %6.2f%%)\n", val, perc * 100);
}
void printBarPartial(const char *label, int squares, int stSquare, double min, double max, double val)
{
	if (max < min)
	{
		max = min + 1;
	}
	if (val < min)
	{
		val = min;
	}
	else if (val > max)
	{
		val = max;
	}
	if(stSquare > squares)
	{
		stSquare = squares - 1;
	}
	double perc = (double)val / max;
	int target = (int)(perc * squares) + stSquare;
	if(target > BAR_SIZE)
	{
		target = BAR_SIZE;
	}
	printf("%s [", label);
	for (int i = 0; i <= stSquare; i++)
	{
		printf("%c", BLOCK_EMPTY);
	}
	for (int i = stSquare + 1; i <= target; i++)
	{
		printf("%c", BLOCK_FILLED);
	}
	for (int i = target; i < squares; i++)
	{
		printf("%c", BLOCK_EMPTY);
	}
	printf("] (%5lf - %6.2f%%)\n", val, perc * 100);
}

//Return a string with the filename.format with 'prefixPadding' spaces before it. 'formatIndex' is the index of the last '.' found
char *getFilename(const char *fullPath, int *formatIndex, int prefixPadding)
{
	int nameStartIndex = 0, outLen = 0, formatLen = 0, j = 0, i = 0;
	char *filename = NULL;
	for (; fullPath[i]; i++)	//Run the entire path and try to find the start of the name
	{
		outLen++;
		//If the input path contains '/', use it's index to find the file name
		if (fullPath[i] == '/' || fullPath[i] == '\\')
		{
			nameStartIndex = i + 1;
			outLen = 0;
		}
		if(fullPath[i] == '.')
		{
			*formatIndex = i + prefixPadding + 1;
		}
	}
	outLen += prefixPadding;
	filename = (char *)malloc(outLen + 1);
	for (i = nameStartIndex, j = prefixPadding; fullPath[i]; i++, j++)
	{
		filename[j] = fullPath[i];
	}
	filename[outLen] = '\0';
	return filename;
}

//Receives the histogram of a grayscaled image and returns a threshold [0..255]
int computeThreshold(int *histogram)
{
	int l0 = g_initialL0, l1 = 0;		//Initial threshold (guess) and temporary threshold
	uint64R sumMx, sumMn;				//Sum of pixels that exceeds the threshold and those that doesn't
	uint64R contMx, contMn;				//Amout of pixels that exceeds the threshold and those that doesn't
	int dif = g_initialL0;
	do {
		contMx = contMn = sumMx = sumMn = 0;
		for (int i = 0; i < l0; i++)
		{
			sumMn += histogram[i] * i;
			contMn += histogram[i];
		}
		for (int i = l0; i < 256; i++)
		{
			sumMx += histogram[i] * i;
			contMx += histogram[i];
		}
//We might be able to optmize this
		if(!contMx || !contMn)			//Return 0 if either 'contMx' or 'contMn' are 0
		{
			return 0;
		}
		l1 = (int)(((sumMx / contMx) + (sumMn / contMn)) / 2);
		dif = abs(l1 - l0);
		//dif = (dif / l0 * g_errIsRelative) + (dif * !g_errIsRelative);	//Branchless implementation
		if (g_errIsRelative)
		{
			dif = dif / l0;
		}
		l0 = l1;
	} while (dif > g_errMargin);
	return l1;
}

//Binarizes videos
void binarizeVideo(cv::Mat *frame, int range, uint8R *grayScale, int *histogram)
{
	int threshold = 0;
	for(int i = 0; i < frame->rows; i++)
	{
		int line = i * frame->cols;
		for(int j = 0; j < frame->cols; j++)
		{
			cv::Vec3b pixel = frame->at<cv::Vec3b>(i, j);
			grayScale[line + j] = (uint8R)(pixel.val[2] * 0.21 + pixel.val[1] * 0.72 + pixel.val[0] * 0.07);	//Because CV uses BGR instead of RGB
		}
	}
	for(int i = 0; i < range; ++i)
	{
		histogram[grayScale[i]]++;
	}
	threshold = computeThreshold(histogram);
	for(int i = 0; i < frame->rows; i++)
	{
		int line = i * frame->cols;
		for(int j = 0; j < frame->cols; j++)
		{
			int wbPixel = (grayScale[line + j] > threshold) * 255;
			cv::Vec3b gPixel(wbPixel, wbPixel, wbPixel);
			frame->at<cv::Vec3b>(i, j) = gPixel;
		}
	}
}

void binarizeVideoVerbose(cv::Mat *frame, int range, int framesTotal, uint8R *grayScale, int *histogram, int frameID, int frameRate)
{
	int threshold = 0;
	double rFreq = 0.0, gFreq = 0.0, bFreq = 0.0;									//Frequency of colors
	double timer = 0.0;
	double convertingT = 0.0, histCatT = 0.0, thresholdingT = 0.0, binaryingT = 0.0;
	timer = omp_get_wtime();
	convertingT = omp_get_wtime();
	for(int i = 0; i < frame->rows; i++)
	{
		int line = i * frame->cols;
		for(int j = 0; j < frame->cols; j++)
		{
			cv::Vec3b pixel = frame->at<cv::Vec3b>(i, j);
			grayScale[line + j] = (uint8R)(pixel.val[2] * 0.21 + pixel.val[1] * 0.72 + pixel.val[0] * 0.07);	//Because CV uses BGR instead of RGB
			rFreq += (double)pixel.val[2] / 255;
			gFreq += (double)pixel.val[1] / 255;
			bFreq += (double)pixel.val[0] / 255;

		}
	}
	rFreq /= range;
	gFreq /= range;
	bFreq /= range;
	convertingT = omp_get_wtime() - convertingT;
	histCatT = omp_get_wtime();
	for(int i = 0; i < range; ++i)
	{
		histogram[grayScale[i]]++;
	}
	histCatT = omp_get_wtime() - histCatT;
	thresholdingT = omp_get_wtime();
	threshold = computeThreshold(histogram);
	thresholdingT = omp_get_wtime() - thresholdingT;
	binaryingT = omp_get_wtime();
	for(int i = 0; i < frame->rows; i++)
	{
		int line = i * frame->cols;
		for(int j = 0; j < frame->cols; j++)
		{
			int wbPixel = (grayScale[line + j] > threshold) * 255;
			cv::Vec3b gPixel(wbPixel, wbPixel, wbPixel);
			frame->at<cv::Vec3b>(i, j) = gPixel;
		}
	}
	binaryingT = omp_get_wtime() - binaryingT;
	timer = omp_get_wtime() - timer;
	double barSkip = 0.0;
	printf("(!!) WARNING: VIDEO VERBOSE MODE IS JUST FOR DEMONSTRATION! (!!)\n");
	printf("Frame [%6d / %6d]: %dpx x %dpx (%8d Bytes payload)\n", frameID, framesTotal, frame->rows, frame->cols, range);
	printf("[FPS: %3d] [%s: %2d] [L0: %3d] [err: %3d] Dif.:[%s]\n", frameRate, (g_isThrCustom ? "Threads" : "Av.Threads"),g_avThreads, g_initialL0, g_errMargin, (g_errIsRelative ? "RELATIVE" : "ABSOLUTE"));
	printf("[Frame Cost: %12.8lf]   [Parallel Cost Est.: %12.8lf]\n", timer, timer / g_avThreads);
	printBar("F.RED      ", BAR_SIZE, 0, 100, (int)(rFreq * 100), 1);
	printBar("F.GREEN    ", BAR_SIZE, 0, 100, (int)(gFreq * 100), 1);
	printBar("F.BLUE     ", BAR_SIZE, 0, 100, (int)(bFreq * 100), 1);
	printBar("THRESHOLD  ", BAR_SIZE, 0, 255, threshold, 0);

	printf("\n* Time consumption breakdown: (Each block is %5.1f%% - %3d segments)\n", (float)100 / BAR_SIZE, BAR_SIZE);
	printBard("CONV. TO GS", BAR_SIZE, 0, timer, convertingT, true);
	barSkip += (convertingT / timer);
	printBarPartial("HIST. CATEG", BAR_SIZE, (int)(BAR_SIZE * barSkip), 0, timer, histCatT);
	barSkip += (histCatT / timer);
	printBarPartial("TH. COMPUTE", BAR_SIZE, (int)(BAR_SIZE * barSkip), 0, timer, thresholdingT);
	barSkip += (thresholdingT / timer);
	printBarPartial("BINARYING  ", BAR_SIZE, (int)(BAR_SIZE * barSkip), 0, timer, binaryingT);
	barSkip += (binaryingT / timer);
	printBarPartial("OTHER TASKS", BAR_SIZE, (int)(BAR_SIZE * barSkip), 0, timer, timer - (convertingT + thresholdingT + binaryingT));
	printBard("TOTAL TIME ", BAR_SIZE, 0, timer, timer, 1);
}

//Handles I/O and feed the 'binarizeVideo' functions
int videoReadParallel(const char *input, const char *output)
{
	cv::VideoCapture vid(input);
	cv::Mat frame;
	int formatIndex = 0;
	char *filename = (char*)output;
	if(g_isDefaultName)	//If the users didn't provided a name, place 'BINA_' on the input name and make it .avi (default)
	{
		filename = getFilename(input, &formatIndex, 5);
		filename[0] = 'B'; filename[1] = 'I'; filename[2] = 'N'; filename[3] = 'A'; filename[4] = '_';
		filename[formatIndex] = 'a'; filename[formatIndex + 1] = 'v'; filename[formatIndex + 2] = 'i';
	}

	if(!vid.isOpened())
	{
		printf("[ERROR] Couldn't open video file!\n");
		return -1;
	}

	double vidFPS = vid.get(cv::CAP_PROP_FPS), vidFrameCount = vid.get(cv::CAP_PROP_FRAME_COUNT);
	int vidWidth = (int)vid.get(cv::CAP_PROP_FRAME_WIDTH), vidHeight = (int)vid.get(cv::CAP_PROP_FRAME_HEIGHT);
	int range = vidWidth * vidHeight, remainFrames = (int)vidFrameCount;
	int batches = ((int)vidFrameCount / g_qSize) + (!((int)vidFrameCount % g_qSize) == 0);
	cv::VideoWriter outVid(filename, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), vidFPS, cv::Size(vidWidth, vidHeight));
	cv::Mat *frames = new cv::Mat[g_qSize];
	double videoSeconds = vidFrameCount / vidFPS;
	double timer = omp_get_wtime();

#ifdef MINIMAL_VERBOSE
		printf("Conversion of %s started\n> %4d x %4d @%6.2lf FPS (%6d Frames)\n", input, vidWidth, vidHeight, vidFPS, (int)vidFrameCount);
		printf("[L0: %3d] [err: %3d] Dif.:[%s]\n", g_initialL0, g_errMargin, (g_errIsRelative ? "RELATIVE" : "ABSOLUTE"));
		printf("[Queue Size: %4d] [%s: %3d]\n", g_qSize, (g_isThrCustom ? "Threads" : "Av. Threads"), g_avThreads);
#endif
	//Creating a common buffer for each thread so that we don't need to alloc / dealloc memory everytime
	uint8R **grayScale = (uint8R**)malloc(g_avThreads * sizeof(uint8R*));
	int **hist = (int**)malloc(256 * sizeof(int*));
	for(int i = 0; i < g_avThreads; i++)
	{
		grayScale[i] = (uint8R*)malloc(range);
		hist[i] = (int*)calloc(256, sizeof(int));
	}
	for(int i = 1; remainFrames; i++)
	{
		int curLoad = g_qSize < remainFrames ? g_qSize : remainFrames;
		//Fill Queue Step
		for(int i = 0; i < curLoad; i++)
		{
			vid >> frames[i];
		}
		//Parallel Binarization Step
#pragma omp parallel
		{
			int me = omp_get_thread_num();
			#pragma omp for
			for(int i = 0; i < curLoad; i++)
			{
				binarizeVideo(&frames[i], range, grayScale[me], hist[me]);
				for(int j = 0; j < 256; j++)
				{
					hist[me][j] = 0;
				}
			}
		}

		//Video Output Step
#ifdef MINIMAL_VERBOSE
		printf("Frame batch %5d / %5d", i, batches);
		printf("\r");
#endif

		for(int i = 0; i < g_qSize; i++)
		{
			outVid << frames[i];
		}
		remainFrames -= curLoad;
	}
	timer = omp_get_wtime() - timer;

#ifdef MINIMAL_VERBOSE
	double outH  = timer / 3600;
	double outM  = fabs(((int)(outH) - outH) * 60);
	double outS  = fabs(((int)(outM) - outM) * 60);
	int outMS	 = (int)fabs(((int)(outS) - outS) * 1000);
	double vidH  = videoSeconds / 3600;
	double vidM  = fabs(((int)(vidH) - vidH) * 60);
	double vidS  = fabs(((int)(vidM) - vidM) * 60);
	int vidMS	 = (int)fabs(((int)(vidS) - vidS) * 1000);
	printf("\tTime took   : %15.8lf seconds (%2dh: %2dm: %2ds: %4dms)\n", timer, (int)outH, (int)outM, (int)outS, outMS);
	printf("\tVideo lenght: %15.8lf seconds (%2dh: %2dm: %2ds: %4dms)\n", videoSeconds, (int)vidH, (int)vidM, (int)vidS, vidMS);
#endif
	//Cleanup step
	for(int i = 0; i < g_avThreads; i++)
	{
		free(grayScale[i]);
		free(hist[i]);
	}
	free(grayScale);
	free(hist);
	if(g_isDefaultName)
	{
		free(filename);
	}
	delete[] frames;
	outVid.release();
	vid.release();
	return 0;
}

//Old, single threaded version
//int videoRead(const char *input)
//{
//	cv::VideoCapture vid(input);
//	cv::Mat frame;
//	if(!vid.isOpened())
//	{
//		printf("[ERROR] Couldn't open video file!\n");
//		return -1;
//	}
//	int vidWidth = (int)vid.get(cv::CAP_PROP_FRAME_WIDTH), vidHeight = (int)vid.get(cv::CAP_PROP_FRAME_HEIGHT);
//	double vidFPS = vid.get(cv::CAP_PROP_FPS);
//	cv::VideoWriter outVid("out.avi", cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), vidFPS, cv::Size(vidWidth, vidHeight));
//	printf("Conversion of %s started\n> %4d x %4d @%6.2lf FPS\n", input, vidWidth, vidHeight, vidFPS);
//	int range = vidWidth * vidHeight;
//	double timer = omp_get_wtime();
//	uint8R *grayScale = (uint8R*)malloc(range);			//The pixel computed grayscale
//	int *histogram = (int*)calloc(256, sizeof(int));	//The histogram
//
//	vid >> frame;
//	while(!frame.empty())
//	{
//		//if(frame.empty())
//		//{
//		//	printf("EOF Reached\n");
//		//	break;
//		//}
//		//if(cv::waitKey(25) == 27)		//Stop if the user press ESC
//		//{
//		//	printf("Execution stopped by User\n");
//		//	break;
//		//}
//		binarizeVideo(&frame, range);// , grayScale, histogram);
//		outVid << frame;
//		//cv::imshow("Press ESC to close", frame);	//Only available on Verbose
//		vid >> frame;
//		/*for(int j = 0; j < 256; ++j)
//		{
//			histogram[j] = 0;
//		}*/
//	}
//
//	timer = omp_get_wtime() - timer;
//	double videoSeconds = vid.get(cv::CAP_PROP_FRAME_COUNT) / vidFPS;
//	printf("Time took   : %12.8lf seconds\n", timer);
//	printf("Video lenght: %12.8lf seconds\n", videoSeconds);
//	printf("Gain        : %12.8lf seconds (%6.2lf%%)\n", videoSeconds - timer, (videoSeconds / timer) * 100);
//	free(histogram);
//	free(grayScale);
//	outVid.release();
//	vid.release();
//	cv::destroyAllWindows();
//	return 0;
//}

int videoReadVerboseParallel(const char *input, const char *output)
{
	uint64R tClock = clock();
	cv::VideoCapture vid(input);
	cv::Mat frame;
	int fRate = 0, formatIndex = 0;
	char *filename = (char*)output;
	if(g_isDefaultName)
	{
		filename = getFilename(input, &formatIndex, 5);
		filename[0] = 'B'; filename[1] = 'I'; filename[2] = 'N'; filename[3] = 'A'; filename[4] = '_';
		filename[formatIndex] = 'a'; filename[formatIndex + 1] = 'v'; filename[formatIndex + 2] = 'i';
	}
	if(!vid.isOpened())
	{
		printf("[ERROR] Couldn't open video file!\n");
		return -1;
	}

	double vidFPS = vid.get(cv::CAP_PROP_FPS), vidFrameCount = vid.get(cv::CAP_PROP_FRAME_COUNT);
	int vidWidth = (int)vid.get(cv::CAP_PROP_FRAME_WIDTH), vidHeight = (int)vid.get(cv::CAP_PROP_FRAME_HEIGHT);
	int range = vidWidth * vidHeight;
	cv::VideoWriter outVid(filename, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), vidFPS, cv::Size(vidWidth, vidHeight));
	cv::Mat *frames = new cv::Mat[g_qSize];

#ifndef __linux__
	system("cls");	//Remove everything from the console
#endif

	uint8R *grayScale = (uint8R*)malloc(range);
	int *hist = (int*)calloc(256, sizeof(int));
	vid >> frame;
	for(int i = 1, framesPS = 0; !frame.empty(); i++, framesPS++, vid >> frame)
	{
		if(cv::waitKey(25) == 27)		//Stop if the user press ESC
		{
			printf("Execution stopped by User\n");
			break;
		}
		if(tClock <= clock())
		{
			tClock = clock() + 1000;
			fRate = framesPS;
			framesPS = 0;
		}
		CONSOLE_CLEAR
		binarizeVideoVerbose(&frame, range, (int)vidFrameCount, grayScale, hist, i, fRate);
		cv::imshow("[VERBOSE] Press ESC to close", frame);
		for(int j = 0; j < 256; j++)
		{
			hist[j] = 0;
		}
	}
	free(hist);
	free(grayScale);
	vid.release();
	cv::destroyAllWindows();
	return 0;
}

//Binarizes images
int binarizeImage(const char *input, const char *output)
{
	int width = 0, heigth = 0, channels = 0;										//Image properties
	int range = 0, threshold = 0;
	int fIndex = 0;																	//Index of the last '.' in the file
	char *filename = NULL;
	FILE *outF = NULL;
	uint8R *image = stbi_load(input, &width, &heigth, &channels, 0);					//Load image
	if (image == NULL)
	{
		printf("[ERROR] Failed to read image at: %s!\n", input);
		return -1;
	}
	range = width * heigth;															//Optimization
	uint8R *grayScale = NULL;														//The pixel computed grayscale
	int *histogram = (int *)calloc(256, sizeof(int));								//The histogram
	if (channels == 1)																//If one channel, image is grayscale
	{
		for (int i = 0; i < range; ++i)												//Making the histogram
		{
			histogram[image[i]]++;
		}
		threshold = computeThreshold(histogram);
		for (int i = 0; i < range; i++)												//"Binarizing" the image: Every pixel with scale <= 'threshold' will become 0 (black)
		{
			image[i] = (image[i] > threshold) * 255;
		}
	}
	else
	{
		grayScale = (uint8R *)malloc(range);
		for (int i = 0, pIndex = 0; i < range; i++, pIndex += channels)				//For each index 'i', read Red, Green, Blue and skip Alpha (if present) and jump 'channels' ahead
		{
			grayScale[i] = (uint8R)(image[pIndex] * 0.21 + image[pIndex + 1] * 0.72 + image[pIndex + 2] * 0.07);
		}
		for (int i = 0; i < range; ++i)
		{
			histogram[grayScale[i]]++;
		}
		threshold = computeThreshold(histogram);
		for (int i = 0, pIndex = 0; i < range; i++, pIndex += channels)
		{
			image[pIndex] = image[pIndex + 1] = image[pIndex + 2] = (grayScale[i] > threshold) * 255;
		}
	}
	if (g_isDefaultName)
	{
		filename = getFilename(input, &fIndex, 5);
		filename[0] = 'B'; filename[1] = 'I'; filename[2] = 'N'; filename[3] = 'A'; filename[4] = '_';	//"BINA_"
		SAVE_IMAGE(filename, width, heigth, channels, image);
		filename[0] = 'H'; filename[2] = 'S'; filename[3] = 'T';										//"HIST_" ('I' and '_' are already there, therefore we skip them)
	}
	else
	{
		filename = getFilename(output, &fIndex, 0);									//User provided an output name, so no prefix is required
		SAVE_IMAGE(output, width, heigth, channels, image);
	}
	filename[fIndex] = 't';	filename[fIndex + 1] = 'x';	filename[fIndex + 2] = 't';	//Changing the format to make a .txt file
	outF = fopen(filename, "w");
	fprintf(outF, "%d\n", threshold);
	for (int i = 0; i < 256; i++)
	{
		fprintf(outF, "%d\n", histogram[i]);
	}
	fclose(outF);
	free(filename);
	free(histogram);
	if (grayScale)
	{
		free(grayScale);
	}
	STBI_FREE(image);
	return 0;
}

int binarizeImageVerbose(const char *input, const char *output)
{
	int width = 0, heigth = 0, channels = 0;
	int range = 0, threshold = 0;
	int fIndex = 0;
	char *filename = NULL;
	FILE *outF;
	double rFreq = 0.0, gFreq = 0.0, bFreq = 0.0;									//Frequency of colors
	double timer = 0.0;
	double loadT = 0.0, allocT = 0.0, convertingT = 0.0, histCatT = 0.0, thresholdingT = 0.0, binaryingT = 0.0, writeIT = 0.0, writeLT = 0.0, deAllocT = 0.0;
	//- LOAD STEP
	loadT = timer = omp_get_wtime();
	uint8R *image = stbi_load(input, &width, &heigth, &channels, 0);
	loadT = omp_get_wtime() - loadT;
	if (image == NULL)
	{
		printf("[ERROR] Failed to read image at: %s!\n", input);
		return -1;
	}
	range = width * heigth;
	//- ALLOCATION STEP
	allocT = omp_get_wtime();
	uint8R *grayScale = NULL;
	int *histogram = (int *)calloc(256, sizeof(int));
	allocT = omp_get_wtime() - allocT;
	if (channels == 1)
	{
		//- HISTOGRAM CATEGORIZATION STEP
		histCatT = omp_get_wtime();
		for (int i = 0; i < range; ++i)
		{
			histogram[image[i]]++;
		}
		histCatT = omp_get_wtime() - histCatT;
		//- THRESHOLDING STEP
		thresholdingT = omp_get_wtime();
		threshold = computeThreshold(histogram);
		thresholdingT = omp_get_wtime() - thresholdingT;
		//- BINARYING STEP
		binaryingT = omp_get_wtime();
		for (int i = 0; i < range; i++)
		{
			image[i] = (image[i] > threshold) * 255;
		}
		binaryingT = omp_get_wtime() - binaryingT;
	}
	else
	{
		grayScale = (uint8R *)malloc(range);
		//- CONVERTING STEP
		convertingT = omp_get_wtime();
		for (int i = 0, pIndex = 0; i < range; i++, pIndex += channels)
		{
			grayScale[i] = (uint8R)(image[pIndex] * 0.21 + image[pIndex + 1] * 0.72 + image[pIndex + 2] * 0.07);
			rFreq += (double)image[pIndex] / 255;
			gFreq += (double)image[pIndex + 1] / 255;
			bFreq += (double)image[pIndex + 2] / 255;
		}
		rFreq /= range;
		gFreq /= range;
		bFreq /= range;
		convertingT = omp_get_wtime() - convertingT;
		//- HISTOGRAM CATEGORIZATION STEP
		histCatT = omp_get_wtime();
		for (int i = 0; i < range; ++i)
		{
			histogram[grayScale[i]]++;
		}
		histCatT = omp_get_wtime() - histCatT;
		//- THRESHOLDING STEP
		thresholdingT = omp_get_wtime();
		threshold = computeThreshold(histogram);
		thresholdingT = omp_get_wtime() - thresholdingT;
		//- BINARYING STEP
		binaryingT = omp_get_wtime();
		for (int i = 0, pIndex = 0; i < range; i++, pIndex += channels)
		{
			image[pIndex] = image[pIndex + 1] = image[pIndex + 2] = (grayScale[i] > threshold) * 255;
		}
		binaryingT = omp_get_wtime() - binaryingT;
	}
	printf("Image: %dpx x %dpx with %d channels (%8d Bytes payload)\n", width, heigth, channels, range * channels);
	printf("[L0: %3d] [err: %3d] Dif.:[%s]\n", g_initialL0, g_errMargin, (g_errIsRelative ? "RELATIVE" : "ABSOLUTE"));
	printBar("F.RED      ", BAR_SIZE, 0, 100, (int)(rFreq * 100), 1);
	printBar("F.GREEN    ", BAR_SIZE, 0, 100, (int)(gFreq * 100), 1);
	printBar("F.BLUE     ", BAR_SIZE, 0, 100, (int)(bFreq * 100), 1);
	printBar("THRESHOLD  ", BAR_SIZE, 0, 255, threshold, 0);
	if (g_isDefaultName)
	{
		//- WRITE IMAGE STEP
		writeIT = omp_get_wtime();
		filename = getFilename(input, &fIndex, 5);
		filename[0] = 'B'; filename[1] = 'I'; filename[2] = 'N'; filename[3] = 'A'; filename[4] = '_';	//"BINA_"
		SAVE_IMAGE(filename, width, heigth, channels, image);
		writeIT = omp_get_wtime() - writeIT;
		//- WRITE THRESHOLD STEP
		writeLT = omp_get_wtime();
		filename[0] = 'H'; filename[2] = 'S'; filename[3] = 'T';										//"HIST_" ('I' and '_' are already there, therefore we skip them)
	}
	else
	{
		//- WRITE IMAGE STEP
		writeIT = omp_get_wtime();
		filename = getFilename(output, &fIndex, 0);
		SAVE_IMAGE(output, width, heigth, channels, image);
		writeIT = omp_get_wtime() - writeIT;
		writeLT = omp_get_wtime();
	}
	filename[fIndex] = 't';	filename[fIndex + 1] = 'x';	filename[fIndex + 2] = 't';						//Changing the format to make a .txt file
	outF = fopen(filename, "w");
	fprintf(outF, "%d\n", threshold);
	for (int i = 0; i < 256; i++)
	{
		fprintf(outF, "%d\n", histogram[i]);
	}
	fclose(outF);
	writeLT = omp_get_wtime() - writeLT;
	//- DEALLOCATION STEP
	deAllocT = omp_get_wtime();
	free(filename);
	free(histogram);
	if (grayScale)
	{
		free(grayScale);
	}
	STBI_FREE(image);
	deAllocT = omp_get_wtime() - deAllocT;
	timer = omp_get_wtime() - timer;

	////Statistics -----------------------------------------------------------------------------------
	double barSkip = 0.0;
	printf("\n* Time consumption breakdown: (Each block is %5.1f%% - %3d segments)\n", (float)100 / BAR_SIZE, BAR_SIZE);
	printBard("LOADING IMG", BAR_SIZE, 0, timer, loadT, 1);
	barSkip += (loadT / timer);
	printBarPartial("ALLOCATING ", BAR_SIZE, (int)(BAR_SIZE * barSkip), 0, timer, allocT);
	barSkip += (allocT / timer);
	printBarPartial("CONV. TO GS", BAR_SIZE, (int)(BAR_SIZE * barSkip), 0, timer, convertingT);
	barSkip += (convertingT / timer);
	printBarPartial("HIST. CATEG", BAR_SIZE, (int)(BAR_SIZE * barSkip), 0, timer, histCatT);
	barSkip += (histCatT / timer);
	printBarPartial("TH. COMPUTE", BAR_SIZE, (int)(BAR_SIZE * barSkip), 0, timer, thresholdingT);
	barSkip += (thresholdingT / timer);
	printBarPartial("BINARYING  ", BAR_SIZE, (int)(BAR_SIZE * barSkip), 0, timer, binaryingT);
	barSkip += (binaryingT / timer);
	printBarPartial("WRITING IMG", BAR_SIZE, (int)(BAR_SIZE * barSkip), 0, timer, writeIT);
	barSkip += (writeIT / timer);
	printBarPartial("WRITING TH.", BAR_SIZE, (int)(BAR_SIZE * barSkip), 0, timer, writeLT);
	barSkip += (writeLT / timer);
	printBarPartial("DEALLOC.   ", BAR_SIZE, (int)(BAR_SIZE * barSkip), 0, timer, deAllocT);
	barSkip += (deAllocT / timer);
	printBarPartial("OTHER TASKS", BAR_SIZE, (int)(BAR_SIZE * barSkip), 0, timer, timer - (loadT + allocT + convertingT + thresholdingT + binaryingT + writeIT + writeLT + deAllocT));
	printBard("TOTAL TIME ", BAR_SIZE, 0, timer, timer, 1);
	printf("\nExecution End\n\t(!) Program ran in Verbose mode. For fast mode, run without the verbose argument 'v' or 'V'\n");
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("[ERROR] No path provided!\n");
return -2;
	}
	else if(argc > 2)													//More than 3 arguments, let's check what those are
	{
	int outputIndex = 0;											//Will point to where the output file name is (if not present, should remain 0)
	bool modeFast = true;
	bool modeImage = false;										//If true, the input will be treated as an image. Default is false -> Read video
	for(int i = 2; i < argc; ++i)
	{
		switch(argv[i][0])
		{
		case '-':		//Flag incoming
		case '/':
			switch(argv[i][1])
			{
			case 'h':	//Help flag. Just ignore it because it shouldn't be called here. The user probably didn't read the documentation >=/
			case 'H':
				break;
			case 'i':
			case 'I':
				modeImage = true;
				break;
			case 'v':	//Verbose mode
			case 'V':
				modeFast = false;
				break;
			case 'l':	//Threshold modifier (limiar em portugues)
			case 'L':
				if(++i >= argc || !sscanf(argv[i], "%d", &g_initialL0) || (g_initialL0 < 0 || g_initialL0 > 255))
				{
					printf("[ERROR] Invalid or absent Threshold value but flag was raised! Operation aborted\n");
					return -1;
				}
				break;
			case 't':
			case 'T':
				if(++i >= argc || !sscanf(argv[i], "%d", &g_avThreads) || g_avThreads < 1)
				{
					printf("[ERROR] Invalid or absent Thread Amount value but flag was raised! Operation aborted\n");
					return -1;
				}
				else if(g_avThreads > omp_get_max_threads())
				{
					printf("[WARNING] Amount of threads requested exceed the system maximum! Setting it to default value\n");
					g_avThreads = omp_get_max_threads();
				}
				else if(g_avThreads < omp_get_max_threads())
				{
					omp_set_dynamic(0);		//Force the OMP to NOT dynamically change the team size
					omp_set_num_threads(g_avThreads);
					g_isThrCustom = true;
				}
				break;
			case 'e':
			case 'E':
				if(++i >= argc || !sscanf(argv[i], "%d", &g_errMargin) || (g_errMargin < 0 || g_errMargin > 255))
				{
					printf("[ERROR] Invalid or absent Error Margin value but flag was raised! Operation aborted\n");
					return -1;
				}
				break;
			case 'q':	//Setting Video Queue Size
			case 'Q':
				if(++i >= argc || !sscanf(argv[i], "%d", &g_qSize) || g_qSize < 0)
				{
					printf("[ERROR] Invalid or absent Queue Size value but flag was raised! Operation aborted");
					return -1;
				}
				break;
			case 'r':	//Defining Error to Relative
			case 'R':
				g_errIsRelative = true;
				break;
			}
			break;
		default:		//Not a flag, probably (and hopefuly) a path
			g_isDefaultName = false;
			outputIndex = i;
			break;
		}
	}
	if(modeFast)	//Fast mode
	{
		if(modeImage)
		{
			return binarizeImage(argv[1], (outputIndex ? argv[outputIndex] : OUTPUT));
		}
		return videoReadParallel(argv[1], (outputIndex ? argv[outputIndex] : "Out.avi"));
	}
	if(modeImage)	//Verbose mode
	{
		return binarizeImageVerbose(argv[1], (outputIndex ? argv[outputIndex] : OUTPUT));
	}
	return videoReadVerboseParallel(argv[1], (outputIndex ? argv[outputIndex] : "Out.avi"));
	}
	if(argv[1][0] == '-' && (argv[1][1] == 'h' || argv[1][1] == 'H'))
	{
		printf("Available flags (case insensitive):\n");
		printf("-H: Shows this help panel\n");
		printf("-I: Interpret input as an image\n");
		printf("-V: Run in Verbose (Demo mode)\n");
		printf("-L: Defines the Initial Threshold [0..255]. Default is %d\n", g_initialL0);
		printf("-T: Defines the size of the team of threads [1..%d]. Default is %d\n\tNote: This flag DOES NOT update the queueSize\n", g_avThreads, g_avThreads);
		printf("-E: Defines the allowed error during threshold calculation [0..255]. Default is %d\n", g_errMargin);
		printf("-Q: Defines the size of the Input Queue. It's recommended to use a multiple of %d. Default is %d\n", g_avThreads, g_qSize);
		printf("-R: Sets the error calculation to be RELATVE. Default is %s\n", (g_errIsRelative ? "RELATIVE" : "ABSOLUTE"));
		return -2;
	}
	return videoReadParallel(argv[1], "Out.avi");											//No flags and no output name
}