#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <omp.h>

#define PAUSE_CONSOLE (void)getchar();

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"			//Image Reading
#include "stb_image_write.h"	//Image Writing	

typedef unsigned char uint8;
typedef unsigned int uint32;
typedef unsigned long long int uint64;

//#define MAKE_BMP	//Un-comment if you want the target image to be a .bmp

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
#else
#define BLOCK_EMPTY		'_' //176
#define BLOCK_FILLED	219
#endif

//Flags
int g_isDefaultName = 1;			//If 1, the user didn't provided an output name. Use the input name for output with 'BIN_' and 'HIST_' for the files. Default is TRUE (1)
int g_initialL0 = 127;			//Initial threshold. Default is 127
int g_errMargin = 5;			//Error margin. Default is 5
int g_errIsRelative = 0;			//Error calculation. If 0, use absolute error. If 1, use relative. Default is 0

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
	double perc = (double)val / max;
	int target = (int)(perc * squares) + stSquare;
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
	int nameStartIndex = 0, outLen = 0, j = 0, i = 0;
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
	}
	outLen += prefixPadding;
	filename = (char *)malloc(outLen + 1);
	for (i = nameStartIndex, j = prefixPadding; fullPath[i]; i++, j++)
	{
		filename[j] = fullPath[i];
	}
	filename[outLen] = '\0';
	*formatIndex = j - 3;	//Since our output is either 'png', 'bmp' or 'txt', we can simply go back 3 spaces
	return filename;
}

