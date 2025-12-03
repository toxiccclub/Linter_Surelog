#pragma once

#include <string>

#include "Surelog/Design/FileContent.h"
#include "Surelog/SourceCompile/VObjectTypes.h"

using namespace SURELOG;

namespace Analyzer {
void checkPrototypeReturnDataType(const FileContent* fC, ErrorContainer* errors,
                                  SymbolTable* symbols);

std::string getFunctionName(const FileContent* fC, NodeId typeNode);
bool hasReturnType(const FileContent* fC, NodeId typeNode);
void checkFunctionPrototype(const FileContent* fC, NodeId protoId,
                            ErrorContainer* errors, SymbolTable* symbols);
}  // namespace Analyzer
