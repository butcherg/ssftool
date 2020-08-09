
//#include <stdio.h>
//#include <sys/select.h>
#include <stdarg.h> 
#include <iostream>
#include <string>
#include <vector>
#include <map>

#ifdef WIN32
#include <windows.h>
#include <io.h>
#else
#include <sys/select.h>
#endif


struct ssf_data {
	float w;
	std::vector<float> d;
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
	char buffer[256000];
	std::vector<std::string> lines;
	
#ifdef WIN32
	HANDLE stdinHandle = (HANDLE)_get_osfhandle(_fileno( f) );
	stdinHandle = GetStdHandle(STD_INPUT_HANDLE);
	if (WaitForSingleObject( stdinHandle, 1000 ) == WAIT_TIMEOUT) return lines;
#else
	fd_set set;
	struct timeval timeout;
	//use select() on first read to detect no data at stdin (terminal, not pipe)...
	FD_ZERO(&set);
	FD_SET(fileno(f), &set);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	if (select (FD_SETSIZE, &set, NULL, NULL, &timeout) == 0) return lines;
#endif

	fgets(buffer,256000, f);
	
	while (!feof(f)) {
		std::string line = std::string(buffer);
		line.erase(line.find_last_not_of(" \n\r\t")+1);
		lines.push_back(line);
		if (fgets(buffer,256000, f) == NULL) return lines;
	}

	return lines;
}

std::vector<ssf_data> get_Data(std::vector<std::string> lines)
{
	std::vector<ssf_data> data;
	for (std::vector<std::string>::iterator line = lines.begin(); line !=lines.end(); ++line) {
		if ((*line)[0] == '#') continue; //ignore comment lines
		ssf_data d;
		std::vector<std::string> tokens = split(*line, ",");
		if (tokens.size() < 2) err(string_format("get_Data error: line does not contain sufficient number of values (%s)",(*line).c_str()));
		d.w = atoi(tokens[0].c_str());
		d.d.push_back(atof(tokens[1].c_str()));
		if (tokens.size() >= 3) d.d.push_back(atof(tokens[2].c_str()));
		if (tokens.size() >= 4) d.d.push_back(atof(tokens[3].c_str()));

		data.push_back(d);
	}
	return data;
}

std::vector<ssf_data> sumData(std::vector<ssf_data> left, std::vector<ssf_data> right)
{
	if (left.size() == 0) return right;
	for (unsigned i=0; i<left.size(); i++) 
		left[i].d[i] += right[i].d[i];
	return left;
}

std::vector<ssf_data> divideData(std::vector<ssf_data> data, float divisor)
{
	for (unsigned i=0; i<data.size(); i++) 
		data[i].d[i] /= divisor;
	return data;
}

//todo: convert to ssf_data, maybe...
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

