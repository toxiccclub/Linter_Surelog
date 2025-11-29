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

namespace Analyzer {

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

void checkParameterDynamicArray(const FileContent* fC) {

        NodeId root = fC->getRootNode();

        // типы параметрических деклараций
        std::vector<VObjectType> declTypes = {
            VObjectType::paParameter_declaration,
            VObjectType::paLocal_parameter_declaration
        };

        for (auto declType : declTypes) {
            auto paramDecls = fC->sl_collect_all(root, declType);

            for (NodeId decl : paramDecls) {

                std::string paramName = findParamName(fC, decl);

                auto unsizedDims =
                    fC->sl_collect_all(decl, VObjectType::paUnsized_dimension);

                if (!unsizedDims.empty()) {

                    // Первый узел ошибки
                    NodeId errNode = unsizedDims[0];

                    std::string fileName =
                        std::string(FileSystem::getInstance()->toPath(
                            fC->getFileId(errNode)));

                    uint32_t line = fC->Line(errNode);

                    std::cerr
                        << "Error PARAMETER_DYNAMIC_ARRAY: parameter '"
                        << paramName
                        << "' uses unsized (dynamic) unpacked array dimension at "
                        << fileName << ":" << line << std::endl;
                }
            }
        }
    }
}
