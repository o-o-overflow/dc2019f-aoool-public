#ifndef CONFIGDRIVER_H
#define CONFIGDRIVER_H

#include <vector>
#include <fstream>
#include <iostream>
#include <string>

#include "scanner.h"
#include "config.h"
#include "osl.h"

#include "parser.hpp"

namespace aoool {

enum ParserMode { UndefinedParserMode, ConfigParserMainConfNodeMode, ConfigParserServerConfNodeMode, OslParserMode };

class Driver
{
public:
    Driver() : 
        m_parser_mode(UndefinedParserMode),
        m_conf_main_node(NULL),
        m_conf_server_node(NULL),
        m_osl_program(NULL)
    {

    }

    MainConfNode* parseConfMainConfNode(std::string fp) {
        Scanner scanner(*this);
        Parser parser(scanner, *this);
        
        m_parser_mode = ConfigParserMainConfNodeMode;

        std::ifstream ifs(fp);
        scanner.switch_streams(&ifs, NULL);
        int ret = parser.parse();
        auto conf = getConfMainConfNode();
        if ( (ret == 0) && (conf != NULL) ) {
            return conf;
        } else {
            return NULL;
        }
    }

    ServerConfNode* parseConfServerConfNode(std::string fp) {
        Scanner scanner(*this);
        Parser parser(scanner, *this);
        
        m_parser_mode = ConfigParserServerConfNodeMode;

        std::ifstream ifs(fp);
        scanner.switch_streams(&ifs, NULL);
        int ret = parser.parse();
        auto conf = getConfServerConfNode();
        if ( (ret == 0) && (conf != NULL) ) {
            return conf;
        } else {
            return NULL;
        }
    }

    ServerConfNode* parseConfServerConfNodeFromString(std::string& data) {
        Scanner scanner(*this);
        Parser parser(scanner, *this);
        
        m_parser_mode = ConfigParserServerConfNodeMode;

        std::istringstream data_stream(data);

        scanner.switch_streams(&data_stream, NULL);
        int ret = parser.parse();
        auto conf = getConfServerConfNode();
        if ( (ret == 0) && (conf != NULL) ) {
            return conf;
        } else {
            return NULL;
        }
    }

    OslProgram* parseOslProgram(std::string fp) {
        Scanner scanner(*this);
        Parser parser(scanner, *this);
        
        m_parser_mode = OslParserMode;

        std::ifstream ifs(fp);
        scanner.switch_streams(&ifs, NULL);
        int ret = parser.parse();
        auto osl_program = getOslProgram();
        if ( (ret == 0) && (osl_program != NULL) ) {
            return osl_program;
        } else {
            return NULL;
        }
    }

    void setConfMainConfNode(MainConfNode* conf) {
        m_conf_main_node = conf;
    }

    void setConfServerConfNode(ServerConfNode* conf) {
        m_conf_server_node = conf;
    }

    void setOslProgram(OslProgram* osl_program) {
        m_osl_program = osl_program;
    }

    ParserMode getParserMode() {
        return m_parser_mode;
    }

    friend class Parser;
    friend class Scanner;
    
private:
    MainConfNode* getConfMainConfNode() {
        if (m_parser_mode != ConfigParserMainConfNodeMode) {
            return NULL;
        }
        return m_conf_main_node;
    }

    ServerConfNode* getConfServerConfNode() {
        if (m_parser_mode != ConfigParserServerConfNodeMode) {
            return NULL;
        }
        return m_conf_server_node;
    }

    OslProgram* getOslProgram() {
        if (m_parser_mode != OslParserMode) {
            return NULL;
        }
        return m_osl_program;
    }

    ParserMode m_parser_mode;
    MainConfNode* m_conf_main_node;
    ServerConfNode* m_conf_server_node;
    OslProgram* m_osl_program;
};

}

#endif // CONFIGDRIVER_H
