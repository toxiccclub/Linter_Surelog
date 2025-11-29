#include <iostream>
#include "Surelog/API/Surelog.h"
#include "Surelog/CommandLine/CommandLineParser.h"
#include "rule_distpatcher.h"

using namespace SURELOG;

int main(int argc, const char** argv) {

    auto* symbolTable = new SymbolTable();
    auto* errors = new ErrorContainer(symbolTable);
    auto* clp = new CommandLineParser(errors, symbolTable, false, false);

    clp->noPython();
    clp->setParse(true);
    clp->setCompile(true);
    clp->setElaborate(true);
    clp->setwritePpOutput(true);
    clp->setCacheAllowed(false);

    bool success = clp->parseCommandLine(argc, argv);
    Design* the_design = nullptr;
    scompiler* compiler = nullptr;
    vpiHandle UHDMdesign = nullptr;

    if (success && !clp->help()) {
        compiler = start_compiler(clp);
        the_design = get_design(compiler);
        UHDMdesign = get_uhdm_design(compiler);
    }

    if (!the_design && !UHDMdesign) {
        std::cerr << "No design created" << std::endl;
        return 1;
    }

    runAllRulesOnDesign(the_design, UHDMdesign);

    if (success && !clp->help()) {
        shutdown_compiler(compiler);
    }

    delete clp;
    delete symbolTable;

    return 0;
}
