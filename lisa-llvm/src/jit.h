/**
 * @file jit.h
 * @version 0.1.2
 * @date 2023-05-27
 * 
 * @copyright Copyright Miracle Factory (c) 2023
 * 
 */

#ifndef JIT_H
#define JIT_H

#pragma once

#include <memory>
#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/ExecutorProcessControl.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"


namespace lisa
{
class LisaJIT
{
private:
    std::unique_ptr<llvm::orc::ExecutionSession> es;
    llvm::DataLayout dl;
    llvm::orc::MangleAndInterner mangle;
    llvm::orc::RTDyldObjectLinkingLayer objectLayer;
    llvm::orc::IRCompileLayer compileLayer;
    llvm::orc::JITDylib &mainJD;
public:
    LisaJIT(std::unique_ptr<llvm::orc::ExecutionSession> es, 
            llvm::orc::JITTargetMachineBuilder jtmb,
            llvm::DataLayout dl):
        es(std::move(es)), dl(std::move(dl)), mangle(*this->es, this->dl),
        objectLayer(*this->es, [](){return std::make_unique<llvm::SectionMemoryManager>();}),
        compileLayer(*this->es, objectLayer, std::make_unique<llvm::orc::ConcurrentIRCompiler>(std::move(jtmb))),
        mainJD(this->es->createBareJITDylib("<main>")) {
            mainJD.addGenerator(
                cantFail(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(dl.getGlobalPrefix())));
            if (jtmb.getTargetTriple().isOSBinFormatCOFF()) {
                objectLayer.setOverrideObjectFlagsWithResponsibilityFlags(true);
                objectLayer.setAutoClaimResponsibilityForObjectSymbols(true);
            }
        }

    
    ~LisaJIT() {
        if (auto err = es->endSession())
            es->reportError(std::move(err));
    }


    static llvm::Expected<std::unique_ptr<LisaJIT>> Create() {
        auto epc = llvm::orc::SelfExecutorProcessControl::Create();
        if (!epc)
            return epc.takeError();
        auto es = std::make_unique<llvm::orc::ExecutionSession>(std::move(*epc));
        llvm::orc::JITTargetMachineBuilder jtmb(es->getExecutorProcessControl().getTargetTriple());
        auto dl = jtmb.getDefaultDataLayoutForTarget();
        if (!dl)
            return dl.takeError();
        return std::make_unique<LisaJIT>(std::move(es), std::move(jtmb), std::move(*dl));
    }


    const llvm::DataLayout &getDataLayout() const { return dl; }


    llvm::orc::JITDylib &getMainJITDylib() { return mainJD; }


    llvm::Error addModule(llvm::orc::ThreadSafeModule tsm, llvm::orc::ResourceTrackerSP rt = nullptr) {
        if (!rt)
            rt = mainJD.getDefaultResourceTracker();
        return compileLayer.add(rt, std::move(tsm));
    }


    llvm::Expected<llvm::JITEvaluatedSymbol> lookup(llvm::StringRef name) {
        return es->lookup({&mainJD}, mangle(name.str()));
    }
};
}


#endif
