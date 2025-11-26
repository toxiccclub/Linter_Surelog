#include <cstdint>
#include <iostream>
#include <string>
#include "Surelog/API/Surelog.h"
#include "Surelog/CommandLine/CommandLineParser.h"
#include "Surelog/Common/FileSystem.h"
#include "Surelog/Design/Design.h"
#include "Surelog/Design/FileContent.h"
#include "Surelog/SourceCompile/VObjectTypes.h"
#include "Surelog/ErrorReporting/ErrorContainer.h"
#include "Surelog/SourceCompile/SymbolTable.h"

using namespace SURELOG;

std::string findParamName(const FileContent* fC, NodeId paramDeclId) {
    // List_of_param_assignments
    auto listNodes = fC->sl_collect_all(paramDeclId, VObjectType::paList_of_param_assignments);
    for (NodeId listNode : listNodes) {
        // Param_assignment
        auto assignNodes = fC->sl_collect_all(listNode, VObjectType::paParam_assignment);
        for (NodeId assignNode : assignNodes) {
            // Имя параметра — StringConst
            NodeId nameNode = fC->Child(assignNode);
            if (nameNode && fC->Type(nameNode) == VObjectType::slStringConst) {
                return std::string(fC->SymName(nameNode));
            }
        }
    }
    return "<unknown>";
}

int main(int argc, const char** argv) {
    uint32_t code = 0;

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

    if (success && !clp->help()) {
        compiler = start_compiler(clp);
        the_design = get_design(compiler);
    }

    if (!the_design) {
        std::cerr << "No design created" << std::endl;
        return 1;
    }

    // ---------------------
    // Обход всех файлов AST
    // ---------------------
    for (auto& it : the_design->getAllFileContents()) {
        const FileContent* fC = it.second;
        if (!fC) continue;

        NodeId root = fC->getRootNode();

        // Ищем Parameter_declaration и Local_parameter_declaration
        std::vector<VObjectType> declTypes = {
            VObjectType::paParameter_declaration,
            VObjectType::paLocal_parameter_declaration
        };

        for (auto declType : declTypes) {
            auto paramDeclNodes = fC->sl_collect_all(root, declType);

            for (NodeId paramDeclId : paramDeclNodes) {

                // Имя параметра
                std::string paramName = findParamName(fC, paramDeclId);

                // Проверяем есть ли Unsized_dimension внутри данного параметра
                auto unsizedDims = fC->sl_collect_all(paramDeclId, VObjectType::paUnsized_dimension);
                if (!unsizedDims.empty()) {

                    // Берём первый узел ошибки
                    NodeId errNode = unsizedDims[0];
                    auto fileId = fC->getFileId(errNode);
                    std::string fileName = std::string(FileSystem::getInstance()->toPath(fileId));
                    uint32_t line = fC->Line(errNode);

                    // Выводим сообщение о нарушении правила
                    std::cerr << "Error PARAMETER_DYNAMIC_ARRAY: parameter '"
                              << paramName
                              << "' uses unsized (dynamic) unpacked array dimension at "
                              << fileName << ":" << line << std::endl;
                }
            }
        }
    }

    if (success && !clp->help()) {
        shutdown_compiler(compiler);
    }

    delete clp;
    delete symbolTable;

    return code;
}
