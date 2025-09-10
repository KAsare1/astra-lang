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

// NEW: Range expression for for loops (start:end or start:end:step)
struct RangeExpr : Expr {
    std::unique_ptr<Expr> start;
    std::unique_ptr<Expr> end;
    std::unique_ptr<Expr> step;  // Optional, defaults to 1
    
    RangeExpr(std::unique_ptr<Expr> s, std::unique_ptr<Expr> e, std::unique_ptr<Expr> st = nullptr)
        : start(std::move(s)), end(std::move(e)), step(std::move(st)) {}
};

struct Stmt {
    virtual ~Stmt() = default;
};

struct VarDeclStmt : Stmt {
    std::string name;
    std::unique_ptr<Expr> initializer;
    VarDeclStmt(const std::string& n, std::unique_ptr<Expr> init)
        : name(n), initializer(std::move(init)) {}
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

// NEW: For loop statements
struct ForStmt : Stmt {
    std::string variable;              // Loop variable name
    std::unique_ptr<Expr> range;       // Range expression (could be RangeExpr)
    std::unique_ptr<Stmt> body;        // Loop body
    
    ForStmt(const std::string& var, std::unique_ptr<Expr> r, std::unique_ptr<Stmt> b)
        : variable(var), range(std::move(r)), body(std::move(b)) {}
};

// NEW: Traditional C-style for loop
struct CForStmt : Stmt {
    std::unique_ptr<Stmt> initialization;  // let i = 0
    std::unique_ptr<Expr> condition;       // i <= 10
    std::unique_ptr<Stmt> increment;       // i = i + 1
    std::unique_ptr<Stmt> body;            // loop body
    
    CForStmt(std::unique_ptr<Stmt> init, std::unique_ptr<Expr> cond, 
             std::unique_ptr<Stmt> inc, std::unique_ptr<Stmt> b)
        : initialization(std::move(init)), condition(std::move(cond)),
          increment(std::move(inc)), body(std::move(b)) {}
};