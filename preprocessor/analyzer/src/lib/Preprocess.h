#include "Analyzer.h"
#include "DataFlowAnalysis.h"
#include "Common.h"


//
// Modeling Preprocess
//
class PreprocessPass : public IterativeModulePass {

	public:
		PreprocessPass(GlobalContext *Ctx_)
			: IterativeModulePass(Ctx_, "Preprocess"), 
			DFA(Ctx_) {
				MIdx = 0;
			}
		virtual bool doInitialization(llvm::Module *);
		virtual bool doFinalization(llvm::Module *);
		virtual bool doModulePass(llvm::Module *, StringRef, int, string);

		// Process final results
		void processResults();

	private:
		DataFlowAnalysis DFA;
		int MIdx;
};
