#include <cstdint>
#include <iostream>
#include <string>

#include "Surelog/API/Surelog.h"
#include "Surelog/CommandLine/CommandLineParser.h"
#include "Surelog/Common/FileSystem.h"
#include "Surelog/Design/Design.h"
#include "Surelog/Design/FileContent.h"
#include "Surelog/ErrorReporting/ErrorContainer.h"
#include "Surelog/SourceCompile/SymbolTable.h"
#include "Surelog/SourceCompile/VObjectTypes.h"

using namespace SURELOG;

namespace Analyzer {

// Вспомогательная функция: получить все StringConst внутри заданного узла
static std::vector<NodeId> collectStringConsts(const FileContent* fC,
                                               NodeId node) {
  std::vector<NodeId> out;
  // sl_collect_all возвращает все дочерние узлы запрошенного типа
  auto stringNodes = fC->sl_collect_all(node, VObjectType::slStringConst);
  for (NodeId s : stringNodes) out.push_back(s);
  return out;
}

// Вспомогательная: получить "склеенное" имя из списка StringConst (например
// "top.id2.id3")
static std::string joinNames(const FileContent* fC,
                             const std::vector<NodeId>& parts) {
  if (parts.empty()) return "<unknown>";
  std::string res;
  for (size_t i = 0; i < parts.size(); ++i) {
    if (i) res += ".";
    res += std::string(fC->SymName(parts[i]));
  }
  return res;
}

void checkHierarchicalInterfaceIdentifier(const FileContent* fC,
                                          ErrorContainer* errors,
                                          SymbolTable* symbols) {
  NodeId root = fC->getRootNode();

  // Ищем interface_identifier
  auto iidNodes = fC->sl_collect_all(root, VObjectType::paInterface_identifier);

  for (NodeId iid : iidNodes) {
    auto parts = collectStringConsts(fC, iid);

    // Если частей >= 1 — это иерархическое имя → нарушение
    if (parts.size() >= 1) {
      std::string fullName = joinNames(fC, parts);

      auto fileId = fC->getFileId(iid);
      uint32_t line = fC->Line(iid);
      uint32_t column = 0;
      try {
        column = fC->Column(iid);
      } catch (...) {
        column = 0;
      }

      SymbolId obj = symbols->registerSymbol(fullName);

      Location loc(fileId, line, column, obj);

      Error err(ErrorDefinition::LINT_HIERARCHICAL_INTERFACE_IDENTIFIER, loc);

      errors->addError(err, false);
    }
  }
}
}  // namespace Analyzer