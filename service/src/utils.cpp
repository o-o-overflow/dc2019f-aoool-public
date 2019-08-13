#include "utils.h"

#include <fstream>

using namespace std;

// from https://stackoverflow.com/a/217605
// trim from end (in place)
void rtrim(string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

void trim(string &s, char c) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [c](char ch) {
            return (ch != c);
    }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [c](char ch) {
        return (ch != c);
    }).base(), s.end());
}

// from https://stackoverflow.com/a/236803
template<typename Out>
void split(const std::string &s, char delim, Out result) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}
std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}


string read_line() {
    // Read a string from stdin until EOF or a \n. The final \n is NOT removed.
    // This function returns a std::string.

    char buffer[MAX_LINE_LEN+1];

    memset(buffer, 0, MAX_LINE_LEN+1);

    size_t i=0;
    int n;
    while (i<MAX_LINE_LEN) {
        n = fread(&buffer[i], 1, 1, stdin);
        if (n != 1) {
            // fread failed, let's err out
            DLOG("fread fail\n");
            exit(1);
        }
        i++;
        if (buffer[i-1] == '\n') {
            break;
        }
    }
    // no new line found, but we have read too much anyways
    buffer[i] = '\0';

    string ret(buffer);

    return ret;
}


string read_n(size_t n) {
    string res(n, '\0');
    cin.read(&res[0], n);
    return res;
}


string read_all_from_stream(istream &in)
{
    string ret;
    char buffer[4096];
    while (in.read(buffer, sizeof(buffer)))
        ret.append(buffer, sizeof(buffer));
    ret.append(buffer, in.gcount());
    return ret;
}


bool starts_with(string& str, const char prefix[]) {
    string prefixStr(prefix);
    if(str.substr(0, prefixStr.size()) == prefixStr) {
        return true;
    }
    return false;
}


string gen_random(size_t len) {
    static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    ifstream urandom("/dev/urandom", ios::in|ios::binary);
    string s = "";
    for (size_t i=0; i<len; i++) {
        char c;
        uint8_t idx;
        urandom.read(&c, 1);
        // for some reason, sizeof(alphanum) == 63...
        idx = ((uint8_t) c) % (sizeof(alphanum)-1);
        s += alphanum[idx];
    }

    return s;
}


void hexdump(unsigned char* buf, size_t buflen) {
    size_t i, j;
    for (i=0; i<buflen; i+=16) {
        for (j=0; j<16; j++) {
            if (i+j < buflen) {
                fprintf(stderr, "%02x ", buf[i+j]);
            }
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");
}
