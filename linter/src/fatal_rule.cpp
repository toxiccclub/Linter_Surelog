#include <functional>
#include <iostream>
#include <string>
#include <vector>
#include <set>

#include "Surelog/API/Surelog.h"
#include "Surelog/CommandLine/CommandLineParser.h"
#include "Surelog/ErrorReporting/ErrorContainer.h"
#include "Surelog/SourceCompile/SymbolTable.h"
#include "fatal_rule.h"

// UHDM
#include <uhdm/VpiListener.h>
#include <uhdm/uhdm.h>
#include <uhdm/vpi_user.h>


void FatalListener::listen(const vpiHandle& design) {
    if (!design) return;
    listenDesigns({design});
}

void FatalListener::enterSys_func_call(const UHDM::sys_func_call* object, vpiHandle handle) {
    if (!object) return;
    if (seen_.count(object)) return; // предотвращаем дубли
    seen_.insert(object);

    if (object->VpiName() != "$fatal") return;

    const char* file = nullptr;
    int line = 0;
    if (handle) {
        file = vpi_get_str(vpiFile, handle);
        line = vpi_get(vpiLineNo, handle);
    }

    const UHDM::VectorOfany* args = object->Tf_call_args();
    if (!args || args->empty()) {
        std::cout << "Error: $fatal has no arguments at " 
                  << (file ? file : "<unknown>") << ":" << line << "\n";
        return;
    }

    UHDM::any* firstArg = (*args)[0];
    if (!firstArg) return;

    int val = 0;
    bool isInteger = false;

    // CASE 1: constant
    if (auto c = dynamic_cast<UHDM::constant*>(firstArg)) {
        int ctype = c->VpiConstType();
        isInteger = (ctype == vpiIntConst || ctype == vpiDecConst ||
                     ctype == vpiHexConst || ctype == vpiOctConst ||
                     ctype == vpiBinaryConst || ctype == vpiUIntConst);

        if (isInteger) {
            std::string raw = std::string(c->VpiValue());
            size_t pos = raw.find(':');
            if (pos != std::string::npos) raw = raw.substr(pos + 1);
            try { val = std::stoi(raw); } catch (...) { isInteger = false; }
        }
    }

    // CASE 2: unary +/- operation
    else if (auto op = dynamic_cast<UHDM::operation*>(firstArg)) {
        int opType = op->VpiOpType();
        if ((opType == vpiPlusOp || opType == vpiMinusOp) &&
            op->Operands() && !op->Operands()->empty()) {
            if (auto c = dynamic_cast<UHDM::constant*>((*op->Operands())[0])) {
                std::string raw = std::string(c->VpiValue());
                size_t pos = raw.find(':');
                if (pos != std::string::npos) raw = raw.substr(pos + 1);
                try { val = std::stoi(raw); } catch (...) { isInteger = false; }
                if (opType == vpiMinusOp) val = -val;
                isInteger = true;
            }
        }
    }

    if (isInteger) {
        if (!(val == 0 || val == 1 || val == 2)) {
            std::cout << "Error: $fatal first argument must be 0, 1, or 2. Got " << val
                      << " at " << (file ? file : "<unknown>") << ":" << line << "\n";
        }
    } else {
        std::cout << "Error: $fatal first argument is not constant at "
                  << (file ? file : "<unknown>") << ":" << line << "\n";
    }

    // SECOND ARG (message)
    if (args->size() > 1) {
        auto secondArg = (*args)[1];
        if (!dynamic_cast<UHDM::constant*>(secondArg)) {
            std::cout << "Warning: $fatal message is not a string constant at "
                      << (file ? file : "<unknown>") << ":" << line << "\n";
        }
    } else {
        std::cout << "Warning: $fatal missing message at "
                  << (file ? file : "<unknown>") << ":" << line << "\n";
    }
}
