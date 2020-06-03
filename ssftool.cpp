
//#include <stdio.h>
#include <sys/select.h>
#include <stdarg.h> 
#include <iostream>
#include <string>
#include <vector>
#include <map>


struct ssfdata {
	float w; 
	float r, g, b;
};

struct channeldata {
	unsigned p;	//x pixel coordinate
	float v; 	//max value
	float w;	//assigned wavelength
	float s; 	//slope, for calculating other wavelengths
};

// Helpers:

int countchar(std::string s, char c)
{
	int count = 0;
	for (int i=0; i<s.size(); i++) {
		if (s[i] == c) count++;
	}
	return count;
}

std::vector<std::string> split(std::string s, std::string delim)
{
	std::vector<std::string> v;
	if (s.find(delim) == std::string::npos) {
		v.push_back(s);
		return v;
	}
	size_t pos=0;
	size_t start;
	while (pos < s.length()) {
		start = pos;
		pos = s.find(delim,pos);
		if (pos == std::string::npos) {
			v.push_back(s.substr(start,s.length()-start));
			return v;
		}
		v.push_back(s.substr(start, pos-start));
		pos += delim.length();
	}
	return v;
}

std::vector<std::string> bifurcate(std::string strg, char c = ' ', bool fromback=false)
{
	std::vector<std::string> result;
	if (countchar(strg, c) == 0) {
		result.push_back(strg);
	}
	else {
		std::size_t eq;
		if (fromback)
			eq = strg.find_last_of(c);
		else
			eq = strg.find_first_of(c);
		result.push_back(strg.substr(0,eq));
		result.push_back(strg.substr(eq+1));
	}
	return result;
}

std::string string_format(const std::string fmt, ...) 
{
    int size = ((int)fmt.size()) * 2 + 50;   // Use a rubric appropriate for your code
    std::string str;
    va_list ap;
    while (1) {     // Maximum two passes on a POSIX system...
        str.resize(size);
        va_start(ap, fmt);
        int n = vsnprintf((char *)str.data(), size, fmt.c_str(), ap);
        va_end(ap);
        if (n > -1 && n < size) {  // Everything worked
            str.resize(n);
            return str;
        }
        if (n > -1)  // Needed size returned
            size = n + 1;   // For null char
        else
            size *= 2;      // Guess at a larger size (OS specific)
    }
    return str;
}

void err(std::string msg)
{
	fprintf(stderr,"%s\n",msg.c_str());
	fflush(stdout);
	exit(1);
}

std::vector<std::string> getFile(FILE *f)
{
	fd_set set;
	struct timeval timeout;
	char buffer[256000];
	std::vector<std::string> lines;
	
	//use select() on first read to detect no data at stdin (terminal, not pipe)...
	FD_ZERO(&set);
	FD_SET(fileno(f), &set);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	if (select (FD_SETSIZE, &set, NULL, NULL, &timeout) == 0) return lines;
	fgets(buffer,256000, f);
	
	while (!feof(f)) {
		std::string line = std::string(buffer);
		line.erase(line.find_last_not_of(" \n\r\t")+1);
		lines.push_back(line);
		if (fgets(buffer,256000, f) == NULL) return lines;
	}
	return lines;
}

std::vector<ssfdata> getData(std::vector<std::string> lines)
{
	std::vector<ssfdata> data;
	for (std::vector<std::string>::iterator line = lines.begin(); line !=lines.end(); ++line) {
		if ((*line)[0] == '#') continue; //ignore comment lines
		ssfdata d;
		std::vector<std::string> tokens = split(*line, ",");
		if (tokens.size() < 4) err(string_format("getData error: line does not contain sufficient number of values (%s)",(*line).c_str()));
		d.w = atoi(tokens[0].c_str());
		d.r = atof(tokens[1].c_str());
		d.g = atof(tokens[2].c_str());
		d.b = atof(tokens[3].c_str());
		data.push_back(d);
	}
	return data;
}

std::vector<ssfdata> sumData(std::vector<ssfdata> left, std::vector<ssfdata> right)
{
	if (left.size() == 0) return right;
	for (unsigned i=0; i<left.size(); i++) {
		left[i].r += right[i].r;
		left[i].g += right[i].g;
		left[i].b += right[i].b;
	}
	return left;
}

