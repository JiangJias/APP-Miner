//===-- DataFlowAnalysis.cc - Utils for data-flow analysis------===//
// 
// This file implements commonly used functions for data-flow
// analysis
//
//===-----------------------------------------------------------===//

#include <llvm/IR/InstIterator.h>

#include "DataFlowAnalysis.h"


/// Check if the value is a constant.
bool DataFlowAnalysis::isConstant(Value *V) {
	// Invalid input.
	if (!V)
		return false;

	// The value is a constant.
	Constant *Ct = dyn_cast<Constant>(V);
	if (Ct)
		return true;

	return false;
}

void DataFlowAnalysis::findForwardRelateVariable(Value *V, map<Value *, set<int>> &FRVSet, map<Value *, set<int>> &TrackedSet, int parameter, bool constantPointer) {
	if (isTimeout()) {
		exit(EXIT_FAILURE);
	}
	if (parameter == -1) {
		if (FRVSet.find(V) != FRVSet.end()) {
			set<int> parameterVs;
			for (int parameterV : FRVSet[V]) {
				parameterVs.insert(parameterV);
			}
			set<Value *> relateVariable;
			for (auto Tracked : TrackedSet) {
				if (Tracked.second.find(-1) != Tracked.second.end()) {
					relateVariable.insert(Tracked.first);
				}
			}
			for (Value *relate : relateVariable) {
				TrackedSet[relate].erase(-1);
				for (int parameterV : parameterVs) {
					TrackedSet[relate].insert(parameterV);
				}
				if (FRVSet.find(relate) != FRVSet.end()) {
					FRVSet[relate].erase(-1);
					for (int parameterV : parameterVs) {
						FRVSet[relate].insert(parameterV);
					}
				}
			}
			return;
		}
	}
	else {
		if (TrackedSet.find(V) != TrackedSet.end() && TrackedSet[V].find(parameter) != TrackedSet[V].end()) {
			return;
		}
	}
	TrackedSet[V].insert(parameter);
	if (isConstant(V)) {
		//OP << "Constant: " << *V << "\n";
		if (constantPointer) {
			//OP << "ConstantPointer: " << *V << "\n";
        	FRVSet[V].insert(parameter);
		}
		return;
	}
	if (isa<Argument>(V) || isa<GlobalVariable>(V) || isa<AllocaInst>(V)) {
		//OP << "Argument: " << *V << "\n";
		FRVSet[V].insert(parameter);
		return;
	}

	if (isa<ConstantExpr>(V)) {
		//OP << "ConstantExpr: " << *V << "\n";
		ConstantExpr *CE = dyn_cast<ConstantExpr>(V);
		findForwardRelateVariable(CE->getOperand(0), FRVSet, TrackedSet, parameter);
		return;
	}

	if (CallInst *CI = dyn_cast<CallInst>(V)) {
		//OP << "CallInst: " << *CI << "\n";
		FRVSet[V].insert(parameter);
		for (unsigned int i = 0; i < CI->getNumArgOperands(); ++i) {
			findForwardRelateVariable(CI->getArgOperand(i), FRVSet, TrackedSet, parameter);				
		}
		return;
	}

	if (SelectInst *SI = dyn_cast<SelectInst>(V)) {
		//OP << "SelectInst: " << *SI << "\n";
		FRVSet[V].insert(parameter);
		findForwardRelateVariable(SI->getTrueValue(), FRVSet, TrackedSet, parameter);
		findForwardRelateVariable(SI->getFalseValue(), FRVSet, TrackedSet, parameter);
		return;
	}

	if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(V)) {
		//OP << "GetElementPtrInst: " << *GEP << "\n";
		//FRVSet[V].insert(parameter);
		return;
	}

	if (PHINode *PN = dyn_cast<PHINode>(V)) {
		//OP << "PHINode: " << *PN << "\n";
		FRVSet[V].insert(parameter);
		for (unsigned i = 0, e = PN->getNumIncomingValues(); i != e; ++i) {
			Value *IV = PN->getIncomingValue(i);
			findForwardRelateVariable(IV, FRVSet, TrackedSet, parameter);
		}
		return;
	}

	if (ICmpInst *ICmp = dyn_cast<ICmpInst>(V)) {
		//OP << "ICmpInst: " << *ICmp << "\n";
		FRVSet[V].insert(parameter);
		for (unsigned i = 0, ie = ICmp->getNumOperands(); i < ie; i++) {
			Value *SCOpd = ICmp->getOperand(i);
			findForwardRelateVariable(SCOpd, FRVSet, TrackedSet, parameter);
		}
		return;
	}

	if (LoadInst *LI = dyn_cast<LoadInst>(V)) {
		//OP << "LoadInst: " << *LI << "\n";
		FRVSet[V].insert(parameter);
		Value *LPO = LI->getPointerOperand();
		findForwardRelateVariable(LPO, FRVSet, TrackedSet, parameter);
		return;
	}
	/*
	if (BitCastInst *BCI = dyn_cast<BitCastInst>(V)) {
		return;
	}
	*/
	if (UnaryInstruction *UI = dyn_cast<UnaryInstruction>(V)) {
		//OP << "UnaryInstruction: " << *UI << "\n";
		FRVSet[V].insert(parameter);
		Value *UO = UI->getOperand(0);
		findForwardRelateVariable(UO, FRVSet, TrackedSet, parameter);
		return;
	}

	if (BinaryOperator *BO = dyn_cast<BinaryOperator>(V)) {
		//OP << "BinaryOperator: " << *BO << "\n";
		FRVSet[V].insert(parameter);
		for (unsigned i = 0, e = BO->getNumOperands();
				i != e; ++i) {
			Value *Opd = BO->getOperand(i);
			findForwardRelateVariable(Opd, FRVSet, TrackedSet, parameter);
		}
		return;
	}

	if (StoreInst *SI = dyn_cast<StoreInst>(V)) {
		//OP << "StoreInst: " << *SI << "\n";
		FRVSet[V].insert(parameter);
		Value *SVO = SI->getValueOperand();
		Value *SPO = SI->getPointerOperand();
		findForwardRelateVariable(SVO, FRVSet, TrackedSet, parameter);
		findForwardRelateVariable(SPO, FRVSet, TrackedSet, parameter, true);
		return;
	}

	//OP << "Unknown Value: " << *V << "\n";
}

