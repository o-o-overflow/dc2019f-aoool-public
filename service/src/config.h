#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <stdint.h>

namespace aoool {

using namespace std;

enum ConfNodeType { mainNodeType, serverNodeType, locationNodeType };
enum ProcessMode { undefinedMode, textMode, oslMode };

class MainConfNode;
class ServerConfNode;
class LocationConfNode;

class ConfNode {
    public:
        ConfNodeType type;
        string root;
        string log;
        ProcessMode mode;

    ConfNode(ConfNodeType type) {
        this->type = type;
        root = "";
        log = "";
        mode = undefinedMode;
    }

    ~ConfNode() {}
};

class LocationConfNode : public ConfNode {
    public:
        string location;
        ServerConfNode *parent;

    LocationConfNode() : ConfNode(locationNodeType) {}
    ~LocationConfNode() {}

    LocationConfNode(string location) : ConfNode(locationNodeType) {
        this->location = location;
    }

    LocationConfNode(string location, string root, string log, ProcessMode mode) : ConfNode(locationNodeType) {
        this->location = location;
        this->root = root;
        this->log = log;
        this->mode = mode;
    }

    string toString() const {
        string s;

#ifdef ENABLE_DLOG
        s = "\t\tLocationConfNode(loc=" + this->location+ ", root=" + this->root + ", log=" + this->log + ", mode=";
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

};

class ServerConfNode : public ConfNode {
    public:
        string server_name;
        vector<LocationConfNode*> locations;
        MainConfNode *parent;

    ServerConfNode() : ConfNode(serverNodeType) {}
    ~ServerConfNode() {}

    ServerConfNode(string server_name) : ConfNode(serverNodeType) {
        this->server_name = server_name;
    }

    ServerConfNode(string server_name, string root, string log, ProcessMode mode) : ConfNode(serverNodeType) {
        this->server_name = server_name;
        this->root = root;
        this->log = log;
        this->mode = mode;
    }

    string toString() const {
        string s;

#ifdef ENABLE_DLOG
        s = "\tServerConfNode(sn=" + this->server_name + ", root=" + this->root + ", log=" + this->log + ", mode=";
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
        for (auto const& location : locations) {
            s += "\n" + location->toString();
        }
        s += ")";
#else
        s = "";
#endif

        return s;
    }

};

class MainConfNode : public ConfNode {
    public:
        vector<ServerConfNode*> servers;

    MainConfNode() : ConfNode(mainNodeType) {}
    ~MainConfNode() {}

    MainConfNode(string root, string log, ProcessMode mode) : ConfNode(mainNodeType) {
        this->root = root;
        this->log = log;
        this->mode = mode;
    }

    string toString() const {
        string s;

#ifdef ENABLE_DLOG
        s = "MainConfNode(root=" + this->root + ", log=" + this->log + ", mode=";
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
        for (auto const& server : servers) {
            s += "\n" + server->toString();
        }
        s += ")";
#else
        s = "";
#endif

        return s;
    }

};

class StringWrapper {
    public:
        string s;

    StringWrapper() {}
    ~StringWrapper() {}
    StringWrapper(string s) {
        this->s = s;
    }
};

}

#endif // CONFIG_H