std::vector<ssfdata> divideData(std::vector<ssfdata> data, float divisor)
{
	for (unsigned i=0; i<data.size(); i++) {
		data[i].r /= divisor;
		data[i].g /= divisor;
		data[i].b /= divisor;
	}
	return data;
}

std::map<int, float> getPowerData(std::vector<std::string> lines)
{
	std::map<int, float> data;
	for (std::vector<std::string>::iterator line = lines.begin(); line !=lines.end(); ++line) {
		float v;
		int w;
		std::vector<std::string> tokens = split(*line, ",");
		if (tokens.size() < 2) err(string_format("getPowerData error: line does not contain sufficient number of values (%s)",(*line).c_str()));
		w = atoi(tokens[0].c_str());
		v = atof(tokens[1].c_str());
		data[w] = v;
	}
	return data;
}

std::vector<channeldata> channelMaxes(std::vector<ssfdata> data)
{
	std::vector<channeldata> max;
	channeldata rmax = {0,0.0}, gmax= {0,0.0}, bmax= {0,0.0};
	for (unsigned i=0; i<data.size(); i++) {
		if (data[i].r > rmax.v) {rmax.v = data[i].r; rmax.p =  i;}
		if (data[i].g > gmax.v) {gmax.v = data[i].g; gmax.p =  i;}
		if (data[i].b > bmax.v) {bmax.v = data[i].b; bmax.p =  i;}
	}
	max.push_back(bmax);
	max.push_back(gmax);
	max.push_back(rmax);
	return max;
}

std::vector<std::string> channel_extract(std::vector<std::string> lines)
{
	std::vector<std::string> l;
	for (std::vector<std::string>::iterator line = lines.begin(); line !=lines.end(); ++line)
		if ((*line).find("red") == 0 | (*line).find("green") == 0 | (*line).find("blue") == 0)
			l.push_back(string_format("%s",(*line).c_str()));
	return l;
}

std::vector<std::string> data_transpose(std::vector<std::string> lines)
{
	std::vector<std::string> l;
	std::map<std::string, std::vector<std::string>> data;
	for (std::vector<std::string>::iterator line = lines.begin(); line !=lines.end(); ++line) {
		std::vector<std::string> linedata = split(*line, ",");
		std::string key = linedata[0];
		linedata.erase(linedata.begin());
		data[key] = linedata;
	}
	for (unsigned i = 0; i < data["red"].size(); i++)
		l.push_back(string_format("%d,%s,%s,%s", i, data["red"][i].c_str(), data["green"][i].c_str(), data["blue"][i].c_str())); 
	return l;
}

std::vector<ssfdata> ssf_wavelengthcalibrate(std::vector<ssfdata> specdata, std::vector<channeldata> markers)
{
	//1. compute slopes between the markers:
	for (unsigned i=0; i<markers.size()-1; i++) 
		markers[i].s = (float) (markers[i+1].w - markers[i].w) / (float) (markers[i+1].p - markers[i].p);
	markers[markers.size()-1].s = markers[markers.size()-2].s;
	
	//2. place marker wavelenghts in data:
	for (std::vector<channeldata>::iterator mkr = markers.begin(); mkr != markers.end(); ++mkr)
		specdata[(*mkr).p].w = (*mkr).w;

	//3. place wavelengths for each interval between calibration marker x-s 
	for (unsigned i=0; i<markers.size()-1; i++) {
		for (unsigned j=markers[i].p; j<markers[i+1].p; j++)
			specdata[j+1].w = specdata[j].w + markers[i].s;
	}
	
	//4. place wavelengths from the highest marker rgb x to the upper end of the spectrum
	for (unsigned j=markers[markers.size()-1].p; j<specdata.size()-1; j++)
		specdata[j+1].w = specdata[j].w + markers[markers.size()-1].s;
	specdata[specdata.size()-1].w = specdata[specdata.size()-2].w + markers[markers.size()-1].s;
	
	//5. place wavelengthsfrom the lowest marker to the lower end of the spectrum
	for (unsigned j=markers[0].p; j>0; j--)
		specdata[j-1].w = specdata[j].w - markers[0].s;
	specdata[0].w - specdata[1].w - markers[0].s;
	
	//print debug maxes;
	//for (std::vector<channeldata>::iterator ch = markers.begin(); ch != markers.end(); ++ch)
	//	printf("p:%d, v:%f  w:%d s:%f\n", (*ch).p, (*ch).v, (int) (*ch).w, (*ch).s); 
	
	return specdata;
}


