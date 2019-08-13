#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <algorithm>
#include <iostream>
#include <vector>
#include <sstream>
#include <iterator>


#ifdef ENABLE_DLOG
#define DLOG(...) fprintf(stderr, "DEBUG: " __VA_ARGS__)
#else
#define DLOG(...)  // do nothing
#endif

#ifdef ENABLE_DDLOG // super verbose
#define DDLOG(...) fprintf(stderr, "DDEBUG: " __VA_ARGS__)
#else
#define DDLOG(...)  // do nothing
#endif

#define MAX_PATH_LEN 1000
#define MAX_HOST_LEN 100
#define MAX_LINES_NUM 1000
#define MAX_LINE_LEN 1000


void rtrim(std::string &s);
void trim(std::string &s, char c);
std::vector<std::string> split(const std::string &s, char delim);
std::string read_line();
std::string read_n(size_t n);
std::string read_all_from_stream(std::istream &in);
bool starts_with(std::string& str, const char prefix[]);
std::string gen_random(size_t len);
void hexdump(unsigned char* buf, size_t buflen);


#endif // UTILS_H
