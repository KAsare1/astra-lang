#pragma once
#include <string>
#include <memory>
#include <vector>

struct Expr {
    virtual ~Expr() = default;
};

struct LiteralExpr : Expr {
    std::string value;
    LiteralExpr(const std::string& v) : value(v) {}
};

struct VariableExpr : Expr {
    std::string name;
    VariableExpr(const std::string& n) : name(n) {}
};

struct UnaryExpr : Expr {
    std::string op;
    std::unique_ptr<Expr> operand;
    UnaryExpr(const std::string& o, std::unique_ptr<Expr> operand) 
        : op(o), operand(std::move(operand)) {}
};

struct BinaryExpr : Expr {
    std::unique_ptr<Expr> left;
    std::string op;
    std::unique_ptr<Expr> right;
    BinaryExpr(std::unique_ptr<Expr> l, const std::string& o, std::unique_ptr<Expr> r)
        : left(std::move(l)), op(o), right(std::move(r)) {}
};

struct CallExpr : Expr {
    std::string callee;
    std::vector<std::unique_ptr<Expr>> arguments;
    CallExpr(const std::string& c) : callee(c) {}
};

struct RangeExpr : Expr {
    std::unique_ptr<Expr> start;
    std::unique_ptr<Expr> end;
    std::unique_ptr<Expr> step;
    
    RangeExpr(std::unique_ptr<Expr> s, std::unique_ptr<Expr> e, std::unique_ptr<Expr> st = nullptr)
        : start(std::move(s)), end(std::move(e)), step(std::move(st)) {}
};

// NEW: Return expression for function returns
struct ReturnExpr : Expr {
    std::unique_ptr<Expr> value;  // Optional return value
    ReturnExpr(std::unique_ptr<Expr> val = nullptr) : value(std::move(val)) {}
};

struct Stmt {
    virtual ~Stmt() = default;
};

struct VarDeclStmt : Stmt {
    std::string name;
    std::string type;  // NEW: Explicit type annotation
    std::unique_ptr<Expr> initializer;
    
    VarDeclStmt(const std::string& n, const std::string& t = "", std::unique_ptr<Expr> init = nullptr)
        : name(n), type(t), initializer(std::move(init)) {}
};

struct ExprStmt : Stmt {
    std::unique_ptr<Expr> expression;
    ExprStmt(std::unique_ptr<Expr> expr) : expression(std::move(expr)) {}
};

struct AssignmentStmt : Stmt {
    std::string name;
    std::unique_ptr<Expr> value;
    
    AssignmentStmt(const std::string& n, std::unique_ptr<Expr> val)
        : name(n), value(std::move(val)) {}
};

struct BlockStmt : Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;
    BlockStmt() = default;
    
    BlockStmt(std::vector<std::unique_ptr<Stmt>> stmts) 
        : statements(std::move(stmts)) {}
};

struct IfStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;
    
    IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> then, 
           std::unique_ptr<Stmt> else_branch = nullptr)
        : condition(std::move(cond)), thenBranch(std::move(then)), 
          elseBranch(std::move(else_branch)) {}
};

struct WhileStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
    
    WhileStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> body)
        : condition(std::move(cond)), body(std::move(body)) {}
};

struct ForStmt : Stmt {
    std::string variable;
    std::unique_ptr<Expr> range;
    std::unique_ptr<Stmt> body;
    
    ForStmt(const std::string& var, std::unique_ptr<Expr> r, std::unique_ptr<Stmt> b)
        : variable(var), range(std::move(r)), body(std::move(b)) {}
};

// NEW: Return statement
struct ReturnStmt : Stmt {
    std::unique_ptr<Expr> value;  // Optional return value
    ReturnStmt(std::unique_ptr<Expr> val = nullptr) : value(std::move(val)) {}
};

// NEW: Function parameter structure
struct Parameter {
    std::string name;
    std::string type;
    
    Parameter(const std::string& n, const std::string& t) : name(n), type(t) {}
};

// NEW: Function declaration statement
struct FunctionDeclStmt : Stmt {
    std::string name;
    std::vector<Parameter> parameters;
    std::string returnType;
    std::unique_ptr<BlockStmt> body;
    
    FunctionDeclStmt(const std::string& n, std::vector<Parameter> params, 
                     const std::string& retType, std::unique_ptr<BlockStmt> b)
        : name(n), parameters(std::move(params)), returnType(retType), body(std::move(b)) {}
};