// Operations:

void ssf_list(FILE *f)
{
	std::vector<std::string> lines = getFile(f);
	for (std::vector<std::string>::iterator line = lines.begin(); line !=lines.end(); ++line)
		printf("%s\n",(*line).c_str());
}

void ssf_extract(FILE *f)
{
	std::vector<std::string> lines = getFile(f);
	lines = channel_extract(lines);
	for (std::vector<std::string>::iterator line = lines.begin(); line !=lines.end(); ++line)
		printf("%s\n",(*line).c_str());
}

void ssf_transpose(FILE *f)
{
	std::vector<std::string> lines = getFile(f);
	lines = data_transpose(lines);
	for (std::vector<std::string>::iterator line = lines.begin(); line !=lines.end(); ++line)
		printf("%s\n",(*line).c_str());
}

void ssf_channelmaxes(FILE *f)
{
	std::vector<std::string> lines = getFile(f);
	std::vector<ssfdata> data = getData(lines);
	std::vector<channeldata> max =  channelMaxes(data);
	printf("blue:%f,%d;green:%f,%d;red:%f,%d\n", max[0].v, max[0].p, max[1].v, max[1].p, max[2].v, max[2].p);
}

void ssf_wavelengthcalibrate(FILE *f, std::vector<channeldata> markers)
{
	std::vector<ssfdata> specdata = getData(getFile(f));
	specdata = ssf_wavelengthcalibrate(specdata, markers);
	
	for (std::vector<ssfdata>::iterator dat = specdata.begin(); dat !=specdata.end(); ++dat)
		printf("%0.2f,%f,%f,%f\n", (*dat).w, (*dat).r, (*dat).g, (*dat).b);
}


void ssf_wavelengthcalibrate(FILE *f, std::string calibrationfile, int bluewavelength, int greenwavelength, int redwavelength) //, int redx=0, int greenx=0, int bluex=0)
{
	FILE *c = fopen(calibrationfile.c_str(), "r");
	if (c == NULL) err(string_format("wavelengthcallibrate error: wavelength calibration file %s not found.",calibrationfile.c_str()));
	std::vector<std::string> caliblines = getFile(c);
	fclose(c);
	std::vector<ssfdata> calibdata = getData(caliblines);
	std::vector<channeldata> markers =  channelMaxes(calibdata);
	
	//put wavelengths in markers:
	(redwavelength != 0) ? markers[2].w = redwavelength : markers[2].w = 0;
	(greenwavelength != 0) ? markers[1].w = greenwavelength : markers[1].w = 0;
	(bluewavelength != 0) ? markers[0].w = bluewavelength : markers[0].w = 0;
	
	//get rid of unused markers:
	std::vector<channeldata> marker;
	for (std::vector<channeldata>::iterator ch = markers.begin(); ch != markers.end(); ++ch)
		if ((*ch).w != 0) marker.push_back(*ch);

	//three is better, puts anchor at a middle marker...
	if (marker.size() < 2) err("wavelengthcalibrate error: need at least two channels");

	//calculate slopes:
	//for (unsigned i=0; i<marker.size()-1; i++) 
	//	marker[i].s = (float) (marker[i+1].w - marker[i].w) / (float) (marker[i+1].p - marker[i].p);
	//marker[marker.size()-1].s = marker[marker.size()-2].s;

	std::vector<ssfdata> specdata = getData(getFile(f));
	
	//do the wavelength assignment:
	specdata = ssf_wavelengthcalibrate(specdata, marker);

	//print production data;
	for (std::vector<ssfdata>::iterator dat = specdata.begin(); dat !=specdata.end(); ++dat)
		printf("%0.2f,%f,%f,%f\n", (*dat).w, (*dat).r, (*dat).g, (*dat).b);

	//print debug data, use in place of 'print production data' above:
	//for (unsigned i=0; i<specdata.size(); i++)
	//	printf("%d:%f,%f,%f,%f\n", i, specdata[i].w, specdata[i].r, specdata[i].g, specdata[i].b);

	//print debug maxes;
	//for (std::vector<channeldata>::iterator ch = max.begin(); ch != max.end(); ++ch)
	//	printf("p:%d, v:%f  w:%d s:%f\n", (*ch).p, (*ch).v, (int) (*ch).w, (*ch).s); 
}

