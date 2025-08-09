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