void DataFlowAnalysis::findBackwardRelateVariable(map<Value *, set<int>> &FRVSet, map<Value *, set<int>> &BRVSet, map<Value *, set<int>> &TrackedSet) {
	if (isTimeout()) {
		exit(EXIT_FAILURE);
	}
	for (auto FRV : FRVSet) {
		for (int parameter : FRV.second) {
			BRVSet[FRV.first].insert(parameter);
		}
		for (User *U : FRV.first->users()) {
			if (isConstant(U)) {
				continue;
			}
			if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(U)) {
				continue;
			}
			/*
			if (BitCastInst *BCI = dyn_cast<BitCastInst>(U)) {
				return;
			}
			*/
			for (int parameter : FRV.second) {
				if (StoreInst *SI = dyn_cast<StoreInst>(U)) {
					Value *SVO = SI->getValueOperand();
					Value *SPO = SI->getPointerOperand();
					if (SVO == FRV.first) {
						BRVSet[SPO].insert(parameter);
					}
				}
				BRVSet[U].insert(parameter);
			}
		}
	}
	bool flag = false;
	for (auto BRV : BRVSet) {
		for (int parameter : BRV.second) {
			if (FRVSet.find(BRV.first) != FRVSet.end() && FRVSet[BRV.first].find(parameter) != FRVSet[BRV.first].end()) {
				continue;
			}
			FRVSet[BRV.first].insert(parameter);
			flag = true;
		}
	}
	if (!flag) {
		return;
	}
	findBackwardRelateVariable(FRVSet, BRVSet, TrackedSet);
}

