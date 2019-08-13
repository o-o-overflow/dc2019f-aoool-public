#ifndef OSL_H
#define OSL_H

#include <iostream>
#include <string>
#include <vector>
#include <stdint.h>

namespace aoool {

using namespace std;

class OslProgram;
class OslStatement;
class OslAssignStmt;
class OslPrintStmt;
class OslPrintLogStmt;
class OslDelStmt;
class OslVar;
class OslValue;

enum OslStmtType {
    OslUndefStmtType,
    OslAssignStmtType,
    OslPrintStmtType,
    OslPrintLogStmtType,
    OslDelStmtType
};

enum OslExprType {
    OslUndefExprType,
    OslIntExprType,
    OslStrExprType,
    OslVarExprType,
    OslAddExprType,
    OslSubExprType,
    OslMulExprType,
    OslMinusExprType
};

enum OslValueType {
    OslIntValueType,
    OslStrValueType
};

class OslStatement {
public:
    OslStatement() :
        type(OslUndefStmtType)
    {}

    virtual string toString() = 0;

    OslStmtType type;
};


class OslVar {
public:
    OslVar(string &name) {
        this->name = name;
    }

    string toShortString() {
        string s = name;
        return s;
    }

    string toString() {
#ifdef ENABLE_DLOG
        string s = "OslVar(" + name + ")";
#else
        string s = "";
#endif
        return s;
    }

    string name;
};


class OslValue {
public:
    virtual string toShortString() = 0;
    virtual string toString() = 0;

    OslValueType type;
};


class OslIntValue : public OslValue {
public:
    OslIntValue(uint64_t value) {
        this->type = OslIntValueType;
        this->value = value;
    }

    virtual string toShortString() {
        string s = to_string(value);
        return s;
    }

    virtual string toString() {
#ifdef ENABLE_DLOG
        string s = "OslIntValue(" + to_string(value) + ")";
#else
        string s = "";
#endif
        return s;
    }

    uint64_t value;
};


class OslStrValue : public OslValue {
public:
    OslStrValue(string &value) {
        this->type = OslStrValueType;
        this->value = value;
    }

    virtual string toShortString() {
#ifdef ENABLE_DLOG
        string s = "\"" + value + "\"";
#else
        string s = "";
#endif
        return s;
    }

    virtual string toString() {
#ifdef ENABLE_DLOG
        string s = "OslStrValue(" + value + ")";
#else
        string s = "";
#endif
        return s;
    }

    string value;
};


class OslProgram {
public:
    OslProgram(vector<OslStatement*> &stmts) {
        for (auto &stmt : stmts) {
            statements.push_back(stmt);
        }
    }

    string toString() {
#ifdef ENABLE_DLOG
        string s = "OSL program with " + to_string(statements.size()) + " stmts:\n";
        for (auto &stmt : statements) {
            s += "\t" + stmt->toString() + "\n";
        }
#else
        string s = "";
#endif
        return s;
    }

    vector<OslStatement*> statements;
};


class OslExpr {
public:
    OslExpr() :
        type(OslUndefExprType),
        left(NULL),
        right(NULL),
        var(NULL),
        value(NULL)
    {}

    OslExpr(OslExprType type, OslExpr* left, OslExpr* right) : OslExpr() {
        this->type = type;
        this->left = left;
        this->right = right;
    }

    OslExpr(OslIntValue* value) : OslExpr() {
        this->type = OslIntExprType;
        this->value = value;
    }

    OslExpr(OslStrValue* value) : OslExpr() {
        this->type = OslStrExprType;
        this->value = value;
    }

    OslExpr(OslVar* var) : OslExpr() {
        this->type = OslVarExprType;
        this->var = var;
    }

    string toString() {
        string s;
#ifdef ENABLE_DLOG
        switch(type) {
            case OslIntExprType:
            case OslStrExprType:
                s = value->toShortString();
                break;
            case OslVarExprType:
                s = var->toShortString();
                break;
            case OslAddExprType:
                s = "(" + left->toString() + "+" + right->toString() + ")";
                break;
            case OslSubExprType:
                s = "(" + left->toString() + "-" + right->toString() + ")";
                break;
            case OslMulExprType:
                s = "(" + left->toString() + "*" + right->toString() + ")";
                break;
            case OslMinusExprType:
                s = "( -" + left->toString() + ")";
                break;
            default:
                s = "error";
                break;
        }
#else
        s = "";
#endif
        return s;

    }

    string typeToString() {
#ifdef ENABLE_DLOG
        switch(type) {
            case OslUndefExprType:
                return "undef";
            case OslIntExprType:
                return "int";
            case OslStrExprType:
                return "str";
            case OslVarExprType:
                return "var";
            case OslAddExprType:
                return "add";
            case OslSubExprType:
                return "sub";
            case OslMulExprType:
                return "mul";
            case OslMinusExprType:
                return "minus";
            default:
                  return "err";
        }
#else
        return "";
#endif
    }

    OslExprType type;
    OslExpr* left;
    OslExpr* right;
    OslVar* var;
    OslValue* value;
};


class OslAssignStmt : public OslStatement {
public:
    OslAssignStmt(OslVar* var, OslExpr* expr) : OslStatement() {
        this->type = OslAssignStmtType;
        this->var = var;
        this->expr = expr;
    }

    virtual string toString() {
#ifdef ENABLE_DLOG
        string s = "Assign: " + var->toString() + " = " + expr->toString() + ";";
#else
        string s = "";
#endif
        return s;
    }
    
    OslVar* var;
    OslExpr* expr;
};


class OslPrintStmt : public OslStatement {
public:
    OslPrintStmt(OslVar* var) : OslStatement() {
        this->type = OslPrintStmtType;
        this->var = var;
    }

    virtual string toString() {
#ifdef ENABLE_DLOG
        string s = "Print: " + var->toString();
#else
        string s = "";
#endif
        return s;
    }
    
    OslVar* var;
};


class OslPrintLogStmt : public OslStatement {
public:
    OslPrintLogStmt() : OslStatement() {
        this->type = OslPrintLogStmtType;
    }

    virtual string toString() {
#ifdef ENABLE_DLOG
        string s = "PrintLog";
#else
        string s = "";
#endif
        return s;
    }
};


class OslDelStmt : public OslStatement {
public:
    OslDelStmt(OslVar* var) : OslStatement() {
        this->type = OslDelStmtType;
        this->var = var;
    }

    virtual string toString() {
#ifdef ENABLE_DLOG
        string s = "Del: " + var->toString();
#else
        string s = "";
#endif
        return s;
    }
    
    OslVar* var;
};

}

#endif // OSL_H
