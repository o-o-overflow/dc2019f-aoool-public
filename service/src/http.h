#ifndef HTTP_H
#define HTTP_H

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string>

#include "utils.h"
#include "config.h"
#include "parserdriver.h"
#include "handler.h"
#include "osl.h"
#include "osleval.h"

using namespace std;

namespace aoool {

static string web_server_root = "/aoool/var/www/";

enum HttpMethod { UndefinedHttpMethod, GetHttpMethod, UploadFileHttpMethod, UploadConfigHttpMethod };

bool is_valid_http_line(string& line);

void handle_http_request(Driver& driver, MainConfNode* conf, HttpMethod method, string& path);
void handle_get_request(Driver& driver, MainConfNode* conf, string& host, string& path);
void handle_upload_file_request(string& data);
void handle_upload_conf_request(Driver& driver, MainConfNode* conf, string& data);

RequestHandler get_request_handler(MainConfNode* conf, string& host, string& path);

void http_send_ok();
void http_send_ok(const char content[], size_t len);
void http_send_ok(string &content);
bool http_send_file(string &fp);
void http_send_bad_request();
void http_send_not_found();

string http_method_to_str(HttpMethod method);

void log_request(HttpMethod method, string &host, string &path, RequestHandler &rh, bool success);

}

#endif // HTTP_H