void ssf_powercalibrate(FILE *f, std::string calibrationfile)
{
	std::vector<ssfdata> specdata = getData(getFile(f));
	FILE *c = fopen(calibrationfile.c_str(), "r");
	if (c == NULL) err(string_format("powercalibrate error: power calibration file %s not found.",calibrationfile.c_str()));
	std::vector<std::string> caliblines = getFile(c);
	fclose(c);
	std::map<int, float> calibdata = getPowerData(caliblines);

	for (std::vector<ssfdata>::iterator dat = specdata.begin(); dat !=specdata.end(); ++dat) {
		float cab;
		if (calibdata.find((*dat).w) != calibdata.end()) {  //exact match 
			cab = calibdata[(*dat).w];
		}
		else {  //interpolate
			auto upper = calibdata.lower_bound((*dat).w);
			auto lower = upper--;
			float mult = (float) ((*dat).w - lower->first) / (float)(upper->first - lower->first);
			cab = lower->second + (upper->second - lower->second) * mult;
		}
		
		(*dat).r /= cab;
		(*dat).g /= cab;
		(*dat).b /= cab;
		printf("%02f,%f,%f,%f\n", (*dat).w, (*dat).r, (*dat).g, (*dat).b);
	}
}

void ssf_normalize(FILE *f)
{
	std::vector<ssfdata> specdata = getData(getFile(f));
	std::vector<channeldata> max =  channelMaxes(specdata);
	float maxval = 0.0;
	for (std::vector<channeldata>::iterator ch = max.begin(); ch != max.end(); ++ch)
		if ((*ch).v > maxval) maxval = (*ch).v;
	for (std::vector<ssfdata>::iterator dat = specdata.begin(); dat !=specdata.end(); ++dat) {
		(*dat).r = (*dat).r / maxval;
		(*dat).g = (*dat).g / maxval;
		(*dat).b = (*dat).b / maxval;
		printf("%0.2f,%f,%f,%f\n", (*dat).w, (*dat).r, (*dat).g, (*dat).b);			
	}
}

void ssf_average(FILE *f)
{
	std::vector<ssfdata> specdata = getData(getFile(f));
	for (std::vector<ssfdata>::iterator dat = specdata.begin(); dat !=specdata.end(); ++dat) {
		printf("%0.2f,%f\n", (*dat).w, ((*dat).r + (*dat).g + (*dat).b) / 3.0 );			
	}
}

void ssf_intervalize(FILE *f, float lower, float upper, float interval)
{
	std::vector<ssfdata> specdata = getData(getFile(f));
	std::map<int,ssfdata> sd;
	//ToDo: median of available values at the same wavelength
	for (std::vector<ssfdata>::iterator dat = specdata.begin(); dat !=specdata.end(); ++dat) 
		sd[(int) (*dat).w] = *dat;
	
	for (unsigned i = lower; i<=upper; i+=interval)
		printf("%d,%f,%f,%f\n", i, sd[i].r, sd[i].g, sd[i].b);
	
}

void ssf_dcamprofjson(FILE *f, std::string cameraname)
{
	std::vector<ssfdata> specdata = getData(getFile(f));
	std::vector<std::string> w, r, g, b;
	for (std::vector<ssfdata>::iterator dat = specdata.begin(); dat !=specdata.end(); ++dat) {
		w.push_back(string_format("%d", (int) (*dat).w));
		r.push_back(string_format("%f", (*dat).r));
		g.push_back(string_format("%f", (*dat).g));
		b.push_back(string_format("%f", (*dat).b));
	}
	printf("{\n");

	printf("\t\"camera_name\": \"%s\",\n\n",cameraname.c_str());

	printf("\t\"ssf_bands\": [ ");
	printf("%s",w[0].c_str());
	for (unsigned i=1; i<b.size(); i++) printf(", %s",w[i].c_str());
	printf(" ],\n\n");

	printf("\t\"red_ssf\": [ ");
	printf("%s",r[0].c_str());
	for (unsigned i=1; i<r.size(); i++) printf(", %s",r[i].c_str());
	printf(" ],\n\n");

	printf("\t\"green_ssf\": [ ");
	printf("%s",g[0].c_str());
	for (unsigned i=1; i<r.size(); i++) printf(", %s",g[i].c_str());
	printf(" ],\n\n");

	printf("\t\"blue_ssf\": [ ");
	printf("%s",b[0].c_str());
	for (unsigned i=1; i<r.size(); i++) printf(", %s",b[i].c_str());
	printf(" ]\n\n");

	printf("}\n");
}

