
#include <string>

#include "Surelog/Design/Design.h"
#include "Surelog/Design/FileContent.h"
#include "Surelog/ErrorReporting/ErrorContainer.h"
#include "Surelog/SourceCompile/SymbolTable.h"

using namespace SURELOG;

namespace Analyzer {

void checkRepetitionInSequence(const FileContent* fC, ErrorContainer* errors,
                               SymbolTable* symbols);

}