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

// ------------------------------------------------------------
// Извлекаем имя переменной из Variable_decl_assignment
// ------------------------------------------------------------
std::string findVarName(const FileContent* fC, NodeId dataDecl) {
    auto lists = fC->sl_collect_all(dataDecl, VObjectType::paList_of_variable_decl_assignments);
    for (NodeId list : lists) {
        auto assigns = fC->sl_collect_all(list, VObjectType::paVariable_decl_assignment);
        for (NodeId assign : assigns) {
            NodeId child = fC->Child(assign);
            if (child && fC->Type(child) == VObjectType::slStringConst)
                return std::string(fC->SymName(child));
        }
    }
    return "<unknown>";
}

// ------------------------------------------------------------
// Проверка наличия явного типа
// ------------------------------------------------------------
bool hasExplicitType(const FileContent* fC, NodeId dataDecl) {
    static const VObjectType typeNodes[] = {
        VObjectType::paNet_type,
        VObjectType::paData_type,
        VObjectType::paInteger_atom_type,
        VObjectType::paInteger_vector_type,
        VObjectType::paNon_integer_type,
        VObjectType::paString_type,
        VObjectType::paClass_type,
        VObjectType::paIntVec_TypeBit
    };

    for (auto t : typeNodes) {
        if (!fC->sl_collect_all(dataDecl, t).empty())
            return true;
    }
    return false;
}

// ------------------------------------------------------------
// MAIN
// ------------------------------------------------------------
int main(int argc, const char** argv) {
    uint32_t code = 0;

    auto* symbolTable = new SymbolTable();
    auto* errors = new ErrorContainer(symbolTable);
    auto* clp = new CommandLineParser(errors, symbolTable, false, false);

    clp->noPython();
    clp->setParse(true);
    clp->setCompile(true);
    clp->setElaborate(true);
    clp->setCacheAllowed(false);
    clp->setwritePpOutput(true);

    bool success = clp->parseCommandLine(argc, argv);
    Design* design = nullptr;
    scompiler* compiler = nullptr;

    if (success && !clp->help()) {
        compiler = start_compiler(clp);
        design = get_design(compiler);
    }

    if (!design) {
        std::cerr << "No design created\n";
        return 1;
    }

    // ------ Посещаем все файлы ------
    for (auto& it : design->getAllFileContents()) {
        const FileContent* fC = it.second;
        if (!fC) continue;

        NodeId root = fC->getRootNode();

        // Ищем Data_declaration
        auto dataDecls = fC->sl_collect_all(root, VObjectType::paData_declaration);

        for (NodeId dataDecl : dataDecls) {

            // Проверяем есть ли Packed_dimension перед типом
            auto packedDims = fC->sl_collect_all(dataDecl, VObjectType::paPacked_dimension);
            if (packedDims.empty()) continue;

            // Проверяем, что нет типа
            if (hasExplicitType(fC, dataDecl))
                continue; // тип есть → нарушения нет

            // Теперь IMPLICIT_DATA_TYPE_IN_DECLARATION
            std::string varName = findVarName(fC, dataDecl);

            NodeId where = packedDims.front();  
            auto fileId = fC->getFileId(where);
            uint32_t line = fC->Line(where);
            std::string filename = std::string(FileSystem::getInstance()->toPath(fileId));

            std::cerr << "Error IMPLICIT_DATA_TYPE_IN_DECLARATION: variable '"
                      << varName
                      << "' declared without explicit type at "
                      << filename << ":" << line << std::endl;
        }
    }

    if (success && !clp->help()) {
        shutdown_compiler(compiler);
    }

    delete clp;
    delete symbolTable;

    return code;
}
