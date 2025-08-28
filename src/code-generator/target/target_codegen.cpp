#include "../target/target_codegen.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include <iostream>

using namespace llvm;

TargetCodegen::TargetCodegen(Module &m) : module(m) {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    std::string error;
    auto targetTriple = sys::getDefaultTargetTriple();
    module.setTargetTriple(targetTriple);

    const Target *target = TargetRegistry::lookupTarget(targetTriple, error);
    if (!target) {
        std::cerr << "Target lookup failed: " << error << "\n";
        exit(1);
    }

    TargetOptions opt;
    targetMachine = target->createTargetMachine(targetTriple, "generic", "", opt, Reloc::PIC_);
    module.setDataLayout(targetMachine->createDataLayout());
}

void TargetCodegen::emitAssembly(const std::string &filename) {
    std::error_code ec;
    raw_fd_ostream dest(filename, ec, sys::fs::OF_Text);
    if (ec) {
        std::cerr << "Error opening file " << filename << ": " << ec.message() << "\n";
        return;
    }

    legacy::PassManager pass;
    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, llvm::CodeGenFileType::AssemblyFile)) {
        std::cerr << "TargetMachine can't emit assembly.\n";
        return;
    }
    pass.run(module);
}

void TargetCodegen::emitObject(const std::string &filename) {
    std::error_code ec;
    raw_fd_ostream dest(filename, ec, sys::fs::OF_None);
    if (ec) {
        std::cerr << "Error opening file " << filename << ": " << ec.message() << "\n";
        return;
    }

    legacy::PassManager pass;
    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, llvm::CodeGenFileType::ObjectFile)) {
        std::cerr << "TargetMachine can't emit object file.\n";
        return;
    }
    pass.run(module);
}
