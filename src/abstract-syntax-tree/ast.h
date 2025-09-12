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

struct ReturnExpr : Expr {
    std::unique_ptr<Expr> value;
    ReturnExpr(std::unique_ptr<Expr> val = nullptr) : value(std::move(val)) {}
};

// NEW: Array literal expression [1, 2, 3]
struct ArrayLiteralExpr : Expr {
    std::vector<std::unique_ptr<Expr>> elements;
    std::string elementType;  // Inferred during semantic analysis
    
    ArrayLiteralExpr() = default;
    ArrayLiteralExpr(std::vector<std::unique_ptr<Expr>> elems) 
        : elements(std::move(elems)) {}
};

// NEW: Array/string indexing expression arr[index]
struct IndexExpr : Expr {
    std::unique_ptr<Expr> object;   // Array or string being indexed
    std::unique_ptr<Expr> index;    // Index expression
    
    IndexExpr(std::unique_ptr<Expr> obj, std::unique_ptr<Expr> idx)
        : object(std::move(obj)), index(std::move(idx)) {}
};

// NEW: String concatenation expression (could use BinaryExpr but this is clearer)
struct ConcatExpr : Expr {
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    
    ConcatExpr(std::unique_ptr<Expr> l, std::unique_ptr<Expr> r)
        : left(std::move(l)), right(std::move(r)) {}
};

struct Stmt {
    virtual ~Stmt() = default;
};

struct VarDeclStmt : Stmt {
    std::string name;
    std::string type;  // Enhanced: "int", "[int]", "string", etc.
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

// NEW: Index assignment statement arr[i] = value
struct IndexAssignmentStmt : Stmt {
    std::unique_ptr<Expr> object;   // Array being assigned to
    std::unique_ptr<Expr> index;    // Index expression
    std::unique_ptr<Expr> value;    // Value to assign
    
    IndexAssignmentStmt(std::unique_ptr<Expr> obj, std::unique_ptr<Expr> idx, std::unique_ptr<Expr> val)
        : object(std::move(obj)), index(std::move(idx)), value(std::move(val)) {}
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

struct ReturnStmt : Stmt {
    std::unique_ptr<Expr> value;
    ReturnStmt(std::unique_ptr<Expr> val = nullptr) : value(std::move(val)) {}
};

struct Parameter {
    std::string name;
    std::string type;  // Enhanced: supports "[int]", "[string]", etc.
    
    Parameter(const std::string& n, const std::string& t) : name(n), type(t) {}
};

struct FunctionDeclStmt : Stmt {
    std::string name;
    std::vector<Parameter> parameters;
    std::string returnType;  // Enhanced: supports "[int]", etc.
    std::unique_ptr<BlockStmt> body;
    
    FunctionDeclStmt(const std::string& n, std::vector<Parameter> params, 
                     const std::string& retType, std::unique_ptr<BlockStmt> b)
        : name(n), parameters(std::move(params)), returnType(retType), body(std::move(b)) {}
};