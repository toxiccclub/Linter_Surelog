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

// Вспомогательная функция: получить все StringConst внутри заданного узла
static std::vector<NodeId> collectStringConsts(const FileContent* fC, NodeId node) {
    std::vector<NodeId> out;
    // sl_collect_all возвращает все дочерние узлы запрошенного типа
    auto stringNodes = fC->sl_collect_all(node, VObjectType::slStringConst);
    for (NodeId s : stringNodes) out.push_back(s);
    return out;
}

// Вспомогательная: получить "склеенное" имя из списка StringConst (например "top.id2.id3")
static std::string joinNames(const FileContent* fC, const std::vector<NodeId>& parts) {
    if (parts.empty()) return "<unknown>";
    std::string res;
    for (size_t i=0;i<parts.size();++i) {
        if (i) res += ".";
        res += std::string(fC->SymName(parts[i]));
    }
    return res;
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

    // ---- Проход по всем файлам и поиск проблемных Interface_identifier ----
    for (auto& it : the_design->getAllFileContents()) {
        const FileContent* fC = it.second;
        if (!fC) continue;

        NodeId root = fC->getRootNode();

        // Собираем все узлы, которые представляют Interface_identifier
        auto interfaceIdNodes = fC->sl_collect_all(root, VObjectType::paInterface_identifier);

        for (NodeId iid : interfaceIdNodes) {
            // Собираем все StringConst внутри этого Interface_identifier.
            // Если их > 1 => это что-то вида a.b.c (иерархия).
            auto parts = collectStringConsts(fC, iid);

            if (parts.size() > 1) {
                // Найдём файл/строку где этот узел расположен
                auto fileId = fC->getFileId(iid);
                std::string fileName = std::string(FileSystem::getInstance()->toPath(fileId));
                uint32_t line = fC->Line(iid);

                // Собираем полное имя для вывода
                std::string fullName = joinNames(fC, parts);

                // Выводим ошибку
                std::cerr << "Error HIERARCHICAL_INTERFACE_IDENTIFIER: hierarchical interface identifier '"
                          << fullName
                          << "' not allowed at "
                          << fileName << ":" << line << std::endl;
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
