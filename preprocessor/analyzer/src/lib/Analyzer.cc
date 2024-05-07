#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/SystemUtils.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/Path.h"

#include <memory>
#include <vector>
#include <sstream>
#include <sys/resource.h>
#include <unistd.h>

#include "Analyzer.h"
#include "Preprocess.h"

using namespace llvm;

static const int cpu_total = sysconf( _SC_NPROCESSORS_CONF) / 4;
int cpu_rest = cpu_total;

// Command line parameters.
cl::opt<string> ProgramPath(cl::Positional, cl::desc("The program path"));

cl::list<string> InputFilenames(cl::Positional, cl::OneOrMore, cl::desc("<input bitcode files>"));

GlobalContext GlobalCtx;

void IterativeModulePass::run(ModuleList &modules) {

	ModuleList::iterator i, e;
	int counter_modules = 0;
	unsigned total_modules = modules.size();
	pid_t pid;
	int status = 0;
	for (i = modules.begin(), e = modules.end(); i != e; ++i) {
		while (cpu_rest <= 0) {
			pid = wait(&status);
			cpu_rest++;
		}
		pid_t pid = fork();
		if (pid < 0){
			OP << "fork error: " << pid << "\n";
			return;
		}
		else if(pid == 0) {
			doModulePass(i->first, i->second, counter_modules, ProgramPath);
			exit(0);
		}
		else {
			cpu_rest--;
		}
	}
	while (cpu_rest < cpu_total) {
		pid = wait(&status);
		cpu_rest++;
	}
}

int main(int argc, char **argv) {
	// Print a stack trace if we signal out.
	sys::PrintStackTraceOnErrorSignal(argv[0]);
	PrettyStackTraceProgram X(argc, argv);

	llvm_shutdown_obj Y;  // Call llvm_shutdown() on exit.

	cl::ParseCommandLineOptions(argc, argv, "global analysis\n");
	SMDiagnostic Err;

	// Loading modules
	string::size_type iPos = ProgramPath.substr(0, ProgramPath.length() - 1).find_last_of("/") + 1;
	string program = ProgramPath.substr(iPos, ProgramPath.length() - iPos - 1);
	if (access(("tmp/" + program + "/dfa/").c_str(), 0) != -1) {
		system(("rm -rf tmp/" + program + "/dfa/").c_str());
	}
	mkdir(("tmp/" + program + "/dfa/").c_str(), 0777);
	if (access(("tmp/" + program + "/timeout").c_str(), 0) != -1) {
		system(("rm -rf tmp/" + program + "/timeout").c_str());
	}

	for (unsigned i = 0; i < InputFilenames.size(); ++i) {
		LLVMContext *LLVMCtx = new LLVMContext();
		unique_ptr<Module> M = parseIRFile(InputFilenames[i], Err, *LLVMCtx);
		
		if (M == NULL) {
			OP << argv[0] << ": error loading file '" << InputFilenames[i] << "'\n";
			continue;
		}

		Module *Module = M.release();
		StringRef MName = StringRef(strdup(InputFilenames[i].data()));
		GlobalCtx.Modules.push_back(make_pair(Module, MName));
		GlobalCtx.ModuleMaps[Module] = InputFilenames[i];
	}
	
	PreprocessPass PPPass(&GlobalCtx);
	PPPass.run(GlobalCtx.Modules);	

	return 0;
}

