#ifndef HANDLER_H
#define HANDLER_H

#include <cstdio>
#include <string>

#include "config.h"

namespace aoool {

using namespace std;

class RequestHandler {
public:
    RequestHandler();
    string toString() const;

    string root_dir;
    string fp;
    string log_fp;
    string osl_output_fp;
    ProcessMode mode;
    bool valid;
};

}

#endif // HANDLER_H
