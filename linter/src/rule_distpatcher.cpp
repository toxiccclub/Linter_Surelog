#include <iostream>
#include "Surelog/Common/FileSystem.h"
#include "Surelog/SourceCompile/VObjectTypes.h"
#include "repet_in_sequence.h"
#include "prototype_return_data_type.h"
#include "parametr_dynamic_array.h"
#include "imlicit_data_type.h"
#include "hierarchical_interface_identifier.h"
#include "dpi_decl_string.h"
#include "class_variable_lifetime.h"
#include "Surelog/Design/FileContent.h"
#include "fatal_rule.h"

using namespace SURELOG;

void runAllRules(const FileContent* fC) {
    Analyzer::checkRepetitionInSequence(fC);
    Analyzer::checkPrototypeReturnDataType(fC);
    Analyzer::checkParameterDynamicArray(fC);
    Analyzer::checkImplicitDataTypeInDeclaration(fC);
    Analyzer::checkHierarchicalInterfaceIdentifier(fC);
    Analyzer::checkDpiDeclarationString(fC);
    Analyzer::checkClassVariableLifetime(fC);
}


void runAllRulesOnDesign(Design* design, const vpiHandle &UHDMdesign) {
    if (!design) return;

    for (auto& it : design->getAllFileContents()) {
        const FileContent* fC = it.second;
        if (!fC) continue;

        runAllRules(fC);
        FatalListener listener;
        listener.listen(UHDMdesign);
    }
}
