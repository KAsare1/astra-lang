// === error_handler.h ===
#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <memory>

// Error severity levels
enum class ErrorSeverity {
    NOTE,       // Informational
    WARNING,    // Potential issues
    ERROR,      // Compilation errors
    FATAL       // Cannot continue
};

// Error categories for different compiler phases
enum class ErrorCategory {
    LEXICAL,    // Tokenization errors
    SYNTAX,     // Parsing errors  
    SEMANTIC,   // Type/scope errors
    CODEGEN,    // IR/target generation errors
    SYSTEM      // File I/O, memory, etc.
};

// Source location information
struct SourceLocation {
    std::string filename;
    int line;
    int column;
    std::string sourceLine;  // The actual line of source code
    
    SourceLocation(const std::string& file = "", int ln = 0, int col = 0, const std::string& src = "")
        : filename(file), line(ln), column(col), sourceLine(src) {}
    
    std::string toString() const {
        if (filename.empty()) return "";
        return filename + ":" + std::to_string(line) + ":" + std::to_string(column);
    }
};

// Individual error/warning/note
struct CompilerMessage {
    ErrorSeverity severity;
    ErrorCategory category;
    std::string message;
    SourceLocation location;
    std::vector<std::string> suggestions;  // Potential fixes
    std::string context;  // What the compiler was trying to do
    
    CompilerMessage(ErrorSeverity sev, ErrorCategory cat, const std::string& msg, 
                   const SourceLocation& loc = SourceLocation(), const std::string& ctx = "")
        : severity(sev), category(cat), message(msg), location(loc), context(ctx) {}
};

// Main error handler class
class ErrorHandler {
private:
    std::vector<CompilerMessage> messages;
    std::string currentFile;
    std::vector<std::string> sourceLines;  // Cache of source file
    bool hasErrors = false;
    bool hasFatalErrors = false;
    
public:
    // Initialize with source file
    void setSourceFile(const std::string& filename, const std::string& source);
    
    // Report different types of messages
    void reportError(ErrorCategory category, const std::string& message, 
                    int line = 0, int column = 0, const std::string& context = "");
    
    void reportWarning(ErrorCategory category, const std::string& message,
                      int line = 0, int column = 0, const std::string& context = "");
    
    void reportNote(const std::string& message, int line = 0, int column = 0);
    
    void reportFatal(ErrorCategory category, const std::string& message,
                    int line = 0, int column = 0, const std::string& context = "");
    
    // Add suggestions to last message
    void addSuggestion(const std::string& suggestion);
    void addSuggestions(const std::vector<std::string>& suggestions);
    
    // Query error state
    bool hasErrorsOccurred() const { return hasErrors; }
    bool hasFatalErrorsOccurred() const { return hasFatalErrors; }
    int getErrorCount() const;
    int getWarningCount() const;
    
    // Display all messages
    void printMessages() const;
    void printSummary() const;
    
    // Clear all messages (for multi-file compilation)
    void clear();
    
private:
    SourceLocation makeLocation(int line, int column) const;
    std::string formatMessage(const CompilerMessage& msg) const;
    std::string getSeverityString(ErrorSeverity severity) const;
    std::string getCategoryString(ErrorCategory category) const;
    void printSourceContext(const SourceLocation& location) const;
};