bool DataFlowAnalysis::isRelateOperation(Value *V, map<Value *, set<int>> &RVSet, map<Value *, set<int>> &TrackedSet) {
	if (isTimeout()) {
		exit(EXIT_FAILURE);
	}
	if (!V) {
		return false;
	}
	if (RVSet.find(V) != RVSet.end()) {
		return true;
	}
	//OP << "isRelateOperation: " << *V << "\n";
	findForwardRelateVariable(V, RVSet, TrackedSet, -1);
	set<Value *> unrelateVariable;
	for (auto Tracked : TrackedSet) {
		if (Tracked.second.find(-1) != Tracked.second.end()) {
			unrelateVariable.insert(Tracked.first);
		}
	}
	for (Value *unrelate : unrelateVariable) {
		TrackedSet[unrelate].erase(-1);
		if (TrackedSet[unrelate].size() == 0) {
			TrackedSet.erase(unrelate);
		}
		if (RVSet.find(unrelate) != RVSet.end()) {
			RVSet[unrelate].erase(-1);
			if (RVSet[unrelate].size() == 0) {
				RVSet.erase(unrelate);
			}
		}
	}
	if (RVSet.find(V) != RVSet.end()) {
		/*
		OP << "find: ";
		for (int parameterV : RVSet[V]) {
			OP << parameterV << " ";
		}
		OP << "\n";
		*/
		return true;
	}
	//OP << "unfind\n";
	return false;
}

void DataFlowAnalysis::findForwardRelateOperation(BasicBlock *BB, map<Value *, set<int>> &FRVSet, APIPath *AP, APIPath *nextbegin, map<BasicBlock *, pair<APIPath *, APIPath *> > &visitBBs, set<APIPath *> &APList, map<Value *, set<int>> &TrackedSet) {
	if (isTimeout()) {
		exit(EXIT_FAILURE);
	}
	if (visitBBs.find(BB) != visitBBs.end()) {
		if (nextbegin) {
			visitBBs[BB].second->next.insert(nextbegin);
			nextbegin->prev.insert(visitBBs[BB].second);
		}
		return;
	}
	APIPath *begin = new APIPath();
	APIPath *end = new APIPath();
	APIPath *current = begin;
	APList.insert(begin);
	APList.insert(end);
	for (BasicBlock::iterator Bit = BB->begin(), Bite = BB->end(); Bit != Bite; Bit++) {
		Value *V = &*Bit;
		OP << "CallInst: " << *V << "\n";
		if (V == API) {
			current->next.insert(AP);
			AP->prev.insert(current);
			return;
		}
		if (CallInst *CI = dyn_cast<CallInst>(V)) {
			if (!isRelateOperation(V, FRVSet, TrackedSet)) {
				continue;
			}
			Function *CF = CI->getCalledFunction();
			if (!CF || CF->getName().contains("llvm.dbg.") || CF->getName().contains("llvm.lifetime.") || CF->getName().contains("llvm.assume") || CF->getName().contains("llvm.frameaddress") || CF->getName().contains("llvm.fshl")) {
				continue;
			}
			APIPath *temp = new APIPath();
			temp->name = CF->getName();
			for (int parameter : FRVSet[V]) {
				temp->parameter.insert(parameter);
			}
			APList.insert(temp);
			current->next.insert(temp);
			temp->prev.insert(current);
			current = temp;
		}
		/*
		else if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(V)) {
			if (!isRelateOperation(GEP->getPointerOperand(), FRVSet, TrackedSet)) {
				continue;
			}
			APIPath *temp = new APIPath();
			temp->name = "MEMBER";
			for (int parameter : FRVSet[GEP->getPointerOperand()]) {
				temp->parameter.insert(parameter);
			}
			APList.insert(temp);
			current->next.insert(temp);
			temp->prev.insert(current);
			current = temp;
		}
		*/
		else if (isa<BranchInst>(V) || isa<SwitchInst>(V) || isa<SelectInst>(V) || isa<ReturnInst>(V)) {
			Value *Cond;
			if (BranchInst *BI = dyn_cast<BranchInst>(V)) {
				if (BI->getNumSuccessors() < 2) {
					continue;
				}
				Cond = BI->getCondition();
			}
			else if (SwitchInst *SI = dyn_cast<SwitchInst>(V)) {
				if (SI->getNumSuccessors() < 2) {
					continue;
				}
				Cond = SI->getCondition();
			}
			else if (SelectInst *SI = dyn_cast<SelectInst>(V)) {
				Cond = SI->getCondition();
			}
			else if (ReturnInst *RI = dyn_cast<ReturnInst>(V)) {
				Cond = RI->getReturnValue();
			}
			if (!isRelateOperation(Cond, FRVSet, TrackedSet)) {
				continue;
			}
			APIPath *temp = new APIPath();
			temp->name = "CHECK";
			for (int parameter : FRVSet[Cond]) {
				temp->parameter.insert(parameter);
			}
			APList.insert(temp);
			current->next.insert(temp);
			temp->prev.insert(current);
			current = temp;
		}
	}
	current->next.insert(end);
	end->prev.insert(current);

	if (nextbegin) {
		end->next.insert(nextbegin);
		nextbegin->prev.insert(end);
	}

	visitBBs[BB] = make_pair(begin, end);
	//pred_iterator pi = pred_begin(BB), pe = pred_end(BB);
	for (BasicBlock *pred : predecessors(BB)) {
		OP << *pred << "\n";
		findForwardRelateOperation(pred, FRVSet, AP, begin, visitBBs, APList, TrackedSet);
	}
}

