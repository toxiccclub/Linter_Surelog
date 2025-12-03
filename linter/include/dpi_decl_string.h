#pragma once

#include "Surelog/Design/Design.h"

using namespace SURELOG;

namespace Analyzer {

void checkDpiDeclarationString(const FileContent* fC, ErrorContainer* errors, SymbolTable* symbols);

}