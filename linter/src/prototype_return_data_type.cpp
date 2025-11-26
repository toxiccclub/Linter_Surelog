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

// --- Получение имени функции из Function_data_type_or_implicit ---
std::string getFunctionName(const FileContent* fC, NodeId typeNode) {
    for (NodeId node = typeNode; node; node = fC->Sibling(node)) {
        if (fC->Type(node) == VObjectType::slStringConst) {
            return std::string(fC->SymName(node));
        }
    }
    return "<unknown>";
}

// --- Проверка наличия return type ---
bool hasReturnType(const FileContent* fC, NodeId typeNode) {
    for (NodeId child = fC->Child(typeNode); child; child = fC->Sibling(child)) {
        if (fC->Type(child) == VObjectType::paFunction_data_type) {
            return true;
        }
    }
    return false;
}

// --- Проверка одного Function_prototype ---
void checkFunctionPrototype(const FileContent* fC, NodeId protoId) {
    auto ftypeNodes = fC->sl_collect_all(protoId, VObjectType::paFunction_data_type_or_implicit, false);
    if (ftypeNodes.empty()) return;

    NodeId typeNode = ftypeNodes.front();

    if (!hasReturnType(fC, typeNode)) {
        std::string funcName = getFunctionName(fC, typeNode);
        uint32_t line = fC->Line(typeNode);
        std::string fileName = std::string(FileSystem::getInstance()->toPath(fC->getFileId(typeNode)));

        std::cerr << "Error PROTOTYPE_RETURN_DATA_TYPE: Function prototype '"
                  << funcName << "' missing return data type at "
                  << fileName << ":" << line << std::endl;
    }
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

    for (auto& it : the_design->getAllFileContents()) {
        const FileContent* fC = it.second;
        if (!fC) continue;

        NodeId root = fC->getRootNode();

        // --- 1. Классы ---
        auto classNodes = fC->sl_collect_all(root, VObjectType::paClass_declaration);
        for (NodeId classId : classNodes) {
            auto methods = fC->sl_collect_all(classId, VObjectType::paClass_method);
            for (NodeId m : methods) {
                auto protoNodes = fC->sl_collect_all(m, VObjectType::paFunction_prototype, false);
                for (NodeId protoId : protoNodes) {
                    checkFunctionPrototype(fC, protoId);
                }
            }
        }

        // --- 2. Интерфейсы ---
        auto interfaceNodes = fC->sl_collect_all(root, VObjectType::paInterface_declaration);
        for (NodeId ifaceId : interfaceNodes) {
            auto nonPortItems = fC->sl_collect_all(ifaceId, VObjectType::paNon_port_interface_item);
            for (NodeId item : nonPortItems) {
                auto externNodes = fC->sl_collect_all(item, VObjectType::paExtern_tf_declaration);
                for (NodeId extId : externNodes) {
                    auto protoNodes = fC->sl_collect_all(extId, VObjectType::paFunction_prototype, false);
                    for (NodeId protoId : protoNodes) {
                        checkFunctionPrototype(fC, protoId);
                    }
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