// here's a ssftool command to process soup-to-nuts, using bash process substitution to input the calibration file to wavelengthcalibrate (Yeow!):
//./ssftool extract DSG_4583-spectrum.csv | ./ssftool transpose | ./ssftool wavelengthcalibrate <(./ssftool extract DSG_4582-calibration.csv | ./ssftool transpose) blue=437,green=546,red=611 | ./ssftool intervalize 400,730,5 | ./ssftool powercalibrate Dedolight_5nm.csv | ./ssftool normalize

int main(int argc, char ** argv)
{
	FILE *f;

	if (argc <= 1) err("Usage: ssftool <operation> [<datafile>] [parameters...]"); 
	std::string operation = std::string(argv[1]);
	
	if (operation == "list") {
		if (argc <= 2) f = stdin; else f = fopen(argv[2], "r"); 
		if (f == NULL) err("Error: data file not found.");
		ssf_list(f);
		fclose(f);
	}
	else if (operation == "extract") {
		if (argc <= 2) f = stdin; else f = fopen(argv[2], "r"); 
		if (f == NULL)  err(string_format("extract error: data file not found: %s",argv[2]));
		ssf_extract(f);
		fclose(f);
	}
	else if (operation == "transpose") {
		if (argc <= 2) f = stdin; else f = fopen(argv[2], "r"); 
		if (f == NULL) err(string_format("transpose error: data file not found: %s",argv[2]));
		ssf_transpose(f);
		fclose(f);
	}
	else if (operation == "channelmaxes") {
		if (argc <= 2) f = stdin; else f = fopen(argv[2], "r"); 
		if (f == NULL)  err(string_format("channelmaxes error: data file not found: %s",argv[2]));
		ssf_channelmaxes(f);
		fclose(f);
	}
	else if (operation == "wavelengthcalibrate") { //wavelengthcalibrate [spectrumfile] markerstring [calibrationfile]
	
	/*
	ssftool wavelengthcalibrate spectrumfile markerstring //4, markers are position in 3 (check for '=' in 3)
	ssftool wavelengthcalibrate spectrumfile markerstring calibrationfile //5, markers are channel maxes in 3
	| ssftool wavelenghtcalibrate markerstring //3, markers are position in 2
	| ssftool wavelengthcalibrate markerstring calibrationfile //4, markers are channel maxes in 2 (check for '=' in 2)
	*/
		std::string calibfile;
		std::string markerstring;
		std::vector<std::string> markers;
		std::vector<channeldata> markerdat;
		if (argc == 3) {  //| ssftool wavelenghtcalibrate markerstring
			f = stdin;
			markerstring = argv[2];
		}
		else if (argc == 4) {
			if (std::string(argv[2]).find("=") != std::string::npos) {  // | ssftool wavelengthcalibrate markerstring calibrationfile
				f =  stdin;
				markerstring = argv[2];
				calibfile = argv[3];
			}
			else { // ssftool wavelengthcalibrate spectrumfile markerstring
				f = fopen(argv[2], "r");
				markerstring = argv[3];
			}
		}
		else if (argc == 5) {  //ssftool wavelengthcalibrate spectrumfile markerstring calibrationfile
			f = fopen(argv[2], "r"); 
			markerstring = argv[3];
			calibfile = argv[4];
		}
		else err(string_format("wavelengthcalibrate error: wrong number of parameters: %d",argc));
		if (f == NULL) err(string_format("wavelengthcalibrate error: data file not found: %s",argv[2]));
		
		markers = split(markerstring, ",");
		
		if (markerstring.find("red") != std::string::npos | markerstring.find("green") != std::string::npos | markerstring.find("blue") != std::string::npos) {
			//blue=437,green=546,red=611 - debug parameters
			int redw=0, greenw=0, bluew=0;
			for (unsigned i=0; i<markers.size(); i++) {
				std::vector<std::string> nameval = split(markers[i], "=");
				if (nameval[0] == "red") redw = atoi(nameval[1].c_str());
				else if (nameval[0] == "green") greenw = atoi(nameval[1].c_str());
				else if (nameval[0] == "blue") bluew = atoi(nameval[1].c_str());
				else err(string_format("wavelengthcalibrage error: bad marker parameter:",nameval[0].c_str()));
			}
			ssf_wavelengthcalibrate(f, calibfile, bluew, greenw, redw);
		}
		else { //use max channels for wavelength markers
			for (unsigned i=0; i<markers.size(); i++) {
				std::vector<std::string> nameval = split(markers[i], "=");
				channeldata d;
				d.p = atoi(nameval[0].c_str());
				d.w = atoi(nameval[1].c_str());
				markerdat.push_back(d);
			}
			ssf_wavelengthcalibrate(f, markerdat);
		}

		fclose(f);
	}
	else if (operation == "powercalibrate") {
		std::string calibfile;
		if (argc == 3) {
			f = stdin; 
			calibfile = std::string(argv[2]);
		}
		else if (argc == 4) {
			f = fopen(argv[2], "r"); 
			calibfile = std::string(argv[3]);
		}
		else err(string_format("powercalibrate error: wrong number of parameters: %d",argc));
		if (f == NULL) err(string_format("powercalibrate error: data file not found: %s",argv[2]));

		ssf_powercalibrate(f, calibfile);
		fclose(f);
	}
	else if (operation == "normalize") {
		if (argc <= 2) f = stdin; else f = fopen(argv[2], "r"); 
		if (f == NULL) err(string_format("normalize error: data file not found: %s",argv[2]));
		ssf_normalize(f);
		fclose(f);
	}
	else if (operation == "averagechannels") {  //produces a singe channel dataset of the r,g,b
		if (argc <= 2) f = stdin; else f = fopen(argv[2], "r"); 
		if (f == NULL) err(string_format("average error: data file not found: %s",argv[2]));
		ssf_average(f);
		fclose(f);
	}
	else if (operation == "averagefiles") { //produces a r,g,b dataset of the average of the input files
		unsigned count = 0;
		std::vector<ssfdata> data = getData(getFile(stdin));
		if (data.size() > 0) count++;
		for (unsigned i = 2; i<argc; i++) {
			f = fopen(argv[i], "r");
			if (f) 
				data = sumData(data, getData(getFile(f)));
			else
				err(string_format("averagefiles error: data file not found: %s",argv[i]));
			fclose(f);
			count++;
		}
		data = divideData(data, count);
		for (std::vector<ssfdata>::iterator dat = data.begin(); dat !=data.end(); ++dat)
			printf("%d,%f,%f,%f\n", (int) (*dat).w, (*dat).r, (*dat).g, (*dat).b);
		
	}
	else if (operation == "intervalize") {
		std::string range;
		if (argc == 3) {
			f = stdin; 
			range = std::string(argv[2]);
		}
		else if (argc == 4) {
			f = fopen(argv[2], "r"); 
			range = std::string(argv[3]);
		}

		else err(string_format("intervalize error: wrong number of parameters: %d",argc));
		if (f == NULL) err("Error: data file not found.");
		
		std::vector<std::string> r = split(range, ",");
		if (r.size() < 3) err("intervalize error: not enough parameters in the range specification:"+range);
		int lower = atof(r[0].c_str());
		int upper = atof(r[1].c_str());
		int interval = atof(r[2].c_str());
		
		ssf_intervalize(f, lower, upper, interval);
		fclose(f);
	}
	else if (operation == "dcamprofjson") {
		std::string cameraname;
		if (argc == 3) {
			f = stdin; 
			cameraname = std::string(argv[2]);
		}
		else if (argc == 4) {
			f = fopen(argv[2], "r"); 
			if (f == NULL) err(string_format("dcamprofjson error: file not found: %s",argv[2]));
			cameraname = std::string(argv[3]);
		}

		else err(string_format("dcamprofjson error: wrong number of parameters: %d", argc));
		if (f == NULL) err("Error: data file not found.");
		ssf_dcamprofjson(f, cameraname);
		fclose(f);
	}
	else printf("%s", string_format("ssf error: unrecognized operation: %s.\n",operation.c_str()).c_str()); fflush(stdout);
	
	

}