void DataFlowAnalysis::findBackwardRelateOperation(BasicBlock *BB, map<Value *, set<int>> &BRVSet, APIPath *AP, APIPath *prevend, map<BasicBlock *, pair<APIPath *, APIPath *> > &visitBBs, set<APIPath *> &APList, map<Value *, set<int>> &TrackedSet) {	
	if (isTimeout()) {
		exit(EXIT_FAILURE);
	}
	if (visitBBs.find(BB) != visitBBs.end()) {
		if (prevend) {
			visitBBs[BB].first->prev.insert(prevend);
			prevend->next.insert(visitBBs[BB].first);
		}
		return;
	}
	APIPath *begin = new APIPath();
	APIPath *end = new APIPath();
	APIPath *current = begin;
	APList.insert(begin);
	APList.insert(end);
	for (BasicBlock::iterator Bit = BB->begin(), Bite = BB->end(); Bit != Bite; Bit++) {
		Value *V = &*Bit;
		if (BB == API->getParent()) {
			if (V == API) {
				current = AP;
				continue;
			}
			if (current->name.empty()) {
				continue;
			}
		}
		if (CallInst *CI = dyn_cast<CallInst>(V)) {
			//OP << "CI: " << *V << "\n";
			if (!isRelateOperation(V, BRVSet, TrackedSet)) {
				continue;
			}
			Function *CF = CI->getCalledFunction();
			if (!CF || CF->getName().contains("llvm.dbg.") || CF->getName().contains("llvm.lifetime.") || CF->getName().contains("llvm.assume") || CF->getName().contains("llvm.frameaddress") || CF->getName().contains("llvm.fshl")) {
				continue;
			}
			APIPath *temp = new APIPath();
			temp->name = CF->getName();
			for (int parameter : BRVSet[V]) {
				temp->parameter.insert(parameter);
			}
			APList.insert(temp);
			current->next.insert(temp);
			temp->prev.insert(current);
			current = temp;
		}
		/*
		else if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(V)) {
			if (!isRelateOperation(GEP->getPointerOperand(), BRVSet, TrackedSet)) {
				continue;
			}
			APIPath *temp = new APIPath();
			temp->name = "MEMBER";
			for (int parameter : BRVSet[GEP->getPointerOperand()]) {
				temp->parameter.insert(parameter);
			}
			APList.insert(temp);
			current->next.insert(temp);
			temp->prev.insert(current);
			current = temp;
		}
		*/
		else if (isa<BranchInst>(V) || isa<SwitchInst>(V) || isa<SelectInst>(V) || isa<ReturnInst>(V)) {
			Value *Cond;
			if (BranchInst *BI = dyn_cast<BranchInst>(V)) {
				if (BI->getNumSuccessors() < 2) {
					continue;
				}
				Cond = BI->getCondition();
			}
			else if (SwitchInst *SI = dyn_cast<SwitchInst>(V)) {
				if (SI->getNumSuccessors() < 2) {
					continue;
				}
				Cond = SI->getCondition();
			}
			else if (SelectInst *SI = dyn_cast<SelectInst>(V)) {
				Cond = SI->getCondition();
			}
			else if (ReturnInst *RI = dyn_cast<ReturnInst>(V)) {
				Cond = RI->getReturnValue();
			}
			if (!isRelateOperation(Cond, BRVSet, TrackedSet)) {
				continue;
			}	
			APIPath *temp = new APIPath();
			temp->name = "CHECK";
			for (int parameter : BRVSet[Cond]) {
				temp->parameter.insert(parameter);
			}
			APList.insert(temp);
			current->next.insert(temp);
			temp->prev.insert(current);
			current = temp;
		}
	}
	current->next.insert(end);
	end->prev.insert(current);

	if (prevend) {
		begin->prev.insert(prevend);
		prevend->next.insert(begin);
	}
	
	visitBBs[BB] = make_pair(begin, end);
	succ_iterator si = succ_begin(BB), se = succ_end(BB);
	for (; si != se; ++si) {
		findBackwardRelateOperation(*si, BRVSet, AP, end, visitBBs, APList, TrackedSet);
	}
}

