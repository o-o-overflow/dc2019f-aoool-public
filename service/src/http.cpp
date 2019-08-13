#include "http.h"

#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>


using namespace std;

namespace aoool {


bool is_valid_http_line(std::string& line) {
    if (line.length() < 2) return false;
    if ( (line.at(line.length()-2) != '\r') || (line.at(line.length()-1) != '\n') ) {
        return false;
    }
    return true;
}


void handle_http_request(Driver& driver, MainConfNode* conf, HttpMethod method, string& path) {
    string host = "";
    size_t content_len = 0;
    vector<string> lines;
    while (true) {
        string curr_line = read_line();
        if (!is_valid_http_line(curr_line)) {
            http_send_bad_request();
            return;
        }
        rtrim(curr_line);

        DLOG("handle_http_request: line: %s\n", curr_line.c_str());

        if (curr_line.length() == 0) break;

        lines.push_back(curr_line);

        if (starts_with(curr_line, "Host: ")) {
            vector<string> tokens = split(curr_line, ' ');
            if (tokens.size() != 2) {
                http_send_bad_request();
                return;
            }
            host = tokens[1];
        }

        if (starts_with(curr_line, "Content-Length: ")) {
            vector<string> tokens = split(curr_line, ' ');
            if (tokens.size() != 2) {
                http_send_bad_request();
                return;
            }
            content_len = stoi(tokens[1]);
        }
    }

    if (host == "") {
        http_send_bad_request();
        return;
    }
    if (host.find(":") != string::npos) {
        // discard the port when matching against server_name
        host = split(host, ':')[0];
    }

    string data;
    if (content_len != 0) {
        data = read_n(content_len);
    }

    switch(method) {
        case GetHttpMethod:
            handle_get_request(driver, conf, host, path);
            break;
        case UploadFileHttpMethod:
            handle_upload_file_request(data);
            break;
        case UploadConfigHttpMethod:
            handle_upload_conf_request(driver, conf, data);
            break;
        default:
            http_send_bad_request();
            break;
    }
}


void handle_get_request(Driver& driver, MainConfNode* conf, string& host, string& path) {

    DLOG("handle_get_request: host=%s, path=%s\n", host.c_str(), path.c_str());

    RequestHandler rh = get_request_handler(conf, host, path);

    bool success;
    if (!rh.valid) {
        // note: proper http response has already been sent
        success = false;
    } else if (rh.mode == textMode) {
        success = http_send_file(rh.fp);
        if (!success) {
            http_send_not_found();
        }
    } else if (rh.mode == oslMode) {
        string fp = rh.fp;

        auto osl_program = driver.parseOslProgram(fp);
        if (osl_program == NULL) {
            http_send_bad_request();
            return;
        }

        auto oe = new OslExecutor(rh);
        string output = oe->eval(osl_program);

        DLOG("osleval output (len: %lu) %s\n", output.length(), output.c_str());

        success = true;
        http_send_ok(output);
    } else {
        success = false;
        http_send_bad_request();
    }

    log_request(GetHttpMethod, host, path, rh, success);
}


void handle_upload_file_request(string& data) {
    string fn = gen_random(10);
    string fp = "/aoool/var/www/uploads/" + fn;
    auto data_len = data.length();

    DLOG("handle_upload_file_request: fn=%s\n", fn.c_str());
    DLOG("------------- DATA ------------\n");
    DLOG("%s\n", data.c_str());
    DLOG("---------- END DATA -----------\n");

    if (boost::filesystem::exists(fp)) {
        DLOG("file %s already exists\n", fp.c_str());
        http_send_bad_request();
        return;
    }

    FILE* f = fopen(fp.c_str(), "wb");
    if (f != NULL) {

        size_t n = fwrite(data.c_str(), 1, data_len, f);
        fclose(f);

        if (data_len != n) {
            DLOG("could not write all bytes\n");
            http_send_bad_request();
            return;
        }

        http_send_ok(fn);
    } else {
        DLOG("error while opening file\n");
        http_send_bad_request();
        return;
    }
}


void handle_upload_conf_request(Driver& driver, MainConfNode* conf, string& data) {
    DLOG("got the fllowlgin data: \n");
    DLOG("%s\n", data.c_str());
    DLOG("----------------------------\n");
    auto server_node = driver.parseConfServerConfNodeFromString(data);
    if (server_node) {
        conf->servers.insert(conf->servers.begin(), server_node);
        DLOG("new server node: %s\n", server_node->toString().c_str());
        http_send_ok();
    } else {
        DLOG("new server node: NULL\n");
        http_send_bad_request();
    }
}


RequestHandler get_request_handler(MainConfNode* conf, string& host, string& path) {

    RequestHandler rh;

    // check for invalid / unsafe path
    if (path.find("..") != string::npos) {
        DLOG("error: invalid path %s\n", path.c_str());
        http_send_bad_request();
        return rh;
    }

    string root_dir = conf->root;
    string log_fp = conf->log;
    ProcessMode mode = conf->mode;

    // find appropriate server node
    ServerConfNode* matching_server = NULL;
    for (auto& server : conf->servers) {
        DLOG("checking server %s against %s\n", server->server_name.c_str(), host.c_str());
        boost::regex server_name_regex{server->server_name.c_str()};
        if ( (host == server->server_name) || (boost::regex_match(host, server_name_regex)) ) {
            DLOG("matched with server %p with server_name %s\n", server, server->server_name.c_str());
            if (!server->root.empty()) root_dir = server->root;
            if (!server->log.empty()) log_fp = server->log;
            if (server->mode != undefinedMode) mode = server->mode;
            matching_server = server;
            break;
        }
    }

    if (matching_server == NULL) {
        // no suitable server was found. Don't know how to handle this.
        DLOG("error: did not find any matching server\n");
        http_send_not_found();
        return rh;
    }

    // find appropriate location node (if any)
    LocationConfNode *matching_location = NULL;
    for (auto& loc : matching_server->locations) {
        boost::regex location_regex{loc->location.c_str()};
        if ( (path == loc->location) || (boost::regex_match(path, location_regex)) ) {
            DLOG("matched with location %p with loc %s\n", loc, loc->location.c_str());
            if (!loc->root.empty()) root_dir = loc->root;
            if (!loc->log.empty()) log_fp = loc->log;
            if (loc->mode != undefinedMode) mode = loc->mode;
            matching_location = loc;
            break;
        }
    }
    if (matching_location == NULL) {
        DLOG("did not find matching location\n");
    }


    // handle root_dir

    if (root_dir.empty()) {
        DLOG("error: root dir is empty\n");
        http_send_not_found();
        return rh;
    }
    rh.root_dir = root_dir;


    // handle log_fp

    if (log_fp.empty()) {
        log_fp = "default.log";
        DLOG("log directive not found, defaulting to %s\n", log_fp.c_str());
    }

    if (log_fp.find("..") != string::npos) {
        // Note: the .. is rejected only at GET time, but it is accepted in the config
        DLOG("bad log_fp: %s\n", log_fp.c_str());
        http_send_bad_request();
        return rh;
    }

    boost::filesystem::path log_path(log_fp);
    boost::filesystem::path abs_log_path;
    if (!log_path.is_absolute()) {
        abs_log_path = boost::filesystem::path("/aoool/var/log/") / log_path;
    } else {
        abs_log_path = log_path;
    }

    // check that it points to a non-existing or regular file
    if (boost::filesystem::exists(abs_log_path) && !boost::filesystem::is_regular_file(abs_log_path)) {
        DLOG("file %s exists and it is not a regular file\n", abs_log_path.string().c_str());
        http_send_bad_request();
        return rh;
    }

    auto abs_log_dir = abs_log_path.parent_path();
    auto abs_log_dir_str = abs_log_dir.string();
    // check that it starts with the expected log path
    if (!starts_with(abs_log_dir_str, "/aoool/var/log")) {
        DLOG("abs_log_dir %s is not the expected one\n", abs_log_dir.string().c_str());
        http_send_bad_request();
        return rh;
    }

    // check that it is a dir
    if (!boost::filesystem::is_directory(abs_log_dir)) {
        DLOG("abs_log_dir %s is not a dir\n", abs_log_dir.string().c_str());
        http_send_bad_request();
        return rh;
    }

    DLOG("log_fp: %s\n", log_fp.c_str());
    DLOG("log_path: %s\n", log_path.string().c_str());
    DLOG("abs_log_path: %s\n", abs_log_path.string().c_str());
    DLOG("abs_log_dir: %s\n", abs_log_dir.string().c_str());

    rh.log_fp = log_fp;


    // handle mode

    if (mode == undefinedMode) {
        DLOG("error: undefined mode\n");
        http_send_bad_request();
        return rh;
    }
    rh.mode = mode;

    string fp = web_server_root + root_dir + path;
    if (path == "/") {
        fp += "index.html";
    }

    DLOG("root_dir: %s\n", root_dir.c_str());
    DLOG("log_fp: %s\n", log_fp.c_str());
    DLOG("mode: %d\n", mode);
    DLOG("serving fp %s\n", fp.c_str());

    // Note: trivial path traversal works, but we check:
    // 1) that the file is a normal file (not a dir, not a symlink)
    // 2) there is no "flag" in the file path
    //
    // Note 2: the path traveral is possible by putting the .. in the root
    // directive in the config file. It should not be possible to do path
    // traveral without uploading a custom config first.

    if (!boost::filesystem::exists(fp)) {
        DLOG("error: %s does not exist\n", fp.c_str());
        http_send_not_found();
        return rh;
    }

    if (!boost::filesystem::is_regular_file(fp)) {
        DLOG("error: %s is not a regular file\n", fp.c_str());
        http_send_bad_request();
        return rh;
    }

    if (fp.find("flag") != string::npos) {
        DLOG("error: %s contains flag.\n", fp.c_str());
        http_send_bad_request();
        return rh;
    }

    rh.fp = fp;

    rh.valid = true;

    DLOG("request handler: %s\n", rh.toString().c_str());

    return rh;
}


void http_send_ok() {
    DLOG("sending OK\n");
    fprintf(stdout, "HTTP/1.1 200 OK\r\n");
    fprintf(stdout, "Server: aoool\r\n");
    fprintf(stdout, "Content-Type: text/html; charset=UTF-8\r\n");
    fprintf(stdout, "Content-Length: 0\r\n");
    fprintf(stdout, "Connection: close\r\n");
    fprintf(stdout, "\r\n");
}


void http_send_ok(const char content[], size_t len) {
    DLOG("sending OK (with content)\n");
    fprintf(stdout, "HTTP/1.1 200 OK\r\n");
    fprintf(stdout, "Server: aoool\r\n");
    fprintf(stdout, "Content-Type: text/html; charset=UTF-8\r\n");
    fprintf(stdout, "Content-Length: %lu\r\n", len);
    fprintf(stdout, "Connection: close\r\n");
    fprintf(stdout, "\r\n");
    fwrite(content, len, 1, stdout);
}


void http_send_ok(string &content) {
    http_send_ok(content.c_str(), content.length());
}


bool http_send_file(string &fp) {
    DLOG("sending file %s\n", fp.c_str());

    bool success = false;

    FILE* f = fopen(fp.c_str(), "rb");
    char *buf;
    if (f != NULL) {
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

        buf = (char *) calloc(fsize + 1, 1);
        memset(buf, 0, fsize+1);
        size_t n = fread(buf, 1, fsize, f);
        buf[n] = '\0';
        fclose(f);
        http_send_ok(buf, fsize);
        free(buf);
        success = true;
    } else {
        success = false;
    }

    return success;
}


void http_send_bad_request() {
    DLOG("sending bad request\n");
    fprintf(stdout, "HTTP/1.1 400 Bad Request\r\n");
    fprintf(stdout, "Server: aoool\r\n");
    fprintf(stdout, "Content-Type: text/html; charset=UTF-8\r\n");
    fprintf(stdout, "Content-Length: 0\r\n");
    fprintf(stdout, "Connection: close\r\n");
    fprintf(stdout, "\r\n");
}


void http_send_not_found() {
    DLOG("sending not found\n");
    fprintf(stdout, "HTTP/1.1 404 Not Found\r\n");
    fprintf(stdout, "Server: aoool\r\n");
    fprintf(stdout, "Content-Type: text/html; charset=UTF-8\r\n");
    fprintf(stdout, "Content-Length: 0\r\n");
    fprintf(stdout, "Connection: close\r\n");
    fprintf(stdout, "\r\n");
}


string http_method_to_str(HttpMethod method) {
    switch(method) {
        case GetHttpMethod:
            return string("GET");
        case UploadFileHttpMethod:
            return string("UF");
        case UploadConfigHttpMethod:
            return string("UC");
        default:
            throw "not supported";
    }
}


void log_request(HttpMethod method, string &host, string &path, RequestHandler &rh, bool success) {
    boost::filesystem::path log_path(rh.log_fp);
    boost::filesystem::path abs_log_path;
    if (!log_path.is_absolute()) {
        abs_log_path = boost::filesystem::path("/aoool/var/log/") / log_path;
    } else {
        abs_log_path = log_path;
    }
    auto log_fp = abs_log_path.string();

    DLOG("logging request to %s\n", log_fp.c_str());

    ofstream ofs(log_fp, ios::out | ios::binary | ios::app);
    if (ofs.is_open()) {
        ofs << http_method_to_str(method) << " " << host << " " << path << ": ";
        if (success) {
            ofs << "OK";
        } else {
            ofs << "ERROR";
        }
        ofs << endl;
        ofs.close();
    } else {
        DLOG("error logging request\n");
    }
}

}
