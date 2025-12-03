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

// Найти имя sequence (StringConst) внутри Sequence_declaration.
std::string findSequenceName(const FileContent* fC, NodeId seqDeclId) {
  // попытаемся найти все StringConst внутри sequence_declaration
  auto nameNodes = fC->sl_collect_all(seqDeclId, VObjectType::slStringConst);
  for (NodeId n : nameNodes) {
    if (n && fC->Type(n) == VObjectType::slStringConst) {
      return std::string(fC->SymName(n));
    }
  }
  // как fallback можно попробовать первый дочерний StringConst
  NodeId child = fC->Child(seqDeclId);
  if (child && fC->Type(child) == VObjectType::slStringConst) {
    return std::string(fC->SymName(child));
  }
  return "<unknown>";
}

// внутри одной Sequence_expr не должно быть одновременно
// paGoto_repetition и paNon_consecutive_repetition
void checkRepetitionInSequence(const FileContent* fC, ErrorContainer* errors,
                               SymbolTable* symbols) {
  if (!fC) return;

  NodeId root = fC->getRootNode();
  if (!root) return;

  // Ищем все Sequence_declaration
  auto seqDecls = fC->sl_collect_all(root, VObjectType::paSequence_declaration);

  for (NodeId seqDeclId : seqDecls) {
    // Имя sequence (если есть)
    std::string seqName = findSequenceName(fC, seqDeclId);

    // Находим все Sequence_expr в данной sequence_declaration
    auto seqExprs = fC->sl_collect_all(seqDeclId, VObjectType::paSequence_expr);

    for (NodeId seqExprId : seqExprs) {
      // Найдём внутри seqExpr разные типы повторений
      auto gotoNodes =
          fC->sl_collect_all(seqExprId, VObjectType::paGoto_repetition);
      auto nonConsecNodes = fC->sl_collect_all(
          seqExprId, VObjectType::paNon_consecutive_repetition);

      bool hasGoto = !gotoNodes.empty();
      bool hasNonConsec = !nonConsecNodes.empty();

      if (hasGoto && hasNonConsec) {
        auto fileId = fC->getFileId(seqExprId);
        uint32_t line = fC->Line(seqExprId);
        uint32_t column = 0;
        try {
          column = fC->Column(seqExprId);
        } catch (...) {
          column = 0;
        }

        SymbolId obj = symbols->registerSymbol(seqName);

        Location loc(fileId, line, column, obj);
        Error err(ErrorDefinition::REPETITION_IN_SEQUENCE, loc);

        errors->addError(err, false);
      }
    }
  }
}

}  // namespace Analyzer
