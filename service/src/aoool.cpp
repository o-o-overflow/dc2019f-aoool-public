#include <stdio.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include <string>
#include <fstream>

#include "utils.h"
#include "http.h"
#include "config.h"
#include "osl.h"
#include "parserdriver.h"
#include "osleval.h"

using namespace aoool;
using namespace std;

static string default_config_fp = "/aoool/etc/default";


int main() {
    setbuf(stdin, 0);
    setbuf(stdout, 0);
    setbuf(stderr, 0);

    // load config file
    Driver driver;
    MainConfNode* conf = driver.parseConfMainConfNode(default_config_fp);

    if (conf == NULL) {
        DLOG("bad config\n");
        return 1;
    }

    DLOG("loaded default config: %s\n", conf->toString().c_str());

    while (true) {
        DLOG("waiting for connections...\n");

        string firstLine = read_line();

        DLOG("Got new connection. First line: %s\n", firstLine.c_str());

        if (!is_valid_http_line(firstLine)) {
            http_send_bad_request();
            continue;
        }

        rtrim(firstLine);

        vector<string> tokens = split(firstLine, ' ');
        if (tokens.size() != 3) {
            http_send_bad_request();
            continue;
        }

        string methodStr = tokens[0];
        string path = tokens[1];
        string protocol = tokens[2];

        HttpMethod method = UndefinedHttpMethod;

        if (methodStr.compare("GET") == 0) method = GetHttpMethod;
        else if (methodStr.compare("UF") == 0) method = UploadFileHttpMethod;
        else if (methodStr.compare("UC") == 0) method = UploadConfigHttpMethod;
        else {
            http_send_bad_request();
            continue;
        }

        if (protocol.compare("HTTP/1.1") == 0) {
            handle_http_request(driver, conf, method, path);
        } else {
            http_send_bad_request();
        }
    }

    DLOG("quitting\n");
}
