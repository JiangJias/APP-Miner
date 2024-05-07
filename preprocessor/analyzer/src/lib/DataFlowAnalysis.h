#ifndef DATA_FLOW_ANALYSIS_H
#define DATA_FLOW_ANALYSIS_H

#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/CFG.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Format.h>
#include <set>
#include <list>
#include <map>
#include <string>
#include <time.h>
#include "Analyzer.h"
#include "Common.h"

using namespace llvm;

class DataFlowAnalysis {

	public:
		DataFlowAnalysis(GlobalContext *GCtx) {Ctx = GCtx;}
		~DataFlowAnalysis() {}

		void findForwardRelateVariable(Value *V, map<Value *, set<int>> &FRVSet, map<Value *, set<int>> &TrackedSet, int parameter, bool constantPointer = false);
		void findBackwardRelateVariable(map<Value *, set<int>> &FRVSet, map<Value *, set<int>> &BRVSet, map<Value *, set<int>> &TrackedSet);
		void findForwardRelateOperation(BasicBlock *BB, map<Value *, set<int>> &FRVSet, APIPath *AP, APIPath *nextbegin, map<BasicBlock *, pair<APIPath *, APIPath *> > &visitBBs, set<APIPath *> &APList, map<Value *, set<int>> &TrackedSet);
		void findBackwardRelateOperation(BasicBlock *BB, map<Value *, set<int>> &BRVSet, APIPath *AP, APIPath *prevend, map<BasicBlock *, pair<APIPath *, APIPath *> > &visitBBs, set<APIPath *> &APList, map<Value *, set<int>> &TrackedSet);
		void findRelateOperation(BasicBlock *BB, map<Value *, set<int>> &BRVSet, APIPath *AP, APIPath *prevend, map<BasicBlock *, pair<APIPath *, APIPath *> > &visitBBs, set<APIPath *> &APList, map<Value *, set<int>> &TrackedSet);
		std::string generateAPIPath(APIPath *AP);
		void printForwardAPIPath(APIPath *AP, set<APIPath *> &TrackedSet);
		void printBackwardAPIPath(APIPath *AP, set<APIPath *> &TrackedSet);
		void printAPIPath(APIPath *AP);
		bool isRelateOperation(Value *V, map<Value *, set<int>> &RVSet, map<Value *, set<int>> &TrackedSet);
		void releaseAPIPath(set<APIPath *> &APList, APIPath *AP);
		void clearEmptyAPIPath(set<APIPath *> &APList);
		void printRelateVariable(map<Value *, set<int>> &RVSet);
		std::string outputAPIPath(set<APIPath *> &APList);
		void collectNext(APIPath *AP, set<APIPath *> &path);
		bool isTimeout();
		bool isConstant(Value *V);

		clock_t fileStartTime;
		clock_t instructionStartTime;
		clock_t functionStartTime;
		string fileName;
		Function *function;
		string program;
		CallInst *API;
	private:
		GlobalContext *Ctx;
};

#endif