void DataFlowAnalysis::findRelateOperation(BasicBlock *BB, map<Value *, set<int>> &BRVSet, APIPath *AP, APIPath *prevend, map<BasicBlock *, pair<APIPath *, APIPath *> > &visitBBs, set<APIPath *> &APList, map<Value *, set<int>> &TrackedSet) {
	if (isTimeout()) {
		exit(EXIT_FAILURE);
	}
	if (visitBBs.find(BB) != visitBBs.end()) {
		if (prevend) {
			visitBBs[BB].first->prev.insert(prevend);
			prevend->next.insert(visitBBs[BB].first);
		}
		return;
	}
	APIPath *begin = new APIPath();
	APIPath *end = new APIPath();
	APIPath *current = begin;
	APList.insert(begin);
	APList.insert(end);
	for (BasicBlock::iterator Bit = BB->begin(), Bite = BB->end(); Bit != Bite; Bit++) {
		Value *V = &*Bit;
		if (BB == API->getParent()) {
			if (V == API) {
				current->next.insert(AP);
				AP->prev.insert(current);
				current = AP;
				continue;
			}
		}
		if (CallInst *CI = dyn_cast<CallInst>(V)) {
			//OP << "CI: " << *V << "\n";
			if (!isRelateOperation(V, BRVSet, TrackedSet)) {
				continue;
			}
			Function *CF = CI->getCalledFunction();
			if (!CF || CF->getName().contains("llvm.dbg.") || CF->getName().contains("llvm.lifetime.") || CF->getName().contains("llvm.assume") || CF->getName().contains("llvm.frameaddress") || CF->getName().contains("llvm.fshl")) {
				continue;
			}
			APIPath *temp = new APIPath();
			temp->name = CF->getName();
			for (int parameter : BRVSet[V]) {
				temp->parameter.insert(parameter);
			}
			APList.insert(temp);
			current->next.insert(temp);
			temp->prev.insert(current);
			current = temp;
		}
		/*
		else if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(V)) {
			if (!isRelateOperation(GEP->getPointerOperand(), BRVSet, TrackedSet)) {
				continue;
			}
			APIPath *temp = new APIPath();
			temp->name = "MEMBER";
			for (int parameter : BRVSet[GEP->getPointerOperand()]) {
				temp->parameter.insert(parameter);
			}
			APList.insert(temp);
			current->next.insert(temp);
			temp->prev.insert(current);
			current = temp;
		}
		*/
		else if (isa<BranchInst>(V) || isa<SwitchInst>(V) || isa<SelectInst>(V) || isa<ReturnInst>(V)) {
			Value *Cond;
			if (BranchInst *BI = dyn_cast<BranchInst>(V)) {
				if (BI->getNumSuccessors() < 2) {
					continue;
				}
				Cond = BI->getCondition();
			}
			else if (SwitchInst *SI = dyn_cast<SwitchInst>(V)) {
				if (SI->getNumSuccessors() < 2) {
					continue;
				}
				Cond = SI->getCondition();
			}
			else if (SelectInst *SI = dyn_cast<SelectInst>(V)) {
				Cond = SI->getCondition();
			}
			else if (ReturnInst *RI = dyn_cast<ReturnInst>(V)) {
				Cond = RI->getReturnValue();
			}
			if (!isRelateOperation(Cond, BRVSet, TrackedSet)) {
				continue;
			}	
			APIPath *temp = new APIPath();
			temp->name = "CHECK";
			for (int parameter : BRVSet[Cond]) {
				temp->parameter.insert(parameter);
			}
			APList.insert(temp);
			current->next.insert(temp);
			temp->prev.insert(current);
			current = temp;
		}
	}
	current->next.insert(end);
	end->prev.insert(current);

	if (prevend) {
		begin->prev.insert(prevend);
		prevend->next.insert(begin);
	}
	
	visitBBs[BB] = make_pair(begin, end);
	succ_iterator si = succ_begin(BB), se = succ_end(BB);
	for (; si != se; ++si) {
		findRelateOperation(*si, BRVSet, AP, end, visitBBs, APList, TrackedSet);
	}
}

