
//#include <stdio.h>
#include <stdarg.h> 
#include <iostream>
#include <string>
#include <vector>
#include <map>


struct ssfdata {
	unsigned w; 
	float r, g, b;
};

struct channeldata {
	unsigned p;	//x pixel coordinate
	float v; 	//max value
	int w;		//assigned wavelength
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
	std::cout << msg << std::endl;
	exit(1);
}

std::vector<std::string> getFile(FILE *f)
{
	char buffer[256000];
	std::vector<std::string> lines;
	while (!feof(f)) {
		fgets(buffer,256000, f);
		std::string line = std::string(buffer);
		line.erase(line.find_last_not_of(" \n\r\t")+1);
		lines.push_back(line);
	}
	return lines;
}

std::vector<ssfdata> getData(std::vector<std::string> lines)
{
	std::vector<ssfdata> data;
	for (std::vector<std::string>::iterator line = lines.begin(); line !=lines.end(); ++line) {
		ssfdata d;
		std::vector<std::string> tokens = split(*line, ",");
		if (tokens.size() < 4) err("Error: line does not contain sufficient number of values");
		d.w = atoi(tokens[0].c_str());
		d.r = atof(tokens[1].c_str());
		d.g = atof(tokens[2].c_str());
		d.b = atof(tokens[3].c_str());
		data.push_back(d);
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
	max.push_back(rmax);
	max.push_back(gmax);
	max.push_back(bmax);
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
	printf("red:%f,%d;green:%f,%d;blue:%f,%d\n", max[0].v, max[0].p, max[1].v, max[1].p, max[2].v, max[2].p);
}

void ssf_wavelengthcalibrate(FILE *f, std::string calibrationfile, int bluewavelength, int greenwavelength, int redwavelength)
{
	FILE *c = fopen(calibrationfile.c_str(), "r");
	if (c == NULL) err(string_format("Error: calibration file %s not found.",calibrationfile.c_str()));
	std::vector<std::string> caliblines = getFile(c);
	caliblines = channel_extract(caliblines);
	caliblines = data_transpose(caliblines);
	std::vector<ssfdata> calibdata = getData(caliblines);
	std::vector<channeldata> maxes =  channelMaxes(calibdata);
	
	(redwavelength != 0) ? maxes[0].w = redwavelength : maxes[0].w = 0;
	(greenwavelength != 0) ? maxes[1].w = greenwavelength : maxes[1].w = 0;
	(bluewavelength != 0) ? maxes[2].w = bluewavelength : maxes[2].w = 0;
	
	std::vector<channeldata> max;
	for (std::vector<channeldata>::iterator ch = maxes.begin(); ch != maxes.end(); ++ch)
		if ((*ch).w != 0) max.push_back(*ch);

	if (max.size() < 2) err("Error: need at least two channels");
	
	std::vector<ssfdata> specdata = getData(getFile(f));
	
	for (unsigned i=max.size()-1; i>0; i--) 
		//max[i].s = ((float) max[i].p - (float) max[i-1].p) / ((float) max[i].w - (float) max[i-1].w);
		max[i].s =  ((float) max[i].w - (float) max[i-1].w) / ((float) max[i].p - (float) max[i-1].p);
	//max[0].s = ((float) max[1].p - (float) max[0].p) / ((float) max[1].w - (float) max[0].w);
	max[0].s = ((float) max[1].w - (float) max[0].w) / ((float) max[1].p - (float) max[0].p);

	
	if (redwavelength != 0) specdata[max[0].p].w = redwavelength;
	if (greenwavelength != 0) specdata[max[1].p].w = greenwavelength;
	if (bluewavelength != 0) specdata[max[2].p].w = bluewavelength;
	
	//for (std::vector<ssfdata>::iterator dat = specdata.begin(); dat !=specdata.end(); ++dat)
	//	printf("%d,%f,%f,%f\n", (*dat).w, (*dat).r, (*dat).g, (*dat).b);

	//printf("bluewavelength:%d greenwavelength:%d redwavelength:%d\n", bluewavelength,greenwavelength,redwavelength);
	for (std::vector<channeldata>::iterator ch = max.begin(); ch != max.end(); ++ch)
		printf("p:%d, v:%f  w:%d s:%f\n", (*ch).p, (*ch).v, (*ch).w, (*ch).s); 
}


int main(int argc, char ** argv)
{
	FILE *f;

	if (argc <= 1) err("Usage: ssftool <operation> [<datafile>] [parameters...]"); 
	std::string operation = std::string(argv[1]);
	
	
	
	if (f == NULL) err("file not found.");
	
	if (operation == "list") {
		if (argc <= 2) f = stdin; else f = fopen(argv[2], "r"); 
		ssf_list(f);
	}
	else if (operation == "extract") {
		if (argc <= 2) f = stdin; else f = fopen(argv[2], "r"); 
		ssf_extract(f);
	}
	else if (operation == "transpose") {
		if (argc <= 2) f = stdin; else f = fopen(argv[2], "r"); 
		ssf_transpose(f);
	}
	else if (operation == "channelmaxes") {
		if (argc <= 2) f = stdin; else f = fopen(argv[2], "r"); 
		ssf_channelmaxes(f);
	}
	else if (operation == "wavelengthcalibrate") {
		std::string calibfile;
		std::vector<std::string> wavelength;
		if (argc == 4) {
			f = stdin; 
			calibfile = std::string(argv[2]);
			wavelength = split(std::string(argv[3]), ",");
		}
		else if (argc == 5) {
			f = fopen(argv[2], "r"); 
			calibfile = std::string(argv[3]);
			wavelength = split(std::string(argv[4]), ",");
		}
		else err("Error: wrong number of parameters for wavelengthcalibrate");
		
		int redw=0, greenw=0, bluew=0;
		for (unsigned i=0; i<wavelength.size(); i++) {
			std::vector<std::string> nameval = split(wavelength[i], "=");
			if (nameval[0] == "red") redw = atoi(nameval[1].c_str());
			else if (nameval[0] == "green") greenw = atoi(nameval[1].c_str());
			else if (nameval[0] == "blue") bluew = atoi(nameval[1].c_str());
			else err("Error: bad wavelength parameter");
		}

		ssf_wavelengthcalibrate(f, calibfile, bluew, greenw, redw);
	}
	else printf("Error: unrecognized operation.");
	
	fclose(f);

}
