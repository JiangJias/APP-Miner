#include <llvm/IR/DebugInfo.h>
#include <llvm/Pass.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/Debug.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/CFG.h>

#include "Preprocess.h"

bool PreprocessPass::doInitialization(Module *M) {
	return false;
}

bool PreprocessPass::doFinalization(Module *M) {
	return false;
}

bool PreprocessPass::doModulePass(Module *M, StringRef FilePath, int counter_modules, string ProgramPath) {
	set<string> violate;
	DFA.fileName = FilePath.str().substr(ProgramPath.length(), FilePath.str().length() - ProgramPath.length() - 2) + 'c';
	string::size_type iPos = ProgramPath.substr(0, ProgramPath.length() - 1).find_last_of("/") + 1;
	DFA.program = ProgramPath.substr(iPos, ProgramPath.length() - iPos - 1);
	DFA.fileStartTime = clock();
	for(Module::iterator f = M->begin(), fe = M->end(); f != fe; ++f) {
		Function *F = &*f;
		if (F->empty())
			continue;
		DFA.function = F;
		DFA.functionStartTime = clock();
		int count = 0;
		for (inst_iterator i = inst_begin(F), ei = inst_end(F); i != ei; ++i) {
			Instruction *Inst = &*i;
			if (CallInst *CI = dyn_cast<CallInst>(Inst)) {
				Function *CF = CI->getCalledFunction();
				if (!CF || CF->getName().contains("llvm.dbg.") || CF->getName().contains("llvm.lifetime.") || CF->getName().contains("llvm.assume") || CF->getName().contains("llvm.frameaddress") || CF->getName().contains("llvm.fshl") || CF->getName().contains("llvm.returnaddress")) {
					continue;
				}
				DFA.instructionStartTime = clock();
				DFA.API = CI;
				APIPath *AP = new APIPath();
				AP->name = CF->getName();
				string topology = DFA.generateAPIPath(AP);
				string result;
				result += "{'file': '" + DFA.fileName;
				result += "', 'function': '" + (DFA.function->getName()).str();
				result += "', 'API': '" + (CF->getName()).str();
				result += "', 'parameter': [";
	            bool flag = false;
				for (int parameter : AP->parameter) {
					result += to_string(parameter) + ", ";
					flag = true;
				}
				if (flag) {
					result = result.substr(0, result.size() - 2);
				}
				ostringstream ossAPI;
				ossAPI << AP;
				result += "], 'address': '" + ossAPI.str();
				result += "', 'path': " + topology + "}";
				ofstream outfile;
				string CFName = (CF->getName()).str();
				string parameterSize = to_string(AP->parameter.size());
				if (access(("tmp/" + DFA.program + "/dfa/" + CFName + "+" + parameterSize + "/").c_str(), 0) == -1) {
					mkdir(("tmp/" + DFA.program + "/dfa/" + CFName + "+" + parameterSize + "/").c_str(), 0777);
				}
				outfile.open("tmp/" + DFA.program + "/dfa/" + (CF->getName()).str() + "+" + to_string(AP->parameter.size()) + "/" + to_string(counter_modules), ios::app);
				outfile << result << '\n';
				outfile.close();
				delete(AP);
			}
		}
	}
	return false;
}
