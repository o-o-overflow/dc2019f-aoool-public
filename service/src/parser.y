%skeleton "lalr1.cc" /* -*- C++ -*- */
%require "3.0"
%defines
%define parser_class_name { Parser }

// NOTE: This has been written by heavily adapting code from
// https://github.com/ezaquarii/bison-flex-cpp-example
// Props to ezaquarii!

%define api.token.constructor
%define api.value.type variant
%define parse.assert false
%define api.namespace { aoool }
%code requires
{
    #include <iostream>
    #include <string>
    #include <vector>
    #include <stdint.h>
    #include "utils.h"
    #include "config.h"
    #include "osl.h"

    using namespace std;

    namespace aoool {
        class Scanner;
        class Driver;
    }
}

// Bison calls yylex() function that must be provided by us to suck tokens
// from the scanner. This block will be placed at the beginning of IMPLEMENTATION file (cpp).
// We define this function here (function! not method).
// This function is called only inside Bison, so we make it static to limit symbol visibility for the linker
// to avoid potential linking conflicts.
%code top
{
    #include <iostream>
    #include "scanner.h"
    #include "parser.hpp"
    #include "parserdriver.h"
    
    // yylex() arguments are defined in parser.y
    static aoool::Parser::symbol_type yylex(aoool::Scanner &scanner, aoool::Driver &driver) {
        return scanner.get_next_token();
    }
    
    // you can accomplish the same thing by inlining the code using preprocessor
    // x and y are same as in above static function
    // #define yylex(x, y) scanner.get_next_token()
    
    using namespace aoool;
}

%lex-param { aoool::Scanner &scanner }
%lex-param { aoool::Driver &driver }
%parse-param { aoool::Scanner &scanner }
%parse-param { aoool::Driver &driver }
%locations
%define parse.trace false
%define parse.error simple

%define api.token.prefix {TOKEN_}

%token END 0 "end of file"
%token MAIN "main";
%token ROOT "root";
%token LOG "log";
%token MODE "mode";
%token TEXT "text";
%token OSL "osl";
%token SERVERNAME "servername";
%token SERVER "server";
%token LOCATION "location";
%token PRINT "print";
%token DEL "del";
%token <std::string> STRING  "string";
%token <std::string> STRINGVALUE  "stringvalue";
%token <uint64_t> NUMBER "number";
%token LEFTPAR "leftpar";
%token RIGHTPAR "rightpar";
%token LEFTCPAR "leftcpar";
%token RIGHTCPAR "rightcpar";
%token EQUAL "equal";
%token SEMICOLON "semicolon";
%token COMMA "comma";

%type< aoool::MainConfNode* > main_node;
%type< aoool::ServerConfNode* > server_node;
%type< aoool::LocationConfNode* > location_node;
%type< std::vector<ServerConfNode*> > server_nodes_opt;
%type< std::vector<ServerConfNode*> > server_nodes;
%type< std::vector<LocationConfNode*> > location_nodes_opt;
%type< std::vector<LocationConfNode*> > location_nodes;
%type< aoool::StringWrapper > server_name;
%type< aoool::StringWrapper > root_node_opt;
%type< aoool::StringWrapper > log_node_opt;
%type< aoool::ProcessMode > mode_node_opt;

%type< std::vector<aoool::OslStatement*> > osl_statements;
%type< aoool::OslStatement* > osl_statement;
%type< aoool::OslExpr* > osl_expr;
%type< aoool::OslStatement* > osl_assign_statement;
%type< aoool::OslStatement* > osl_print_statement;
%type< aoool::OslStatement* > osl_print_log_statement;
%type< aoool::OslStatement* > osl_del_statement;

%left MINUS PLUS
%left ASTERISK
%nonassoc UMINUS
%left EQUAL

%start entry_point

%%

entry_point : config_node | osl_program;

config_node : main_node  {
          MainConfNode* main_node = $1;
          driver.setConfMainConfNode(main_node);
      }
      | server_node {
          ServerConfNode* server_node = $1;
          driver.setConfServerConfNode(server_node);
      }
      ;

osl_program : osl_statements
    {
        vector<OslStatement*> statements = $1;
        auto osl_program = new aoool::OslProgram(statements);
        driver.setOslProgram(osl_program);
    }
    ;

osl_statements : osl_statement
    {
        aoool::OslStatement* stmt = $1;
        $$ = std::vector<OslStatement*>();
        $$.push_back(stmt);
    }
    | osl_statements osl_statement
    {
        std::vector<aoool::OslStatement*> stmts = $1;
        aoool::OslStatement* stmt = $2;
        stmts.push_back(stmt);
        $$ = stmts;
    }
    ;

osl_statement : osl_assign_statement { $$ = $1; }
    | osl_print_statement { $$ = $1; }
    | osl_print_log_statement { $$ = $1; }
    | osl_del_statement { $$ = $1; }
    ;

