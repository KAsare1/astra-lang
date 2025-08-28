#ifndef TARGET_CODEGEN_H
#define TARGET_CODEGEN_H

#include "llvm/IR/Module.h"
#include "llvm/Target/TargetMachine.h"
#include <string>

class TargetCodegen {
public:
    TargetCodegen(llvm::Module &m);
    void emitAssembly(const std::string &filename);
    void emitObject(const std::string &filename);

private:
    llvm::Module &module;
    llvm::TargetMachine *targetMachine;
};

#endif
