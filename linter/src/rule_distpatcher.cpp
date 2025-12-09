#include <iostream>

#include "Surelog/Common/FileSystem.h"
#include "Surelog/Design/FileContent.h"
#include "Surelog/SourceCompile/VObjectTypes.h"
#include "class_variable_lifetime.h"
#include "covergroup_expression.h"
#include "coverpoint_expression_type.h"
#include "dpi_decl_string.h"
#include "fatal_rule.h"
#include "hierarchical_interface_identifier.h"
#include "imlicit_data_type.h"
#include "parametr_dynamic_array.h"
#include "prototype_return_data_type.h"
#include "repet_in_sequence.h"

using namespace SURELOG;

void runAllRules(const FileContent* fC, ErrorContainer* errors,
                 SymbolTable* symbols) {
  Analyzer::checkRepetitionInSequence(fC, errors, symbols);
  Analyzer::checkPrototypeReturnDataType(fC, errors, symbols);
  Analyzer::checkParameterDynamicArray(fC, errors, symbols);
  Analyzer::checkImplicitDataTypeInDeclaration(fC, errors, symbols);
  Analyzer::checkHierarchicalInterfaceIdentifier(fC, errors, symbols);
  Analyzer::checkDpiDeclarationString(fC, errors, symbols);
  Analyzer::checkClassVariableLifetime(fC, errors, symbols);
  Analyzer::checkCoverpointExpressionType(fC, errors, symbols);
  Analyzer::checkCovergroupExpression(fC, errors, symbols);
}

void runAllRulesOnDesign(Design* design, const vpiHandle& UHDMdesign,
                         ErrorContainer* errors, SymbolTable* symbols) {
  if (!design) return;

  for (auto& it : design->getAllFileContents()) {
    const FileContent* fC = it.second;
    if (!fC) continue;

    runAllRules(fC, errors, symbols);
    FatalListener listener(fC, errors, symbols);
    listener.listen(UHDMdesign);
  }
}
