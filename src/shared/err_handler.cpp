// === error_handler.cpp ===
#include "error_handler.h"
#include <sstream>
#include <algorithm>

void ErrorHandler::setSourceFile(const std::string& filename, const std::string& source) {
    currentFile = filename;
    sourceLines.clear();
    
    // Split source into lines for context display
    std::istringstream stream(source);
    std::string line;
    while (std::getline(stream, line)) {
        sourceLines.push_back(line);
    }
}

void ErrorHandler::reportError(ErrorCategory category, const std::string& message,
                              int line, int column, const std::string& context) {
    SourceLocation location = makeLocation(line, column);
    messages.emplace_back(ErrorSeverity::ERROR, category, message, location, context);
    hasErrors = true;
}

void ErrorHandler::reportWarning(ErrorCategory category, const std::string& message,
                                int line, int column, const std::string& context) {
    SourceLocation location = makeLocation(line, column);
    messages.emplace_back(ErrorSeverity::WARNING, category, message, location, context);
}

void ErrorHandler::reportNote(const std::string& message, int line, int column) {
    SourceLocation location = makeLocation(line, column);
    messages.emplace_back(ErrorSeverity::NOTE, ErrorCategory::SYSTEM, message, location);
}

void ErrorHandler::reportFatal(ErrorCategory category, const std::string& message,
                              int line, int column, const std::string& context) {
    SourceLocation location = makeLocation(line, column);
    messages.emplace_back(ErrorSeverity::FATAL, category, message, location, context);
    hasErrors = true;
    hasFatalErrors = true;
}

void ErrorHandler::addSuggestion(const std::string& suggestion) {
    if (!messages.empty()) {
        messages.back().suggestions.push_back(suggestion);
    }
}

void ErrorHandler::addSuggestions(const std::vector<std::string>& suggestions) {
    if (!messages.empty()) {
        for (const auto& suggestion : suggestions) {
            messages.back().suggestions.push_back(suggestion);
        }
    }
}

int ErrorHandler::getErrorCount() const {
    return std::count_if(messages.begin(), messages.end(),
        [](const CompilerMessage& msg) {
            return msg.severity == ErrorSeverity::ERROR || msg.severity == ErrorSeverity::FATAL;
        });
}

int ErrorHandler::getWarningCount() const {
    return std::count_if(messages.begin(), messages.end(),
        [](const CompilerMessage& msg) {
            return msg.severity == ErrorSeverity::WARNING;
        });
}

void ErrorHandler::printMessages() const {
    for (const auto& msg : messages) {
        std::cerr << formatMessage(msg) << std::endl;
        
        // Print source context if available
        if (msg.location.line > 0) {
            printSourceContext(msg.location);
        }
        
        // Print suggestions
        for (const auto& suggestion : msg.suggestions) {
            std::cerr << "  suggestion: " << suggestion << std::endl;
        }
        
        std::cerr << std::endl;  // Blank line between messages
    }
}

void ErrorHandler::printSummary() const {
    int errors = getErrorCount();
    int warnings = getWarningCount();
    
    std::cerr << "Compilation ";
    if (errors > 0) {
        std::cerr << "failed with " << errors << " error(s)";
        if (warnings > 0) {
            std::cerr << " and " << warnings << " warning(s)";
        }
    } else if (warnings > 0) {
        std::cerr << "completed with " << warnings << " warning(s)";
    } else {
        std::cerr << "completed successfully";
    }
    std::cerr << std::endl;
}

void ErrorHandler::clear() {
    messages.clear();
    hasErrors = false;
    hasFatalErrors = false;
}

SourceLocation ErrorHandler::makeLocation(int line, int column) const {
    std::string sourceLine = "";
    if (line > 0 && line <= static_cast<int>(sourceLines.size())) {
        sourceLine = sourceLines[line - 1];  // Convert to 0-based indexing
    }
    return SourceLocation(currentFile, line, column, sourceLine);
}

std::string ErrorHandler::formatMessage(const CompilerMessage& msg) const {
    std::ostringstream oss;
    
    // Location (if available)
    if (!msg.location.filename.empty()) {
        oss << msg.location.toString() << ": ";
    }
    
    // Severity and category
    oss << getSeverityString(msg.severity) << ": ";
    
    if (msg.category != ErrorCategory::SYSTEM) {
        oss << "[" << getCategoryString(msg.category) << "] ";
    }
    
    // Main message
    oss << msg.message;
    
    // Context (if provided)
    if (!msg.context.empty()) {
        oss << " (while " << msg.context << ")";
    }
    
    return oss.str();
}

std::string ErrorHandler::getSeverityString(ErrorSeverity severity) const {
    switch (severity) {
        case ErrorSeverity::NOTE: return "note";
        case ErrorSeverity::WARNING: return "warning";
        case ErrorSeverity::ERROR: return "error";
        case ErrorSeverity::FATAL: return "fatal error";
    }
    return "unknown";
}

std::string ErrorHandler::getCategoryString(ErrorCategory category) const {
    switch (category) {
        case ErrorCategory::LEXICAL: return "lexical";
        case ErrorCategory::SYNTAX: return "syntax";
        case ErrorCategory::SEMANTIC: return "semantic";
        case ErrorCategory::CODEGEN: return "codegen";
        case ErrorCategory::SYSTEM: return "system";
    }
    return "unknown";
}

void ErrorHandler::printSourceContext(const SourceLocation& location) const {
    if (location.sourceLine.empty()) return;
    
    // Print the source line
    std::cerr << "    " << location.sourceLine << std::endl;
    
    // Print a caret pointing to the error location
    if (location.column > 0) {
        std::cerr << "    ";
        for (int i = 1; i < location.column; ++i) {
            std::cerr << " ";
        }
        std::cerr << "^" << std::endl;
    }
}