void DataFlowAnalysis::clearEmptyAPIPath(set<APIPath *> &APList) {
	for (APIPath *AP : APList) {
		if (AP->name.empty()) {
			for (APIPath *prev : AP->prev) {
				for (APIPath *next : AP->next) {
					prev->next.insert(next);
					next->prev.insert(prev);
				}
			}
			for (APIPath *prev : AP->prev) {
				prev->next.erase(AP);
			}	
			for (APIPath *next : AP->next) {
				next->prev.erase(AP);
			}
		}
	}
}

void DataFlowAnalysis::printForwardAPIPath(APIPath *AP, set<APIPath *> &TrackedSet) {
	if (TrackedSet.find(AP) != TrackedSet.end()) {
		return;
	}
	TrackedSet.insert(AP);
	OP << AP->name << ": { ";
	for (APIPath *prev : AP->prev) {
		OP << prev->name << " ";
	}
	OP << "}\n";
	for (APIPath *prev : AP->prev) {
		printForwardAPIPath(prev, TrackedSet);
	}
}

void DataFlowAnalysis::printBackwardAPIPath(APIPath *AP, set<APIPath *> &TrackedSet) {
	if (TrackedSet.find(AP) != TrackedSet.end()) {
		return;
	}
	TrackedSet.insert(AP);
	OP << AP->name << ": { ";
	for (APIPath *next : AP->next) {
		OP << next->name << " ";
	}
	OP << "}\n";
	for (APIPath *next : AP->next) {
		printBackwardAPIPath(next, TrackedSet);
	}
}

void DataFlowAnalysis::printAPIPath(APIPath *AP) {
	set<APIPath *> TrackedSet;
	OP << "------------------------------\n" << "printForwardAPIPath\n";
	printForwardAPIPath(AP, TrackedSet);
	TrackedSet.clear();
	OP << "------------------------------\n" << "printBackwardAPIPath\n";
	printBackwardAPIPath(AP, TrackedSet);
}

