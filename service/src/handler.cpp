#include "handler.h"

using namespace std;
using namespace aoool;


RequestHandler::RequestHandler() {
    root_dir = "";
    fp = "";
    log_fp = "";
    osl_output_fp = "";
    mode = undefinedMode;
    valid = false;
}


string RequestHandler::toString() const {
    string s;

#ifdef ENABLE_DLOG
    s = "RH(fp=" + fp + ", log_fp=" + log_fp + ", mode=";
    switch (this->mode) {
        case undefinedMode:
            s += "undef";
            break;
        case textMode:
            s += "text";
            break;
        case oslMode:
            s += "osl";
            break;
        default:
            s += "error";
            break;
    }
    s += ")";
#else
    s = "";
#endif

    return s;
}