//Receives the histogram of a grayscaled image and returns a threshold [0..255]
int computeThreshold(int *histogram)
{
	int l0 = g_initialL0, l1 = 0;	//Initial threshold (guess) and temporary threshold
	uint64 sumMx, sumMn;				//Sum of pixels that exceeds the threshold and those that doesn't
	uint64 contMx, contMn;				//Amout of pixels that exceeds the threshold and those that doesn't
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
		l1 = ((sumMx / contMx) + (sumMn / contMn)) / 2;
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

//Same code as 'verboseMode()' but without console printing
void fastMode(const char *input, const char *output)
{
	int width = 0, heigth = 0, channels = 0;										//Image properties
	int range = 0, threshold = 0;
	int fIndex = 0;																	//Index of the last '.' in the file
	int pIndex = 0;
	char *filename = NULL;
	FILE *outF = NULL;
	uint8 *image = stbi_load(input, &width, &heigth, &channels, 0);					//Load image
	if (image == NULL)
	{
		printf("[ERROR] Failed to read image at: %s!\n", input);
		return;
	}
	range = width * heigth;															//Optimization
	uint8 *grayScale = NULL;														//The pixel computed grayscale
	int *histogram = (int *)calloc(256, sizeof(int));								//The histogram
	if (channels == 1)																//If one channel, image is grayscale
	{
#pragma omp for
		for (int i = 0; i < range; ++i)												//Making the histogram
		{
			histogram[image[i]]++;
		}
		threshold = computeThreshold(histogram);
#pragma omp for
		for (int i = 0; i < range; i++)												//"Binarizing" the image: Every pixel with scale <= 'threshold' will become 0 (black)
		{
			image[i] = (image[i] > threshold) * 255;
		}
	}
	else
	{
		grayScale = (uint8 *)malloc(range);
		pIndex = 0;
#pragma omp for private(pIndex)
		for (int i = 0; i < range; i++)				//For each index 'i', read Red, Green, Blue and skip Alpha (if present) and jump 'channels' ahead
		{
			grayScale[i] = (uint8)(image[pIndex] * 0.21 + image[pIndex + 1] * 0.72 + image[pIndex + 2] * 0.07);
			pIndex += channels;
		}
#pragma omp for
		for (int i = 0; i < range; ++i)
		{
			histogram[grayScale[i]]++;
		}
		threshold = computeThreshold(histogram);
		pIndex = 0;
#pragma omp for private(pIndex)
		for (int i = 0; i < range; i++)
		{
			image[pIndex] = image[pIndex + 1] = image[pIndex + 2] = (grayScale[i] > threshold) * 255;
			pIndex += channels;
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
}

void verboseMode(const char *input, const char *output)
{
	int width = 0, heigth = 0, channels = 0;
	int range = 0, threshold = 0;
	int fIndex = 0;
	int pIndex = 0;
	char *filename = NULL;
	FILE *outF;
	double rFreq = 0.0, gFreq = 0.0, bFreq = 0.0;									//Frequency of colors
	double timer = 0.0;
	double loadT = 0.0, allocT = 0.0, convertingT = 0.0, histCatT = 0.0, thresholdingT = 0.0, binaryingT = 0.0, writeIT = 0.0, writeLT = 0.0, deAllocT = 0.0;
	//- LOAD STEP
	loadT = timer = omp_get_wtime();
	uint8 *image = stbi_load(input, &width, &heigth, &channels, 0);
	loadT = omp_get_wtime() - loadT;
	if (image == NULL)
	{
		printf("[ERROR] Failed to read image at: %s!\n", input);
		return;
	}
	range = width * heigth;
	//- ALLOCATION STEP
	allocT = omp_get_wtime();
	uint8 *grayScale = NULL;
	int *histogram = (int *)calloc(256, sizeof(int));
	allocT = omp_get_wtime() - allocT;
	if (channels == 1)
	{
		//- HISTOGRAM CATEGORIZATION STEP
		histCatT = omp_get_wtime();
#pragma omp for
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
#pragma omp for
		for (int i = 0; i < range; i++)
		{
			image[i] = (image[i] > threshold) * 255;
		}
		binaryingT = omp_get_wtime() - binaryingT;
	}
	else
	{
		grayScale = (uint8 *)malloc(range);
		//- CONVERTING STEP
		convertingT = omp_get_wtime();
		pIndex = 0;
#pragma omp for private(pIndex)
		for (int i = 0; i < range; i++)
		{
			grayScale[i] = (uint8)(image[pIndex] * 0.21 + image[pIndex + 1] * 0.72 + image[pIndex + 2] * 0.07);
//#pragma omp atomic
//			rFreq += (double)image[pIndex] / 255;
//#pragma omp atomic
//			gFreq += (double)image[pIndex + 1] / 255;
//#pragma omp atomic
//			bFreq += (double)image[pIndex + 2] / 255;
			pIndex += channels;
		}
		/*rFreq /= range;
		gFreq /= range;
		bFreq /= range;*/
		convertingT = omp_get_wtime() - convertingT;
		//- HISTOGRAM CATEGORIZATION STEP
		histCatT = omp_get_wtime();
#pragma omp for
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
		pIndex = 0;
#pragma omp for private(pIndex)
		for (int i = 0; i < range; i++)
		{
			image[pIndex] = image[pIndex + 1] = image[pIndex + 2] = (grayScale[i] > threshold) * 255;
			pIndex += channels;
		}
		binaryingT = omp_get_wtime() - binaryingT;
	}
	printf("Image: %dpx x %dpx with %d channels (%8d Bytes payload)\n", width, heigth, channels, range * channels);
	printf("[L0: %3d] [err: %3d] Dif.:[%s]\n", g_initialL0, g_errMargin, (g_errIsRelative ? "RELATIVE" : "ABSOLUTE"));
	/*printBar("FREQ.RED      ", BAR_SIZE, 0, 100, (int)(rFreq * 100), 1);
	printBar("FREQ.GREEN    ", BAR_SIZE, 0, 100, (int)(gFreq * 100), 1);
	printBar("FREQ.BLUE     ", BAR_SIZE, 0, 100, (int)(bFreq * 100), 1);*/
	printBar("THRESHOLD     ", BAR_SIZE, 0, 255, threshold, 0);
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
	printBard("[S]LOADING IMG", BAR_SIZE, 0, timer, loadT, 1);
	barSkip += (loadT / timer);
	printBarPartial("[S]ALLOCATING ", BAR_SIZE, (int)(BAR_SIZE * barSkip), 0, timer, allocT);
	barSkip += (allocT / timer);
	printBarPartial("[P]CONV. TO GS", BAR_SIZE, (int)(BAR_SIZE * barSkip), 0, timer, convertingT);
	barSkip += (convertingT / timer);
	printBarPartial("[P]HIST. CATEG", BAR_SIZE, (int)(BAR_SIZE * barSkip), 0, timer, histCatT);
	barSkip += (histCatT / timer);
	printBarPartial("[S]TH. COMPUTE", BAR_SIZE, (int)(BAR_SIZE * barSkip), 0, timer, thresholdingT);
	barSkip += (thresholdingT / timer);
	printBarPartial("[P]BINARYING  ", BAR_SIZE, (int)(BAR_SIZE * barSkip), 0, timer, binaryingT);
	barSkip += (binaryingT / timer);
	printBarPartial("[S]WRITING IMG", BAR_SIZE, (int)(BAR_SIZE * barSkip), 0, timer, writeIT);
	barSkip += (writeIT / timer);
	printBarPartial("[S]WRITING TH.", BAR_SIZE, (int)(BAR_SIZE * barSkip), 0, timer, writeLT);
	barSkip += (writeLT / timer);
	printBarPartial("[S]DEALLOC.   ", BAR_SIZE, (int)(BAR_SIZE * barSkip), 0, timer, deAllocT);
	barSkip += (deAllocT / timer);
	printBarPartial("OTHER TASKS   ", BAR_SIZE, (int)(BAR_SIZE * barSkip), 0, timer, timer - (loadT + allocT + convertingT + thresholdingT + binaryingT + writeIT + writeLT + deAllocT));
	printBard("TOTAL TIME    ", BAR_SIZE, 0, timer, timer, 1);
	printf("\nExecution End\n\t(!) Program ran in Verbose mode. For fast mode, run without the verbose argument 'v' or 'V'\n");
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("[ERROR] No path provided!\n");
		return -2;
	}
	else if (argc > 2)													//More than 3 arguments, let's check what those are
	{
		int outputIndex = 0;											//Will point to where the output file name is (if not present, should remain 0)
		int modeFast = 1;
		for (int i = 2; i < argc; ++i)
		{
			switch (argv[i][0])
			{
			case '-':		//Flag incoming
			case '/':
				switch (argv[i][1])
				{
				case 'v':	//Verbose mode
				case 'V':
					modeFast = 0;
					break;
				case 't':	//Threshold modifier
				case 'T':
					if (++i >= argc || !sscanf(argv[i], "%d", &g_initialL0) || (g_initialL0 < 0 || g_initialL0 > 255))
					{
						printf("[ERROR] Invalid or absent Threshold value but flag was raised! Operation aborted\n");
						return -1;
					}
					break;
				case 'e':
				case 'E':
					if (++i >= argc || !sscanf(argv[i], "%d", &g_errMargin) || (g_errMargin < 0 || g_errMargin > 255))
					{
						printf("[ERROR] Invalid or absent Error Margin value but flag was raised! Operation aborted\n");
						return -1;
					}
					break;
				case 'r':
				case 'R':
					g_errIsRelative = 1;
					break;
				}
				break;
			default:
				g_isDefaultName = 0;
				outputIndex = i;
				break;
			}
		}
		if (modeFast)
		{
			fastMode(argv[1], (outputIndex ? argv[outputIndex] : OUTPUT));
			return 0;
		}
		verboseMode(argv[1], (outputIndex ? argv[outputIndex] : OUTPUT));
		return 0;
	}
	fastMode(argv[1], OUTPUT);											//No flags and no output name
	return 0;
}