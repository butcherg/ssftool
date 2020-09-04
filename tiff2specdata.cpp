#include <stdio.h>
#include <string.h>
#include "tiffio.h"

#include <string>

struct pix {
	unsigned short r, g, b;
};

void err(std::string msg)
{
	fprintf(stderr,"%s\n",msg.c_str());
	fflush(stdout);
	exit(1);
}

int main(int argc, char ** argv)
{
	char *img, *buf;
	FILE * infile;
	std::string filename;
	uint32 w, h;
	uint16 c, b, config;
	bool debug = false;
	
	unsigned bgX = 0, bgY = 0, gtop=0, gbottom = 0;
	unsigned top, left, bottom, right;
	float bg = 0.0;
	unsigned short rmax=0, gmax=0, bmax=0;
	
	unsigned band=100; 
	float greenthreshold=0.7;

	TIFFSetErrorHandler(0);

	if (argc < 2) err("Error: no file specified.");
	if (argc == 3)
		if (std::string(argv[2]) == "debug")
			debug = true;
	
	filename = std::string(argv[1]);
	
	TIFF* tif = TIFFOpen(filename.c_str(), "r");
	if (tif) {
		
		TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
		TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
		TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &c);
		TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &b);
		TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &config);
		
		if (config != PLANARCONFIG_CONTIG) err("Error: TIFF file not planar.");
		if (c != 3) err("Error: image not RGB.");
		if (b != 16) err("Error: image not 16-bit integer");
		
		img = new char[w*h*c*(b/8)];
		buf = (char *) _TIFFmalloc(TIFFScanlineSize(tif));
		int stride = TIFFScanlineSize(tif);
		
		char * dst = (char *) img;
		for (unsigned y = 0; y < h; y++){
			TIFFReadScanline(tif, buf, y, 0);
			memcpy(dst,buf,stride);
			dst += stride;
		}
		
		pix * image = (pix *) img;
		
		//collect channel maxes:
		for (unsigned i=0; i< w*h; i++) {
			if (image[i].r > rmax) rmax = image[i].r;
			if (image[i].g > gmax) gmax = image[i].g;
			if (image[i].b > bmax) bmax = image[i].b;
		}
		
		//compute thresholds for finding the brightest green:
		unsigned gthreshold = gmax * 0.7;
		unsigned rthreshold = rmax * 0.5;
		unsigned bthreshold = bmax * 0.5;
		
		if (debug) printf("rmax: %u gmax: %u bmax: %u\n",rmax,gmax,bmax);
		if (debug) printf("rthreshold: %u gthreshold: %u bthreshold: %u\n",rthreshold,gthreshold,bthreshold);
		
		//find the x,y of the brightest green, maxes of each channel:
		for (unsigned x=0; x<w; x++) {
			for (unsigned y=0; y<h; y++) {
				unsigned pos = x + y*w;
				if (image[pos].g > bg & image[pos].r < rthreshold & image[pos].b < bthreshold) {
					bg = image[pos].g;
					bgX = x;
					bgY = y;
				}
			}
		}
		if (debug) printf("largest green: %d,%d\n",bgX,bgY);

		//determine the band top and bottom by walking the brightest green column:
		{
			unsigned x = bgX;
			bool inband = false;
			for (unsigned y=0; y<h; y++) {
				unsigned pos = x + y*w;
				if (image[pos].g > gthreshold & !inband) {gtop = y; inband = true;}
				else if (image[pos].g < gthreshold &  inband) {gbottom = y; break;}
			}
		}
		
		//compute the band (crop) extents extents from the center of the detected band:
		unsigned bandcenter = gtop + (gbottom - gtop)/2;
		top = bandcenter - band/2;
		left = 0;
		bottom = bandcenter + band/2;
		right = w;
		
		if (debug) printf("left,top,right,bottom: %d,%d,%d,%d\n",left,top,right,bottom);

		/*
		//use the crop algorithm to print the channelaverages.  
		//This also transposes the data to column-major:
		for (unsigned x=left; x<right; x++) {
			double rsum = 0, gsum=0.0, bsum = 0.0;
			for (unsigned y=top; y<bottom; y++) {
				unsigned pos = x + y*w;
				rsum += image[pos].r;
				gsum += image[pos].g;
				bsum += image[pos].b;
			}
			if (!debug) printf("%d,%0.0f,%0.0f,%0.0f\n",x,rsum/band,gsum/band,bsum/band);
		}
		*/

		//use the crop algorithm to print the channelaverages.  
		//This also transposes the data to column-major, and flips the data horizontally:
		for (unsigned x=right; x>left; x--) {
			double rsum = 0, gsum=0.0, bsum = 0.0;
			for (unsigned y=top; y<bottom; y++) {
				unsigned pos = x + y*w;
				rsum += image[pos].r;
				gsum += image[pos].g;
				bsum += image[pos].b;
			}
			if (!debug) printf("%d,%0.0f,%0.0f,%0.0f\n",right-x,rsum/band,gsum/band,bsum/band);
		}
		
		if (buf) _TIFFfree(buf);
		TIFFClose(tif);
	}
	else err("Error: file failed to open.");
	
}