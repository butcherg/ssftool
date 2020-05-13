
//#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>


struct ssfdata {
	unsigned w; 
	float r, g, b;
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

std::vector<std::string> channel_extract(std::vector<std::string> lines)
{
	std::vector<std::string> l;
	for (std::vector<std::string>::iterator line = lines.begin(); line !=lines.end(); ++line)
		if ((*line).find("red") == 0 | (*line).find("green") == 0 | (*line).find("blue") == 0)
			printf("%s\n",(*line).c_str());
	return l;
}

std::map<std::string, std::vector<std::string>> data_transpose(std::vector<std::string> lines)
{
	std::map<std::string, std::vector<std::string>> data;
	for (std::vector<std::string>::iterator line = lines.begin(); line !=lines.end(); ++line) {
		std::vector<std::string> linedata = split(*line, ",");
		std::string key = linedata[0];
		linedata.erase(linedata.begin());
		data[key] = linedata;
	}
	return data;
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
	std::map<std::string, std::vector<std::string>> data = data_transpose(lines);
	for (unsigned i = 0; i < data["red"].size(); i++)
		printf("%d,%s,%s,%s\n", i, data["red"][i].c_str(), data["green"][i].c_str(), data["blue"][i].c_str()); 
}

void ssf_wavelengthcalibrate(FILE *f, std::string calibrationfile, int redwavelength, int greenwavelength, int bluewavelength=0)
{
	FILE *c = fopen(calibrationfile.c_str(), "r");
	if (c == NULL) err("Error: calibration file not found.");
	std::vector<ssfdata> calib = getData(getFile(c));
	
	std::vector<ssfdata> data = getData(getFile(f));
	
}


int main(int argc, char ** argv)
{
	FILE *f;

	if (argc < 2) err("Usage: ssftool <operation> [<datafile>] [parameters...]"); 
	std::string operation = std::string(argv[1]);
	
	if (argc < 3)
		f = stdin;
	else
		f = fopen(argv[2], "r"); 
	
	if (f == NULL) err("file not found.");
	
	if (operation == "list")
		ssf_list(f);
	else if (operation == "extract")
		ssf_extract(f);
	else if (operation == "transpose")
		ssf_transpose(f);
	else printf("Error: unrecognized operation.");
	
	fclose(f);

}