std::vector<channeldata> channelMaxes(std::vector<ssf_data> data)
{
	std::vector<channeldata> max;
	channeldata rmax = {0,0.0}, gmax= {0,0.0}, bmax= {0,0.0};
	for (unsigned i=0; i<data.size(); i++) {
		if (data[i].d.size() >=1) {
			if (data[i].d[0] > rmax.v) {
				rmax.v = data[i].d[0]; 
				rmax.p =  i;
			}
		}
		if (data[i].d.size() >=2) {
			if (data[i].d[1] > gmax.v) {
				gmax.v = data[i].d[1]; 
				gmax.p =  i;
			}
		}
		if (data[i].d.size() >=3) {
			if (data[i].d[2] > bmax.v) {
				bmax.v = data[i].d[2]; 
				bmax.p =  i;
			}
		}
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
		//if ((*line).find("x,") == 0 | (*line).find("red") == 0 | (*line).find("green") == 0 | (*line).find("blue") == 0)
		if ((*line).substr(0,1) == "x" | (*line).substr(0,3) == "red" | (*line).substr(0,5) == "green" | (*line).substr(0,4) == "blue")
			l.push_back(string_format("%s",(*line).c_str()));
	return l;
}

std::vector<std::string> data_transpose_old(std::vector<std::string> lines)
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

std::vector<std::string> data_transpose(std::vector<std::string> lines)
{
	std::vector<std::string> l;
	std::vector<std::vector<std::string> > data;
	
	for (std::vector<std::string>::iterator line = lines.begin(); line !=lines.end(); ++line) {
		std::vector<std::string> linedata = split(*line, ",");
		data.push_back(linedata);
	}
	
	for (unsigned i = 0; i < data[0].size(); i++) {
		//l.push_back(data[0][i]);
		//l.push_back(string_format("%d",i));
		for (unsigned j=0; j<data.size(); j++) {
			l.back().append("."+data[j][i]);
		}
	}
	return l;
}

//ssf_data'ed
std::vector<ssf_data> wavelengthcalibrate(std::vector<ssf_data> specdata, std::vector<channeldata> markers)
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

void print_ssfdata(std::vector<ssf_data> specdata)
{
	for (std::vector<ssf_data>::iterator dat = specdata.begin(); dat !=specdata.end(); ++dat) {
		printf("%0.2f", (*dat).w);
		for (unsigned i=0; i< (*dat).d.size(); i++) {
			printf(",%f", (*dat).d[i]);
		}
		printf("\n");
	}
}

void print_ssfline(ssf_data l)
{
	printf("%0.2f", l.w);
	for (unsigned i=0; i< l.d.size(); i++) {
		printf(",%f", l.d[i]);
	}
	printf("\n");
}


// Operations:

void ssf_list(FILE *f, bool wavelengths)
{
	std::vector<std::string> lines = getFile(f);
	if (wavelengths) {
		bool first = true;
		for (std::vector<std::string>::iterator line = lines.begin(); line !=lines.end(); ++line) {
			std::vector<std::string> v = split(*line, ",");
			if (first) {
				printf("%s",v[0].c_str());
				first = false;
			}
			else printf(",%s",v[0].c_str());
		}
		printf("\n");
	}
	else {
		for (std::vector<std::string>::iterator line = lines.begin(); line !=lines.end(); ++line)
			printf("%s\n",(*line).c_str());
	}
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
	std::vector<ssf_data> data = get_Data(lines);
	std::vector<channeldata> max =  channelMaxes(data);
	printf("blue:%f,%d;green:%f,%d;red:%f,%d\n", max[0].v, max[0].p, max[1].v, max[1].p, max[2].v, max[2].p);
}



//new routine, for x markers:
void ssf_wavelengthcalibrate(FILE *f, std::vector<channeldata> markers)
{
	std::vector<ssf_data> specdata = get_Data(getFile(f));
	specdata = wavelengthcalibrate(specdata, markers);
	print_ssfdata(specdata);
}

//old routine, for channelmax markers:
//ssf_data'ed
void ssf_wavelengthcalibrate(FILE *f, std::string calibrationfile, int bluewavelength, int greenwavelength, int redwavelength) //, int redx=0, int greenx=0, int bluex=0)
{
	FILE *c = fopen(calibrationfile.c_str(), "r");
	if (c == NULL) err(string_format("wavelength_callibrate error: wavelength calibration file %s not found.",calibrationfile.c_str()));
	std::vector<std::string> caliblines = getFile(c);
	fclose(c);
	std::vector<ssf_data> calibdata = get_Data(caliblines);
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

	std::vector<ssf_data> specdata = get_Data(getFile(f));
	
	//do the wavelength assignment:
	specdata = wavelengthcalibrate(specdata, marker);
	print_ssfdata(specdata);

}

void ssf_powercalibrate(FILE *f, std::string calibrationfile)
{
	std::vector<ssf_data> specdata = get_Data(getFile(f));
	FILE *c = fopen(calibrationfile.c_str(), "r");
	if (c == NULL) err(string_format("powercalibrate error: power calibration file %s not found.",calibrationfile.c_str()));
	std::vector<std::string> caliblines = getFile(c);
	fclose(c);
	std::map<int, float> calibdata = getPowerData(caliblines);

	for (std::vector<ssf_data>::iterator dat = specdata.begin(); dat !=specdata.end(); ++dat) {
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
		
		for (unsigned i=0; i< (*dat).d.size(); i++)
			(*dat).d[i] /= cab;
	}
	print_ssfdata(specdata);
}

//ssf_data'ed
void ssf_normalize(FILE *f)
{
	std::vector<ssf_data> specdata = get_Data(getFile(f));
	float maxval = 0.0;

	for (std::vector<ssf_data>::iterator dat = specdata.begin(); dat !=specdata.end(); ++dat)
		for (unsigned i=0; i< (*dat).d.size(); i++)
			if ((*dat).d[i] > maxval) maxval = (*dat).d[i];
		
	for (std::vector<ssf_data>::iterator dat = specdata.begin(); dat !=specdata.end(); ++dat)
		for (unsigned i=0; i< (*dat).d.size(); i++)
			(*dat).d[i] /= maxval;
		
	print_ssfdata(specdata);
	
}

//ssf_data'ed
void ssf_average(FILE *f)
{
	std::vector<ssf_data> specdata = get_Data(getFile(f));
	for (std::vector<ssf_data>::iterator dat = specdata.begin(); dat !=specdata.end(); ++dat) {
		float sum = 0.0;
		for (unsigned i=0; i< (*dat).d.size(); i++)
			sum += (*dat).d[i];
		printf("%0.2f,%f\n", (*dat).w, sum / (*dat).d.size() );			
	}
}

void ssf_linearpower(float lower, float upper, float interval, float lowval, float hival)
{
	float increment = (hival-lowval) / ((upper-lower)/interval);
	printf("%0.2f,%f\n", lower, lowval);
	float prev = lowval;
	for (float step = lower+interval; step < upper; step+= interval) {
		printf("%0.2f,%f\n", step, prev+increment);
		prev += increment;
	}
	printf("%0.2f,%f\n", upper, hival);
}


//ssf_data'ed
void ssf_intervalize(FILE *f, float lower, float upper, float interval)
{
	std::vector<ssf_data> specdata = get_Data(getFile(f));

	unsigned i = 0;
	float prev = specdata[0].w;
	
	if (specdata[0].w > lower) err(string_format("intervalize error: data doesn't cover lower bound: %0.2f",specdata[0].w));
	if (specdata[specdata.size()-1].w < upper) err(string_format("intervalize error: data doesn't cover upper bound: %0.2f",specdata[specdata.size()-1].w));

	for (float step = lower; step <= upper; step+= interval) {
		while (i < specdata.size()-1 && specdata[i].w < lower) i++; // walk the data up to the lower bound

		if (specdata[i].w == step) { //if the datum is exactly the step, use it and go on to the next step
			print_ssfline(specdata[i]);
			i++;
			continue;
		}
		else if (specdata[i].w < step) {
			while (i < specdata.size()-1 && specdata[i].w < step) i++;
		}

		ssf_data dat;
		dat.w = step;
		float interp;
		if (i == specdata.size() - 1) { //data is now at its end, need to interpolate with previous data interval and finish
			interp = (specdata[i].w - specdata[i-1].w) / (specdata[i].w - step);
			for (unsigned j=0; j<specdata[i].d.size(); j++) 
				dat.d.push_back(specdata[i].d[j]); //nearest neighbor, hack
				//dat.d[j] = specdata[i].d[j] + (specdata[i].d[j] * interp)... //todo: interpolation
			print_ssfline(dat);
			return;
		}
		else { //interpolate with the current interval
			interp = (specdata[i+1].w - specdata[i].w) / (specdata[i].w - step);
			for (unsigned j=0; j<specdata[i].d.size(); j++) 
				if (interp > 0.5) //nearest neighbor, hack
					dat.d.push_back(specdata[i+1].d[j]);
				else
					dat.d.push_back(specdata[i].d[j]);
				//dat.d[j] = specdata[i].d[j]... //todo: interpolation
			print_ssfline(dat);
			i++;
		}

		//else { //walk the data up to where its w is >= the step
		//	while (i < specdata.size() && specdata[i].w < step) i++;
		//}
	}
	
}

//ssf_data'ed
void ssf_dcamprofjson(FILE *f, std::string cameraname)
{
	std::vector<ssf_data> specdata = get_Data(getFile(f));
	std::vector<std::string> w, r, g, b;
	for (std::vector<ssf_data>::iterator dat = specdata.begin(); dat !=specdata.end(); ++dat) {
		w.push_back(string_format("%d", (int) (*dat).w));
		r.push_back(string_format("%f", (*dat).d[0]));
		g.push_back(string_format("%f", (*dat).d[1]));
		b.push_back(string_format("%f", (*dat).d[2]));
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

//ssf_data'ed
void ssf_format(FILE *f, int precision=2)
{
	std::vector<ssf_data> specdata = get_Data(getFile(f));
	for (std::vector<ssf_data>::iterator dat = specdata.begin(); dat !=specdata.end(); ++dat) {
		printf("%d", int((*dat).w));
		for (unsigned i=0; i< (*dat).d.size(); i++) {
			printf(",%0.*f", precision, (*dat).d[i]);
		}
		printf("\n");
	}
}

void ssf_smooth(FILE *f, int lookback=2)
{
	std::vector<ssf_data> specdata = get_Data(getFile(f));
	std::vector<ssf_data> smoothdata;
	for (unsigned i=0; i<specdata.size(); i++) {
		int lb;
		if (i < lookback) lb = i; else lb = lookback;
		ssf_data s;
		s.w = specdata[i].w;
		for (unsigned j=0; j<specdata[i].d.size(); j++) {
			float sum = 0.0;
			for (unsigned k=0; k<lb; k++)
				sum += specdata[i-k].d[j];
			s.d.push_back(sum / (float) (lb));
		}
		smoothdata.push_back(s);
	}	
	print_ssfdata(smoothdata);
}

// here's a ssftool command to process soup-to-nuts, using bash process substitution to input the calibration file to wavelengthcalibrate (Yeow!):
// $ ssftool extract DSG_4583-spectrum.csv | ssftool transpose | ssftool wavelengthcalibrate blue=437,green=546,red=611 <(ssftool extract DSG_4582-calibration.csv | ssftool transpose)  | ssftool intervalize 400,730,5 | ssftool powercalibrate Dedolight_5nm.csv | ssftool normalize

int main(int argc, char ** argv)
{
	FILE *f;

	if (argc <= 1) {
		printf("Usage:\n\n"); 
		printf("ssftool list [<datafile>] ['wavelengths']  - prints the data file. \n'wavelengths' prints just the wavelenghts as a comma-separated list\n\n");
		printf("ssftool extract [<datafile>] - extracts data from a rawproc data file.\n\n");
		printf("ssftool transpose [<datafile>] - turns a row-major file into column-major.\n\n");
		printf("ssftool channelmaxes [<datafile>] - calculates the pixel locations of each of \nthe channel maximum values.\n\n");
		printf("ssftool wavelengthcalculate [<datafile>] markerstring [<calibrationfile>] -  \ncalibrate either using a markerstring of \"red=www,green=www,blue=www\" to a \ncalibration file or \"position=wavelength...\"\n\n");
		printf("ssftool powercalibrate [<datafile>] [<calibrationfile]> - divide each value in \nthe datafile by the corresponding value from the calibration file.\n\n");
		printf("ssftool normalize [<datafile>] - normalizes the data to the range 0.0-1.0 based \non the largest channel maximum.\n\n");
		printf("ssftool intervalize <lowerbound>,<upperbound>,<interval> [<datafile>] - \ncollapses the data to the range specified by lowerbound, upperbound, \nand interval.\n\n");
		printf("ssftool averagechannels [<datafile>] - averages the r, g, and b values of \neach line to produce a single value for the line.\n\n");
		printf("ssftool averagefiles [<datafile>][...] - averages the r, g, and b values \nfrom each file to form a single r, g, and b for each line.\n\n"); 
		printf("ssftool format [<datafile>] <precision> - formats the w,r,g,b file to \ninteger-ize the w, and round each r, g, and b to the specified precision.\n\n"); 
		printf("ssftool dcamprofjson [<datafile>] - produces a JSON format from the w,r,g,b \ndata that can be ingested by dcamprof.\n\n");
		printf("\n");
		exit(1);
	}
	std::string operation = std::string(argv[1]);
	
	if (operation == "list") {
		if (argc <= 2 || std::string(argv[2]) == "wavelengths") {
			f = stdin; 
		}
		else {
			f = fopen(argv[2], "r");
		} 
		if (f == NULL) err("Error: data file not found.");
		bool w = false;
		if (std::string(argv[argc-1]) == "wavelengths") w = true; 
		ssf_list(f, w);
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
		std::vector<ssf_data> data = get_Data(getFile(stdin));
		if (data.size() > 0) count++;
		for (unsigned i = 2; i<argc; i++) {
			f = fopen(argv[i], "r");
			if (f) 
				data = sumData(data, get_Data(getFile(f)));
			else
				err(string_format("averagefiles error: data file not found: %s",argv[i]));
			fclose(f);
			count++;
		}
		data = divideData(data, count);
		print_ssfdata(data);
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
	else if (operation == "format") {
		int precision = 2;
		if (argc == 3) {
			f = stdin; 
			precision = atoi(argv[2]);
		}
		else if (argc == 4) {
			f = fopen(argv[2], "r"); 
			if (f == NULL) err(string_format("format error: file not found: %s",argv[2]));
			precision = atoi(argv[3]);
		}
		 else err(string_format("format error: wrong number of parameters: %d", argc));

		if (f == NULL) err(string_format("format error: data file not found: %s",argv[2]));
		ssf_format(f, precision);
		fclose(f);	
	}
	else if (operation == "smooth") { //todo: add to usage
		int lookback = 2;
		if (argc == 2) { //ssftool smooth
			f = stdin;
		}
		else if (argc == 3) { //ssftool smooth file|lookback
			f = fopen(argv[2], "r"); 
			if (f == NULL) { 
				f = stdin; 
				lookback = atoi(argv[2]);
			}
		}
		else if (argc == 4) { //ssftool smooth file lookback
			f = fopen(argv[2], "r"); 
			if (f == NULL) err(string_format("smooth error: file not found: %s",argv[2]));
			lookback = atoi(argv[3]);
		}
		else err(string_format("smooth error: wrong number of parameters: %d", argc));

		if (f == NULL) err(string_format("smooth error: data file not found: %s",argv[2]));
		ssf_smooth(f, lookback);
		fclose(f);	
	}
	else if (operation == "linearpower") {  //todo: add to usage
		if (argc == 3) {
			std::string range = std::string(argv[2]);
			std::vector<std::string> r = split(range, ",");
			if (r.size() < 5) err("linearpower error: not enough parameters in the range specification:"+range);
			float lower = atof(r[0].c_str());
			float upper = atof(r[1].c_str());
			float interval = atof(r[2].c_str());
			float lowval = atof(r[3].c_str());
			float hival = atof(r[4].c_str());
		
			ssf_linearpower(lower, upper, interval, lowval, hival);
		}
		else err(string_format("linearpower error: wrong number of parameters: %d", argc));
	}
	else printf("%s", string_format("ssf error: unrecognized operation: %s.\n",operation.c_str()).c_str()); fflush(stdout);
	
	

}