osl_assign_statement : STRING EQUAL osl_expr SEMICOLON
    {
        std::string var_name = $1;
        OslExpr* osl_expr = $3;
        auto var = new aoool::OslVar(var_name);
        auto stmt = new aoool::OslAssignStmt(var, osl_expr);
        $$ = stmt;
    }
    ;

osl_expr : NUMBER { $$ = new OslExpr(new OslIntValue($1)); }
    | STRINGVALUE { 
        string val = $1;
        trim(val, '"');
        $$ = new OslExpr(new OslStrValue(val)); }
    | STRING { $$ = new OslExpr(new OslVar($1)); }
    | osl_expr PLUS osl_expr { $$ = new OslExpr(OslAddExprType, $1, $3); }
    | osl_expr MINUS osl_expr { $$ = new OslExpr(OslSubExprType, $1, $3); }
    | osl_expr ASTERISK osl_expr { $$ = new OslExpr(OslMulExprType, $1, $3); }
    | MINUS osl_expr %prec UMINUS { $$ = new OslExpr(OslMinusExprType, $2, NULL); }
    ;

osl_print_statement : PRINT STRING SEMICOLON
    {
        std::string var_name = $2;
        auto var = new aoool::OslVar(var_name);
        auto stmt = new aoool::OslPrintStmt(var);
        $$ = stmt;
    }
    ;

osl_print_log_statement : PRINT LOG SEMICOLON
    {
        auto stmt = new aoool::OslPrintLogStmt();
        $$ = stmt;
    }
    ;

osl_del_statement : DEL STRING SEMICOLON
    {
        std::string var_name = $2;
        auto var = new aoool::OslVar(var_name);
        auto stmt = new aoool::OslDelStmt(var);
        $$ = stmt;
    }
    ;

main_node : MAIN LEFTCPAR root_node_opt log_node_opt mode_node_opt server_nodes_opt RIGHTCPAR
    {
        string &root_node = $3.s;
        string &log_node = $4.s;
        ProcessMode mode = $5;
        vector<ServerConfNode*> &servers = $6;

        if (mode == undefinedMode) mode = textMode;

        auto main_node = new MainConfNode(root_node, log_node, mode);
        for (auto &server : servers) {
            server->parent = main_node;
        }
        main_node->servers = servers;

        $$ = main_node;
    }
    ;

server_nodes_opt : { $$ = vector<ServerConfNode*>(); }
          | server_nodes { $$ = $1; }
      ;

server_nodes : server_node { $$ = vector<ServerConfNode*>(); $$.push_back($1); }
             | server_nodes server_node 
                {
                    vector<ServerConfNode*> &servers = $1;
                    servers.push_back($2);
                    $$ = servers;
                }
        ;

server_node : SERVER LEFTCPAR server_name root_node_opt log_node_opt mode_node_opt location_nodes_opt RIGHTCPAR
    {
        string &server_name = $3.s;
        string &root_node = $4.s;
        string &log_node = $5.s;
        ProcessMode mode = $6;
        vector<LocationConfNode*> &locations = $7;

        auto server_node = new ServerConfNode(server_name, root_node, log_node, mode);
        for (auto &location : locations) {
            location->parent = server_node;
        }
        server_node->locations = locations;

        $$ = server_node;
    }
    ;

server_name: SERVERNAME STRINGVALUE SEMICOLON
    {
        string &server_name = $2;
        trim(server_name, '"');
        $$ = StringWrapper(server_name);
    };

root_node_opt : { $$ = StringWrapper(""); }
           | ROOT STRINGVALUE SEMICOLON
              {
                string &val = $2;
                trim(val, '"');
                $$ = StringWrapper(val);
              }
    ;

log_node_opt : { $$ = StringWrapper(""); }
           | LOG STRINGVALUE SEMICOLON
              {
                string &val = $2;
                trim(val, '"');
                $$ = StringWrapper(val);
              }
    ;

mode_node_opt : { $$ = aoool::undefinedMode; }
           | MODE TEXT SEMICOLON
              {
                $$ = aoool::textMode;
              }
           | MODE OSL SEMICOLON
              {
                $$ = aoool::oslMode;
              }
    ;

location_nodes_opt : { $$ = std::vector<LocationConfNode*>(); }
           | location_nodes
              {
                $$ = $1;
              }
    ;

location_nodes : location_node
              {
                LocationConfNode* locNode = $1;
                $$ = std::vector<LocationConfNode*>();
                $$.push_back(locNode);
              }
           | location_nodes location_node
              {
                LocationConfNode* locNode = $2;
                std::vector<LocationConfNode*> &locs = $1;
                locs.push_back(locNode);
                $$ = locs;
              }
    ;

location_node : LOCATION STRINGVALUE LEFTCPAR root_node_opt log_node_opt mode_node_opt RIGHTCPAR
      {
          string &val = $2;
          trim(val, '"');
          $$ = new LocationConfNode(val, $4.s, $5.s, $6);
      }
      ;

%%

// Bison expects us to provide implementation - otherwise linker complains
void aoool::Parser::error(const location &loc , const std::string &message) {
    DLOG("parsing error: %s\n", message.c_str());
}
