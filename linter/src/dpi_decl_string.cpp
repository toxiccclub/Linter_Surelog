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

static std::string trim(const std::string& s) {
  auto start = s.find_first_not_of(" \t\n\r");
  if (start == std::string::npos) return "";
  auto end = s.find_last_not_of(" \t\n\r");
  return s.substr(start, end - start + 1);
}

void checkDpiDeclarationString(const FileContent* fC, ErrorContainer* errors, SymbolTable* symbols) {
  NodeId root = fC->getRootNode();

  // DPI-import/export
  auto dpiNodes = fC->sl_collect_all(root, VObjectType::paDpi_import_export);

  for (NodeId dpiId : dpiNodes) {
    // Ищем IMPORT
    NodeId importNode = fC->Child(dpiId);
    if (!importNode || fC->Type(importNode) != VObjectType::paIMPORT) continue;

    // Ищем StringLiteral
    NodeId stringNode = fC->Sibling(importNode);
    if (!stringNode || fC->Type(stringNode) != VObjectType::slStringLiteral)
      continue;

    std::string dpiStr = std::string(fC->SymName(stringNode));

    // Убираем кавычки
    if (!dpiStr.empty() && dpiStr.front() == '"' && dpiStr.back() == '"') {
      dpiStr = dpiStr.substr(1, dpiStr.size() - 2);
    }

    dpiStr = trim(dpiStr);

    // Проверка значения
    if (dpiStr != "DPI-C" && dpiStr != "DPI") {
      auto fileId = fC->getFileId(stringNode);
      uint32_t line = fC->Line(stringNode);
      uint32_t column = 0;

      try {
        column = fC->Column(stringNode);
      } catch (...) {
        column = 0;
      }

      SymbolId obj = symbols->registerSymbol(dpiStr);


      Location loc(fileId, line, column, obj);
      Error err(ErrorDefinition::LINT_DPI_DECLARATION_STRING, loc);
      errors->addError(err, false);
    }
  }
}
}