string DataFlowAnalysis::outputAPIPath(set<APIPath *> &APList) {
	string topology = "[";
	bool flag1 = false;
	for (APIPath *AP : APList) {
		if (AP->name.empty()) {
			continue;
		}
		ostringstream ossAP;
		ossAP << AP;
		topology += "{'AP': '" + AP->name + "', 'address': '" + ossAP.str() + "', 'parameter': [";
		bool flag3 = false;
		for (int parameter : AP->parameter) {
			topology += to_string(parameter) + ", ";
			flag3 = true;
		}
		if (flag3) {
			topology = topology.substr(0, topology.size() - 2);
		}
		topology += "], 'next': [";
		set<APIPath *> path;
		collectNext(AP, path);
		bool flag2 = false;
		for (APIPath *next : path) {
			ostringstream ossnext;
			ossnext << next;
			topology += "{'AP': '" + next->name + "', 'address': '" + ossnext.str() + "', 'parameter': [";
			bool flag4 = false;
			for (int parameter : next->parameter) {
				topology += to_string(parameter) + ", ";
				flag4 = true;
			}
			if (flag4) {
				topology = topology.substr(0, topology.size() - 2);
			}
			topology += "]}, ";
			flag2 = true;
		}
		if (flag2) {
			topology = topology.substr(0, topology.size() - 2);
		}
		topology += "]}, ";
		flag1 = true;
	}
	if (flag1) {
		topology = topology.substr(0, topology.size() - 2);
	}
	topology += "]";
	return topology;
}

void DataFlowAnalysis::collectNext(APIPath *AP, set<APIPath *> &path) {
	if (isTimeout()) {
		exit(EXIT_FAILURE);
	}
	for (APIPath *next : AP->next) {
		if (path.find(next) != path.end()) {
			return;
		}
		path.insert(next);
		collectNext(next, path);
	}
}

void DataFlowAnalysis::releaseAPIPath(set<APIPath *> &APList, APIPath * API) {
	for (APIPath *AP : APList) {
		if (AP != API) {
			delete(AP);
		}
	}
}

void DataFlowAnalysis::printRelateVariable(map<Value *, set<int>> &RVSet) {
	OP << "RVSet: {\n";
	for (auto RV : RVSet) {
		OP << *(RV.first) << ": ";
		for (int parameter : RV.second) {
			OP << parameter << " ";
		}
		OP << "\n";
	}
	OP << "}\n";
}

bool DataFlowAnalysis::isTimeout() {
	double instructionDuration = (double)(clock() - instructionStartTime) / CLOCKS_PER_SEC; // seconds
	double functionDuration = (double)(clock() - functionStartTime) / CLOCKS_PER_SEC; // seconds
	double fileDuration = (double)(clock() - fileStartTime) / CLOCKS_PER_SEC; // seconds
	if (instructionDuration > 60 || functionDuration > 3600 || fileDuration > 36000) { // 1 minute, 1 hour, or 10 hours
		OP << "Timeout: {" << "file: " << fileName << ", function: " << (function->getName()).str() << ", functionDuration: " << functionDuration << " seconds, fileDuration: " << fileDuration << " seconds}\n";
		ofstream outfile;
		outfile.open("tmp/" + program + "/timeout", ios::app);
		outfile << "Timeout: {" << "file: " << fileName << ", function: " << (function->getName()).str() << ", functionDuration: " << functionDuration << " seconds, fileDuration: " << fileDuration << " seconds}\n";
		outfile.close();
		return true;
	}
	return false;
}

string DataFlowAnalysis::generateAPIPath(APIPath *AP) {
	map<Value *, set<int>> FRVSet;
	map<Value *, set<int>> BRVSet;
	set<APIPath *> APList;
	map<Value *, set<int>> TrackedSet;
	map<BasicBlock *, pair<APIPath *, APIPath *> > visitBBs;
	BasicBlock *BB = &*(function->begin());
	string topology;

	APList.insert(AP);
	FRVSet[API].insert(0);
	AP->parameter.insert(0);
	TrackedSet[API].insert(0);
	for (unsigned int i = 0; i < API->getNumArgOperands(); ++i) {
		AP->parameter.insert(i + 1);
		TrackedSet[API].insert(i + 1);
		findForwardRelateVariable(API->getArgOperand(i), FRVSet, TrackedSet, i + 1);				
	}
	findBackwardRelateVariable(FRVSet, BRVSet, TrackedSet);
	findRelateOperation(BB, BRVSet, AP, NULL, visitBBs, APList, TrackedSet);

	clearEmptyAPIPath(APList);
 	topology = outputAPIPath(APList);

	releaseAPIPath(APList, AP);
	return topology;
}
