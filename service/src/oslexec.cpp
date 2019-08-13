#include <stdio.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include <string>
#include <fstream>

#include "handler.h"
#include "parserdriver.h"
#include "osl.h"
#include "osleval.h"

using namespace aoool;
using namespace std;


// util to execute OSL without the full aoool service


int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: %s OSL_FP LOG_FP\n", argv[0]);
        return 1;
    }

    Driver driver;
    auto oslProgram = driver.parseOslProgram(argv[1]);
    if (oslProgram == NULL) {
        fprintf(stderr, "bad osl program\n");
        return 1;
    }
    printf("Parsed OSL program: %s\n", oslProgram->toString().c_str());

    printf("Start OSL exec\n");
    RequestHandler dummy_rh;
    dummy_rh.log_fp = string(argv[2]);
    auto oe = new OslExecutor(dummy_rh);
    auto output = oe->eval(oslProgram);
    cerr << "output:" << endl;
    cerr << output << endl;
    cerr << "-------------------------" << endl;
    fprintf(stderr, "Finished OSL exec\n");
}
