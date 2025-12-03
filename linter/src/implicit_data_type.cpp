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

// Извлекаем имя переменной из Variable_decl_assignment
std::string findVarName(const FileContent* fC, NodeId dataDecl) {
  auto lists = fC->sl_collect_all(
      dataDecl, VObjectType::paList_of_variable_decl_assignments);
  for (NodeId list : lists) {
    auto assigns =
        fC->sl_collect_all(list, VObjectType::paVariable_decl_assignment);
    for (NodeId assign : assigns) {
      NodeId child = fC->Child(assign);
      if (child && fC->Type(child) == VObjectType::slStringConst)
        return std::string(fC->SymName(child));
    }
  }
  return "<unknown>";
}

// Проверка наличия явного типа
bool hasExplicitType(const FileContent* fC, NodeId dataDecl) {
  static const VObjectType typeNodes[] = {
      VObjectType::paNet_type,          VObjectType::paData_type,
      VObjectType::paInteger_atom_type, VObjectType::paInteger_vector_type,
      VObjectType::paNon_integer_type,  VObjectType::paString_type,
      VObjectType::paClass_type,        VObjectType::paIntVec_TypeBit};

  for (auto t : typeNodes) {
    if (!fC->sl_collect_all(dataDecl, t).empty()) return true;
  }
  return false;
}

void checkImplicitDataTypeInDeclaration(const FileContent* fC,
                                        ErrorContainer* errors,
                                        SymbolTable* symbols) {
  NodeId root = fC->getRootNode();

  // Ищем Data_declaration
  auto dataDecls = fC->sl_collect_all(root, VObjectType::paData_declaration);

  for (NodeId dataDecl : dataDecls) {
    auto packedDims =
        fC->sl_collect_all(dataDecl, VObjectType::paPacked_dimension);
    if (packedDims.empty()) continue;

    if (hasExplicitType(fC, dataDecl)) continue;

    // Ошибка IMPLICIT_DATA_TYPE_IN_DECLARATION
    std::string varName = findVarName(fC, dataDecl);

    NodeId where = packedDims.front();
    auto fileId = fC->getFileId(where);
    uint32_t line = fC->Line(where);
    uint32_t column = 0;
    try {
      column = fC->Column(where);
    } catch (...) {
      column = 0;
    }

    SymbolId obj = symbols->registerSymbol(varName);

    Location loc(fileId, line, column, obj);

    Error err(ErrorDefinition::IMPLICIT_DATA_TYPE_IN_DECLARATION, loc);

    errors->addError(err, false);
  }
}
}  // namespace Analyzer
