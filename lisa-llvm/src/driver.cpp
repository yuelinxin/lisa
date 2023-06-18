/**
 * @file driver.cpp
 * @version 0.1.1
 * @date 2023-06-18
 * 
 * @copyright Copyright Miracle Factory (c) 2023
 * 
 */

#include <getopt.h>

#include "lexer.h"
#include "parser.h"
#include "ast.h"


bool debug = false; // tidy this up later !!!


// handle definition
static void handleDefinition(Lexer *lex, CodeGenVisitor *codegen) {
    if (auto fnAST = Definition(lex)) {
        if (auto *fnIR = fnAST->accept(*codegen)) {
            if (debug) {
                fprintf(stderr, "\033[1;34m->\033[0m Read function definition:\n");
                fnIR->print(errs());
            }
        }
    } else {
        lex->getTok();
    }
}


// handle extern
static void handleExtern(Lexer *lex, CodeGenVisitor *codegen) {
    if (auto protoAST = Extern(lex)) {
        if (auto *fnIR = protoAST->accept(*codegen)) {
            if (debug) {
                fprintf(stderr, "\033[1;34m->\033[0m Read extern:\n");
                fnIR->print(errs());
            }
        }
    } else {
        lex->getTok();
    }
}


// handle top-level expression
static void handleTopLevelExpr(Lexer *lex, CodeGenVisitor *codegen) {
    if (auto fnAST = TopLevelExpr(lex)) {
        if (auto *fnIR = fnAST->accept(*codegen)) {
            if (debug) {
                fprintf(stderr, "\033[1;34m->\033[0m Read top-level expression:\n");
                fnIR->print(errs());
            }
        }
    } else {
        lex->getTok();
    }
}


// main loop
static void mainLoop(Lexer *lex, CodeGenVisitor *codegen) {
    while (true) {
        Token t = lex->peekTok();
        switch (t.tp) {
            case TOK_EOF:
                return;
            case TOK_FN:
                handleDefinition(lex, codegen);
                break;
            case TOK_EXTERN:
                handleExtern(lex, codegen);
                break;
            default:
                handleTopLevelExpr(lex, codegen);
                break;
        }
    }
}


static void initEnv() {
    // Initialize the target registry etc.
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();
}


// parse options
static void parseOpt(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "dhv")) != -1) {
        switch (opt) {
            case 'd':
                debug = true;
                break;
            case 'h':
                std::cout << "Usage: " << argv[0] 
                    << " [options] <input_file>" << std::endl;
                std::cout << "Available options:" << std::endl;
                std::cout << "-h:  Display this information" << std::endl;
                std::cout << "-v:  Display version information" << std::endl;
                exit(0);
            case 'v':
                std::cout << "Lisa Compiler v0.1.1" << std::endl;
                std::cout << "Copyright (c) 2023 Miracle Factory" << std::endl;
                exit(0);
            default:
                std::cerr << "Invalid option: " << opt << "\n";
                exit(1);
        }
    }
}


// parse input file name
static std::string parseFilenames(int argc, char **argv) {
    std::string inputFilename;
    std::string outputFilename;
    if (argc < 2) {
        std::cerr << "Missing input file\n";
        exit(1);
    }
    inputFilename = argv[argc - 1];
    size_t lastDot = inputFilename.find_last_of(".");
    if (lastDot == std::string::npos)
        outputFilename = inputFilename + ".o";
    else
        outputFilename = inputFilename.substr(0, lastDot) + ".o";
    return outputFilename;
}


// main driver code
int main(int argc, char **argv) {
    // Initialize the environment
    initEnv();

    // Parse command line arguments
    parseOpt(argc, argv);
    std::string outputFile;
    outputFile = parseFilenames(argc, argv);

    // Create the lexer and codegen visitor
    auto lex = std::make_unique<Lexer>(argv[argc - 1]);
    auto codegen = std::make_unique<CodeGenVisitor>();

    // start the main loop
    mainLoop(lex.get(), codegen.get());

    // configure target
    std::string error;
    auto targetTriple = llvm::sys::getDefaultTargetTriple();
    auto target = TargetRegistry::lookupTarget(targetTriple, error);
    if (!target) {
        errs() << error;
        return 1;
    }
    auto cpu = "generic";
    auto features = "";
    TargetOptions opt;
    auto rm = Optional<Reloc::Model>();
    auto targetMachine = target->createTargetMachine(
        targetTriple, cpu, features, opt, rm);
    auto module = codegen->borrowModule();
    module->setDataLayout(targetMachine->createDataLayout());
    module->setTargetTriple(targetTriple);

    // write to file
    std::error_code ec;
    raw_fd_ostream dest(outputFile, ec, sys::fs::OF_None);
    if (ec) {
        errs() << "Could not open file: " << ec.message();
        return 1;
    }
    legacy::PassManager pass;
    auto fileType = CGFT_ObjectFile;
    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType)) {
        errs() << "TargetMachine can't emit a file of this type";
        return 1;
    }
    pass.run(*module);
    dest.flush();

    return 0;
}
