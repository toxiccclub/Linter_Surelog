#include <cstdint>
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

std::string getCoverpointName(const FileContent* fC, NodeId cpNode) {
  for (NodeId node = fC->Child(cpNode); node; node = fC->Sibling(node)) {
    if (fC->Type(node) == VObjectType::slStringConst) {
      return std::string(fC->SymName(node));
    }
  }
  return "<unknown>";
}

bool isIntegralType(VObjectType type) {
  switch (type) {
    case VObjectType::paIntVec_TypeBit:
    case VObjectType::paIntVec_TypeLogic:
    case VObjectType::paIntegerAtomType_Int:
    case VObjectType::paIntegerAtomType_LongInt:
    case VObjectType::paIntegerAtomType_Shortint:
    case VObjectType::paIntegerAtomType_Byte:
    case VObjectType::paEnum_base_type:
      return true;
    default:
      return false;
  }
}

VObjectType getVariableType(const FileContent* fC, NodeId exprNode) {
  if (!exprNode) return VObjectType::slNoType;

  // Находим идентификатор переменной (slStringConst) внутри выражения
  // coverpoint
  NodeId idNode = exprNode;
  while (idNode && fC->Type(idNode) != VObjectType::slStringConst) {
    idNode = fC->Child(idNode);
  }
  if (!idNode) return VObjectType::slNoType;

  std::string varName = std::string(fC->SymName(idNode));

  // Ищем все объявления переменных
  auto varDeclNodes = fC->sl_collect_all(fC->getRootNode(),
                                         VObjectType::paVariable_declaration);
  for (NodeId varDeclId : varDeclNodes) {
    auto assignNodes = fC->sl_collect_all(
        varDeclId, VObjectType::paVariable_decl_assignment, false);
    for (NodeId assignId : assignNodes) {
      NodeId nameNode = fC->Child(assignId);
      if (!nameNode) continue;

      std::string declName = std::string(fC->SymName(nameNode));
      if (declName == varName) {
        NodeId typeNode = fC->Child(varDeclId);
        if (typeNode) {
          NodeId baseTypeNode = fC->Child(typeNode);
          if (baseTypeNode) {
            return fC->Type(baseTypeNode);
          }
        }
      }
    }
  }
  //аргументы covergroup-функций
  auto tfItems =
      fC->sl_collect_all(fC->getRootNode(), VObjectType::paTf_port_item);
  for (NodeId tfId : tfItems) {
    NodeId nameNode = fC->Child(tfId);
    while (nameNode && fC->Type(nameNode) != VObjectType::slStringConst)
      nameNode = fC->Sibling(nameNode);

    if (!nameNode) continue;

    if (std::string(fC->SymName(nameNode)) == varName) {
      NodeId dtNode = fC->Child(tfId);
      if (dtNode) {
        NodeId dataType = fC->Child(dtNode);
        if (dataType) {
          NodeId base = fC->Child(dataType);
          if (base) return fC->Type(base);
        }
      }
    }
  }

  return VObjectType::slNoType;
}

// Проверка одного coverpoint
void checkSingleCoverpoint(const FileContent* fC, NodeId cpId,
                           ErrorContainer* errors, SymbolTable* symbols) {
  NodeId exprNode;

  // Находим выражение внутри coverpoint
  for (NodeId child = fC->Child(cpId); child; child = fC->Sibling(child)) {
    if (fC->Type(child) == VObjectType::paPrimary ||
        fC->Type(child) == VObjectType::paExpression) {
      exprNode = child;
      break;
    }
  }
  if (!exprNode) return;

  // Получаем тип переменной через объявление
  VObjectType varType = getVariableType(fC, exprNode);

  if (!isIntegralType(varType)) {
    std::string cpName = getCoverpointName(fC, cpId);

    auto fileId = fC->getFileId(cpId);
    uint32_t line = fC->Line(cpId);
    uint32_t column = 0;
    try {
      column = fC->Column(cpId);
    } catch (...) {
      column = 0;
    }

    SymbolId obj = symbols->registerSymbol(cpName);
    Location loc(fileId, line, column, obj);
    Error err(ErrorDefinition::LINT_COVERPOINT_EXPRESSION_TYPE, loc);
    errors->addError(err, false);
  }
}

// Основная функция проверки всех coverpoints
void checkCoverpointExpressionType(const FileContent* fC,
                                   ErrorContainer* errors,
                                   SymbolTable* symbols) {
  NodeId root = fC->getRootNode();
  auto coverpoints = fC->sl_collect_all(root, VObjectType::paCover_point);

  for (NodeId cpId : coverpoints) {
    checkSingleCoverpoint(fC, cpId, errors, symbols);
  }
}

}  // namespace Analyzer