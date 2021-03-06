
#include "decafcomp-defs.h"
#include <list>
#include <stack>
#include <ostream>
#include <iostream>
#include <sstream>

#ifndef YYTOKENTYPE
#include "decafcomp.tab.h"
#endif

using namespace std;

extern llvm::Function *gen_main_def();

class descriptor;

typedef map<string, descriptor* > DecafSymbolTable;
typedef list<DecafSymbolTable* > DecafSymbolTableList;

//DecafSymbolTable GlobalCurrentST;
DecafSymbolTableList SymbolTableList;
string currentFunctionCall;

void outputCurrentSymTabList()
{
	cout << endl;
	for(list<DecafSymbolTable* >::iterator i=SymbolTableList.begin(); i!=SymbolTableList.end(); i++) {
		cout << "Symbol Table: ";
		for(map<string, descriptor* >::iterator j=(*i)->begin(); j!=(*i)->end(); j++) {
			cout << j->first << " ";
		}
		cout << endl;
	}
}

/// decafAST - Base class for all abstract syntax tree nodes.
/// TODO:Done

bool isDebugging = false;

class descriptor {
	string identifierName;
	int type;
	int lineNumber;
	llvm::Value *Alloca;

public:
	descriptor(string idName, int targetType, int lineNo, llvm::Value *allocai) {
		identifierName = idName;
		type = targetType;
		lineNumber = lineNo;
		Alloca = allocai;
	}
	~descriptor() { }
	void debug() {
		cout << "Defind variable in line " << lineNumber << " : " << identifierName << endl;
	}
	llvm::Type *getType() {

		if(type == 17) return Builder.getInt32Ty();
		if(type == 18) return Builder.getInt1Ty();
		if(type == 19) return Builder.getVoidTy();
		if(type == 20) return Builder.getInt8PtrTy();
		else return Alloca->getType();
		//return Alloca->getType();
	}
	llvm::Value *getAlloca() {
		return Alloca;
	}

	llvm::Value *getValue() {
		return Builder.CreateLoad(Alloca, identifierName);
	}

	void setAlloca(llvm::Value *allocai) {
		Alloca = allocai;
	}

	string getIDName()
	{
		return identifierName;
	}

	int getLineNum() {
		return lineNumber;
	}
};

descriptor *getSymbolTable(string idName) {

	DecafSymbolTable::iterator fetchedObject;
	for(DecafSymbolTableList::iterator i = SymbolTableList.begin(); i != SymbolTableList.end(); ++i) {
		//DecafSymbolTable::iterator fetchedObject;
		if(isDebugging) cout << "Searching table..." << endl;
		if((*i)) {
			fetchedObject = (*i)->find(idName);
			if(fetchedObject != (*i)->end()) {
				if(isDebugging) cout << "FetchedObject: Found " << idName << endl;
				return fetchedObject->second;
			} else {
				if(isDebugging) cout << "Table not found..." << idName << endl;
			}
		}
	}

	if(isDebugging) cout << "Not found: " << idName << endl;
	return NULL;
}

void checkTable(DecafSymbolTable *table) {
	for(DecafSymbolTable::iterator i = table->begin(); i != table->end(); i++) {
		(*i).second->debug();
	}
}

llvm::Type* getLLVMType(int typeIndex) {
	switch (typeIndex) {
		case 17: return Builder.getInt32Ty();
		case 18: return Builder.getInt1Ty();
		case 19: return Builder.getVoidTy();
		case 20: return Builder.getInt8PtrTy();
		default: return NULL;
	}
}

class decafAST {
public:
  virtual ~decafAST() {}
  virtual string str() { return string(""); }
	virtual llvm::Value *Codegen() = 0;
	virtual void insertSymbolIntoSymbolTable() = 0;
};

string getString(decafAST *d) {
	if (d != NULL) {
		return d->str();
	} else {
		return string("None");
	}
}

template <class T>
string commaList(list<T> vec) {
    string s("");
    for (typename list<T>::iterator i = vec.begin(); i != vec.end(); i++) { 
        s = s + (s.empty() ? string("") : string(",")) + (*i)->str(); 
    }   
    if (s.empty()) {
        s = string("None");
    }   
    return s;
}

template <class T>
llvm::Value *listCodegen(list<T> vec) {
	llvm::Value *val = NULL;
	for (typename list<T>::iterator i = vec.begin(); i != vec.end(); i++) {
		llvm::Value *j = (*i)->Codegen();
		if (j != NULL) { val = j; }
	}
	return val;
}

/// TODO:Done
/// decafStmtList - List of Decaf statements
class decafStmtList : public decafAST {
public: 
	list<decafAST *> stmts;
	//DecafSymbolTable *currentST;
//public:
	decafStmtList() {}
	~decafStmtList() {
		for (list<decafAST *>::iterator i = stmts.begin(); i != stmts.end(); i++) { 
			delete *i;
		}
	}
	int size() { return stmts.size(); }
	void push_front(decafAST *e) { stmts.push_front(e); }
	void push_back(decafAST *e) { stmts.push_back(e); }
	string pop_front() { string value = stmts.back()->str(); stmts.pop_back(); return value; }
	string str() { return commaList<class decafAST *>(stmts); }
	llvm::Value *Codegen() {
		return listCodegen<decafAST *>(stmts);
	}
	void insertSymbolIntoSymbolTable() {
		for (list<decafAST *>::iterator i = stmts.begin(); i != stmts.end(); i++) {
			(*i)->insertSymbolIntoSymbolTable();
		}
	}
};

/// TODO:Done
// Package
class PackageAST : public decafAST {
	string Name;
	decafStmtList *FieldDeclList;
	decafStmtList *MethodDeclList;
	DecafSymbolTable *currentST;
public:
	PackageAST(string name, decafStmtList *fieldlist, decafStmtList *methodlist) 
		: Name(name), FieldDeclList(fieldlist), MethodDeclList(methodlist) {}
	~PackageAST() { 
		//if (FieldDeclList != NULL) { delete FieldDeclList; }
		//if (MethodDeclList != NULL) { delete MethodDeclList; }
	}
	string str() { 
		return string("Package") + "(" + Name + "," + getString(FieldDeclList) + "," + getString(MethodDeclList) + ")";
	}
	llvm::Value *Codegen() {
		SymbolTableList.push_front(currentST);
		// Check main function
		DecafSymbolTable::iterator fetchedObject = currentST->find("main");
		if(fetchedObject == currentST->end()) {
			throw runtime_error("Syntax Error: No main function.");
		}

		if(isDebugging) cout << "Generating package..." << endl;
		//checkTable(currentST);
		llvm::Value *val = NULL;
		TheModule->setModuleIdentifier(llvm::StringRef(Name));
		if (NULL != FieldDeclList) {
			val = FieldDeclList->Codegen();
		}
		if (NULL != MethodDeclList) {
			//printf("Debug message: Generating MethodDecls...\n");
			val = MethodDeclList->Codegen();
		}
		// Q: should we enter the class name into the symbol table?
		if(isDebugging) outputCurrentSymTabList();
		SymbolTableList.pop_front();
		return val;
	}
	void insertSymbolIntoSymbolTable() {
		this->currentST = new DecafSymbolTable;
		SymbolTableList.push_front(currentST);

		FieldDeclList->insertSymbolIntoSymbolTable();
		MethodDeclList->insertSymbolIntoSymbolTable();

		SymbolTableList.pop_front();
	}
};

/// TODO: Done
// Expr
class ExprAST : public decafAST {
	decafAST *decafASTNode;
public:
	ExprAST (decafAST *node) { decafASTNode = node; }
	~ExprAST() { delete decafASTNode; }
	string str() { return getString(decafASTNode); }
	llvm::Value *Codegen() {
		return decafASTNode->Codegen();
	}
	void insertSymbolIntoSymbolTable() {
		decafASTNode->insertSymbolIntoSymbolTable();
	}
};

/// TODO:Done
/// ProgramAST - the decaf program
class ProgramAST : public decafAST {
	decafStmtList *ExternList;
	PackageAST *PackageDef;
	DecafSymbolTable *currentST;
public:
	ProgramAST(decafStmtList *externs, PackageAST *c) : ExternList(externs), PackageDef(c) {}
	~ProgramAST() { 
		if (ExternList != NULL) { delete ExternList; } 
		if (PackageDef != NULL) { delete PackageDef; }
	}
	string str() { return string("Program") + "(" + getString(ExternList) + "," + getString(PackageDef) + ")"; }
	llvm::Value *Codegen() {
		if(isDebugging) printf("Generating program...\n");
		llvm::Value *val = NULL;
		SymbolTableList.push_front(currentST);

		if (NULL != PackageDef) {
			val = PackageDef->Codegen();
		} else {
			throw runtime_error("No package definition in decaf program.");
		}
		SymbolTableList.pop_front();
		return val;
	}
	void insertSymbolIntoSymbolTable() {
		this->currentST = new DecafSymbolTable;
		SymbolTableList.push_front(this->currentST);
		if(ExternList != NULL) ExternList->insertSymbolIntoSymbolTable();
		if(PackageDef != NULL) PackageDef->insertSymbolIntoSymbolTable();
		SymbolTableList.pop_front();
	}
};


// Get Binary Operator
string getBinaryOp (int opId) {
	switch(opId) {
		case 0: return string("Plus"); break;
		case 1: return string("Minus"); break;
		case 2: return string("Mult"); break;
		case 3: return string("Div"); break;
		case 4: return string("Leftshift"); break;
		case 5: return string("Rightshift"); break;
		case 6: return string("Mod"); break;
		case 7: return string("Lt"); break;
		case 8: return string("Gt"); break;
		case 9: return string("Leq"); break;
		case 10: return string("Geq"); break;
		case 11: return string("Eq"); break;
		case 12: return string("Neq"); break;
		case 13: return string("And"); break;
		case 14: return string("Or"); break;
		default: throw runtime_error("Operator not defined.");
	}
}

llvm::Value* BinaryOpExpr(int opId, decafAST *LHS, decafAST *RHS) {
	switch(opId) {
		case 0: if(isDebugging) cout << "OpId " << opId << endl; return Builder.CreateAdd(LHS->Codegen(), RHS->Codegen(), "AddTmp");
		case 1: if(isDebugging) cout << "OpId " << opId << endl; return Builder.CreateSub(LHS->Codegen(), RHS->Codegen(), "SubTmp");
		case 2: if(isDebugging) cout << "OpId " << opId << endl; return Builder.CreateMul(LHS->Codegen(), RHS->Codegen(), "MulTmp");
		case 3: if(isDebugging) cout << "OpId " << opId << endl; return Builder.CreateSDiv(LHS->Codegen(), RHS->Codegen(), "DivTmp");
		case 4: if(isDebugging) cout << "OpId " << opId << endl; return Builder.CreateShl(LHS->Codegen(), RHS->Codegen(), "LShiftTmp");
		case 5: if(isDebugging) cout << "OpId " << opId << endl; return Builder.CreateLShr(LHS->Codegen(), RHS->Codegen(), "RShitTmp");
		case 6: if(isDebugging) cout << "OpId " << opId << endl; return Builder.CreateSRem(LHS->Codegen(), RHS->Codegen(), "RemainTmp");
		case 7: if(isDebugging) cout << "OpId " << opId << endl; return Builder.CreateICmpSLT(LHS->Codegen(), RHS->Codegen(), "LTTmp");
		case 8: if(isDebugging) cout << "OpId " << opId << endl; return Builder.CreateICmpSGT(LHS->Codegen(), RHS->Codegen(), "GTTmp");
		case 9: if(isDebugging) cout << "OpId " << opId << endl; return Builder.CreateICmpSLE(LHS->Codegen(), RHS->Codegen(), "LEQTmp");
		case 10: if(isDebugging) cout << "OpId " << opId << endl; return Builder.CreateICmpSGE(LHS->Codegen(), RHS->Codegen(), "GEQTmp");
		case 11: if(isDebugging) cout << "OpId " << opId << endl; return Builder.CreateICmpEQ(LHS->Codegen(), RHS->Codegen(), "EQTmp");
		case 12: if(isDebugging) cout << "OpId " << opId << endl; return Builder.CreateICmpNE(LHS->Codegen(), RHS->Codegen(), "NEQTmp");
		case 13: {
			llvm::Value *ConditionCode;
			llvm::Value *L = LHS->Codegen();

			if(L->getType() == getLLVMType(18)) // bool
				ConditionCode = Builder.CreateICmpEQ(L, llvm::ConstantInt::get(llvm::getGlobalContext(), llvm::APInt(1, 1)), "LHS");
			else
				throw runtime_error("Semantic error: Invalid operand on LHS.");

			if(L->getType() == getLLVMType(18)) {
				llvm::Value *ConditionCode = Builder.CreateICmpEQ(L, llvm::ConstantInt::get(llvm::getGlobalContext(), llvm::APInt(1, 1)), "LHS");

				llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();
				llvm::BasicBlock *ThenBlock = llvm::BasicBlock::Create(llvm::getGlobalContext(), "True", TheFunction);
				llvm::BasicBlock *EBlock = llvm::BasicBlock::Create(llvm::getGlobalContext(), "False");
				llvm::BasicBlock *MergeBlock = llvm::BasicBlock::Create(llvm::getGlobalContext(), "BooleanShortCircuit");
				Builder.CreateCondBr(ConditionCode, ThenBlock, EBlock);

				Builder.SetInsertPoint(ThenBlock);
				llvm::Value *R = RHS->Codegen();

				if(R->getType() != getLLVMType(18))
					throw runtime_error("Semantic error: Invalid operand on RHS.");

				Builder.CreateBr(MergeBlock);
				ThenBlock = Builder.GetInsertBlock();

				TheFunction->getBasicBlockList().push_back(EBlock);
				Builder.SetInsertPoint(EBlock);
				llvm::Value *tempFalse = Builder.getInt1(0);

				Builder.CreateBr(MergeBlock);
				EBlock = Builder.GetInsertBlock();
				TheFunction->getBasicBlockList().push_back(MergeBlock);
				Builder.SetInsertPoint(MergeBlock);

				llvm::PHINode *phinode = Builder.CreatePHI(getLLVMType(18), 2, "phival");
				phinode->addIncoming(R, ThenBlock);
				phinode->addIncoming(tempFalse, EBlock);

				return phinode;
			} else {
				throw runtime_error("Semantic error: Invalid type on boolean operands.");
			}

			if(isDebugging) cout << "OpId " << opId << endl;

		}
		case 14: {
			llvm::Value *ConditionCode;
			llvm::Value *L = LHS->Codegen();

			if (L->getType() == getLLVMType(18)) // bool
				ConditionCode = Builder.CreateICmpEQ(L, llvm::ConstantInt::get(llvm::getGlobalContext(),
																			   llvm::APInt(1, 1)), "LHS");
			else
				throw runtime_error("Semantic error: Invalid operand on LHS.");

			if (L->getType() == getLLVMType(18)) {
				llvm::Value *ConditionCode = Builder.CreateICmpEQ(L, llvm::ConstantInt::get(llvm::getGlobalContext(),
																							llvm::APInt(1, 1)), "LHS");

				llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();
				llvm::BasicBlock *ThenBlock = llvm::BasicBlock::Create(llvm::getGlobalContext(), "True", TheFunction);
				llvm::BasicBlock *EBlock = llvm::BasicBlock::Create(llvm::getGlobalContext(), "False");
				llvm::BasicBlock *MergeBlock = llvm::BasicBlock::Create(llvm::getGlobalContext(),
																		"BooleanShortCircuit");
				Builder.CreateCondBr(ConditionCode, ThenBlock, EBlock);

				Builder.SetInsertPoint(ThenBlock);
				llvm::Value *tempTrue = Builder.getInt1(1);

				Builder.CreateBr(MergeBlock);
				ThenBlock = Builder.GetInsertBlock();

				TheFunction->getBasicBlockList().push_back(EBlock);
				Builder.SetInsertPoint(EBlock);

				llvm::Value *R = RHS->Codegen();
				if (R->getType() != getLLVMType(18))
					throw runtime_error("Semantic error: Invalid operand on RHS.");

				Builder.CreateBr(MergeBlock);
				EBlock = Builder.GetInsertBlock();
				TheFunction->getBasicBlockList().push_back(MergeBlock);
				Builder.SetInsertPoint(MergeBlock);

				llvm::PHINode *phinode = Builder.CreatePHI(getLLVMType(18), 2, "phival");
				phinode->addIncoming(tempTrue, ThenBlock);
				phinode->addIncoming(R, EBlock);

				return phinode;
			}
		}
		default: if(isDebugging) cout << "OpId " << opId << endl; return NULL;
	}
}


// Get Unary Operator
string getUnaryOp (int opId) {
	switch(opId) {
		case 15: return string("UnaryMinus"); break;
		case 16: return string("Not"); break;
		default: return string(""); break;
	}
}

llvm::Value* UnaryOpExpr(int opId, llvm::Value *L) {
	switch(opId) {
		case 15: return Builder.CreateNeg(L, "NegTmp");
		case 16: return Builder.CreateNot(L, "NotTmp");
		default: return NULL;
	}
}

// DecafType
string getDecafType (int typeIndex) {
	switch (typeIndex) {
		case 17: return string("IntType"); break;
		case 18: return string("BoolType"); break;
		default: return string(""); break;
	}
}

// Method Type
string getMethodType (int typeIndex) {
	switch (typeIndex) {
		case 17: return getDecafType(typeIndex); break;
		case 18: return getDecafType(typeIndex); break;
		case 19: return string("VoidType"); break;
		default: return string(""); break;
	}
}



/// TODO: Done
class BoolConstantAST : public decafAST {
	bool value;
public:
	BoolConstantAST(bool type) { value = type; };
	~BoolConstantAST() { }
	string str() { if(value) return string("BoolExpr(True)"); else return string("BoolExpr(False)"); }
	llvm::Value *Codegen() {
		if(value) return llvm::ConstantInt::get(llvm::getGlobalContext(), llvm::APInt(1, 1));
		else return llvm::ConstantInt::get(llvm::getGlobalContext(), llvm::APInt(1, 0));
	}
	void insertSymbolIntoSymbolTable() {

	}
};

/// TODO: Not Done
class rvalueAST : public decafAST {
	string idName;
	bool isArray;
	decafAST *arrayIndex;
public:
	rvalueAST(string ID, bool arraytype = false) { idName = ID; isArray = arraytype; arrayIndex = NULL; }
	rvalueAST(string ID, bool arraytype, decafAST *index) { idName = ID; isArray = arraytype; arrayIndex = index;}
	~rvalueAST() { }
	string str() {
		if(!isArray) {
			return string("VariableExpr") + "(" + idName + ")";
		} else {
			return string("ArrayLocExpr") + "(" + idName + "," + getString(arrayIndex) + ")";
		}
	}
	llvm::Value *Codegen() {
		descriptor *fetchedVar = getSymbolTable(idName);
		if(fetchedVar==NULL) {
			throw runtime_error("Undefined Variable: " + idName + ".");
		}
		else {
			if(!isArray) {
				// Not Array
				return fetchedVar->getValue();
			} else {
				// Array
				llvm::GlobalVariable *fetchedArray = (llvm::GlobalVariable *)fetchedVar->getAlloca();
				llvm::ArrayType *arraytype = (llvm::ArrayType *)fetchedArray->getValueType();
				llvm::Value *arrayLoc = Builder.CreateStructGEP(arraytype, fetchedArray, 0, "arrayloc");
				llvm::Value *ArrayIndex = arrayIndex->Codegen();
				llvm::Value *ArrayIndexPar = Builder.CreateGEP(Builder.getInt32Ty(), arrayLoc, ArrayIndex, "arrayindex");
				llvm::Value *loadTmp = Builder.CreateLoad(ArrayIndexPar, "loadtmp");

				return loadTmp;
			};
		}
	}
	void insertSymbolIntoSymbolTable() {

	}
};

/// TODO: Done
class NumberExprAST : public decafAST {
	int integerValue;
public:
	NumberExprAST(int value) { integerValue = value; }
	~NumberExprAST();
	string str() {
		return string("NumberExpr") + "(" + std::to_string(integerValue) + ")";
	}
	llvm::Value *Codegen() {
		return llvm::ConstantInt::get(llvm::getGlobalContext(), llvm::APInt(32, integerValue));
	}
	void insertSymbolIntoSymbolTable() {

	}
};

/// TODO: Done
class BoolExprAST : public decafAST {
	decafAST *boolASTNode;
public:
	BoolExprAST(decafAST *boolConstant) { boolASTNode = boolConstant; }
	~BoolExprAST() { delete boolASTNode; }
	string str() {
		return string("BoolExpr(") + getString(boolASTNode) + ")";
	}
	llvm::Value *Codegen() {
		return boolASTNode->Codegen();
	}
	void insertSymbolIntoSymbolTable() {

	}
};

/// TODO: Done
class BinaryExprAST : public decafAST {
	int binaryOperator;
	decafAST *leftValueNode;
	decafAST *rightValueNode;
public:
	BinaryExprAST(int binaryOp, decafAST *left, decafAST *right) {
		binaryOperator = binaryOp;
		leftValueNode = left;
		rightValueNode = right;
	}
	~BinaryExprAST() {
		delete leftValueNode;
		delete rightValueNode;
	}
	string str() {
		return string("BinaryExpr(") + getBinaryOp(binaryOperator) + "," + getString(leftValueNode) + "," + getString(rightValueNode) + ")";
	}
	llvm::Value *Codegen() {
		//llvm::Value *L = leftValueNode->Codegen();
		//llvm::Value *R = rightValueNode->Codegen();
		if(leftValueNode == NULL || rightValueNode == NULL) {
			throw runtime_error("Semantic error: Missing operands in binary expressions.");
		}

		return BinaryOpExpr(binaryOperator, leftValueNode, rightValueNode);
	}
	void insertSymbolIntoSymbolTable() {

	}
};

/// TODO: Done
class UnaryExprAST : public decafAST {
	int unaryOperator;
	decafAST *opNumber;
public:
	UnaryExprAST(int unaryOp, decafAST *exp) {
		unaryOperator = unaryOp;
		opNumber = exp;
	}
	~UnaryExprAST() {
		delete opNumber;
	}
	string str() {
		return string("UnaryExpr(") + getUnaryOp(unaryOperator) + "," + getString(opNumber) + ")";
	}
	llvm::Value *Codegen() {
		llvm::Value *opNum = opNumber->Codegen();
		if(opNum == NULL) return NULL;
		return UnaryOpExpr(unaryOperator, opNum);
	}
	void insertSymbolIntoSymbolTable() {

	}
};

// Method_Call
/// TODO: Not Done
class MethodCallAST : public decafAST {
	string identifierName;
	decafStmtList *argumentList;
public:
	MethodCallAST(string idName, decafStmtList *args) { identifierName = idName; argumentList = args; }
	~MethodCallAST() { identifierName = ""; delete argumentList; }
	string str() { return string("MethodCall(") + identifierName + "," + argumentList->str() + ")"; }
	llvm::Value *Codegen() {
		if(isDebugging) {
			cout << "Debug message: Generating method call... Calling " << identifierName << endl;
		}

		llvm::Function *TheFunction = TheModule->getFunction(identifierName);
		//return NULL;
		if(TheFunction==NULL) {
			throw runtime_error("Undefined function is called.");
			// return NULL;
		} else {
			if(isDebugging) cout << "ArgumentListSize = " << argumentList->stmts.size() << endl;
			if(argumentList->stmts.size() != TheFunction->arg_size())
				throw runtime_error("Incorrect # arguments passed.");

			std::vector<llvm::Value *> args;
			llvm::Function::arg_iterator j = TheFunction->arg_begin();

			for (list<decafAST *>::iterator i = argumentList->stmts.begin();
				i != argumentList->stmts.end(); ++i, ++j) {

				if(isDebugging) cout << "Argument Type: ";
				llvm::Value *tempCodeGen = (*i)->Codegen();
				if(isDebugging) cout << "(finishing codegen for this argument) ";
				if(isDebugging) {
					if(tempCodeGen->getType()==getLLVMType(17)) cout<< "int type" <<endl;//int
					else if(tempCodeGen->getType()==getLLVMType(18)) cout<< "bool type" <<endl;//bool
					else if(tempCodeGen->getType()==getLLVMType(19)) cout<< "void type" <<endl;//void
					else if(tempCodeGen->getType()==getLLVMType(20)) cout<< "string type" <<endl;//string
				}
				if(isDebugging) {
					if((*j).getType()==getLLVMType(17)) cout<< "Func Arg: int type" <<endl;//int
					else if((*j).getType()==getLLVMType(18)) cout<< "Func Arg: bool type" <<endl;//bool
					else if((*j).getType()==getLLVMType(19)) cout<< "Func Arg: void type" <<endl;//void
					else if((*j).getType()==getLLVMType(20)) cout<< "Func Arg: string type" <<endl;//string
				}

				// Matching
				if((*j).getType()==getLLVMType(17) && tempCodeGen->getType()==getLLVMType(18)) {
					tempCodeGen= Builder.CreateZExt(tempCodeGen, Builder.getInt32Ty(), "zexttmp");
					if(isDebugging) cout << "Promoting a bool" << endl;
				}
				else if( (*j).getType() != tempCodeGen->getType() )
				{
					if(isDebugging) cout << tempCodeGen << tempCodeGen->getType() << endl;
					throw runtime_error("Unmatched Argument Passed.");
				}
					
				if(tempCodeGen==NULL) {
					return NULL;
				}
				else args.push_back(tempCodeGen);
				//cout << "Matched Sequence: " << (*i)->str() << endl;

			}


			bool isVoid = TheFunction->getReturnType()->isVoidTy();
			llvm::Value *val = Builder.CreateCall(
			    TheFunction,
			    args,
			    isVoid ? "" : "calltmp"
			);
			//cout << "here#" << argumentList->stmts.size() << endl;
			if(isDebugging) cout << "Method Call done..." << endl;
			return val;
		}
	}
	void insertSymbolIntoSymbolTable() {

	}
};

string getExternType (int typeIndex) {
	if(typeIndex == 17 || typeIndex == 18 || typeIndex == 19) {
		return getMethodType(typeIndex);
	} else {
		return string("StringType");
	}
}

// Method Argument
/// TODO: Not Done
class MethodArgumentAST : public decafAST {
	decafAST *decafASTNode;
public:
	MethodArgumentAST(decafAST *node) {
		decafASTNode = node;
	}
	~MethodArgumentAST() { delete decafASTNode; }
	string str() { return getString(decafASTNode); }
	llvm::Value *Codegen() {
		return decafASTNode->Codegen();
	}
	void insertSymbolIntoSymbolTable() {

	}
};

// String
/// TODO: Done
class StringAST : public decafAST {
	string decafASTString;
public:
	StringAST(string value) { decafASTString = value; }
	~StringAST() { }
	string str() { return string("StringConstant") + "(" + decafASTString + ")"; }
	//string rawStr() { return decafASTString; }
	llvm::Value *Codegen() {
		string str;
		for(string::iterator i=decafASTString.begin(); i!=decafASTString.end(); ) {
			if( (*i) == '"') {
				i++;
				continue;
			}
			else if( (*i) == '\\' ) {
				if( (*(i+1)) == 'a' ) str+='\a';
				else if( (*(i+1)) == 'b' ) str+='\b';
				else if( (*(i+1)) == 't' ) str+='\t';
				else if( (*(i+1)) == 'n' ) str+='\n';
				else if( (*(i+1)) == 'v' ) str+='\v';
				else if( (*(i+1)) == 'f' ) str+='\f';
				else if( (*(i+1)) == 'r' ) str+='\r';
				else if( (*(i+1)) == '\\' ) str+='\\';
				else if( (*(i+1)) == '\'' ) str+='\'';
				else if( (*(i+1)) == '\"' ) str+='\"';
				i += 2;
			}
			else {
				str+=(*i);
				i++;
			}
		}
		if(isDebugging) cout << "Generating String..." << str << endl;
		llvm::GlobalVariable *GlobalString = Builder.CreateGlobalString(str, "globalstring");
		llvm::Value *stringConst = Builder.CreateConstGEP2_32(GlobalString->getValueType(), GlobalString, 0, 0, "cast");
		return stringConst;
	}
	void insertSymbolIntoSymbolTable() {

	}
};

/// TODO: Done
/// Arguments in a method
class IDTypeStringAST : public decafAST {
	string decafASTString;
	int methodType;
	int linepos;
public:
	IDTypeStringAST(string value, int typeId, int pos) { decafASTString = value; methodType = typeId; linepos = pos; }
	~IDTypeStringAST() { }
	string str() { return decafASTString + "," + getMethodType(methodType); }
	string getName() { return decafASTString; }
	int getType() { return methodType; }
	//string rawStr() { return decafASTString; }
	llvm::Value *Codegen() {
		if(isDebugging) cout << "Allocating " << decafASTString << endl;
		descriptor *fetchedVar=getSymbolTable(decafASTString);
		llvm::AllocaInst *Alloca = Builder.CreateAlloca(getLLVMType(methodType), nullptr, decafASTString);
		fetchedVar->setAlloca(Alloca);
		return NULL;
	}
	void insertSymbolIntoSymbolTable() {
		//llvm::AllocaInst *Alloca = Builder.CreateAlloca(getLLVMType(methodType), nullptr, decafASTString);
		descriptor *newDecp = new descriptor(decafASTString, methodType, linepos, NULL);
		SymbolTableList.front()->insert(std::pair<string, descriptor* >(decafASTString, newDecp));
	}

	string getIDName() {
		return decafASTString;
	}
};

/// TODO: Done
class IDTypeStringSpecialAST : public decafAST {
	IDTypeStringAST *strs;
public:
	IDTypeStringSpecialAST(IDTypeStringAST *node) { strs = node; }
	~IDTypeStringSpecialAST() { delete strs; }
	string str() { return string("VarDef(") + getString(strs) + ")"; }
	string getName() { return strs->getName(); }
	int getTypeId() { return strs->getType(); }
	llvm::Value *Codegen() {
		if(isDebugging) cout << "In IDTypeStringSpecialAST" << endl;
		return strs->Codegen();
	}
	void insertSymbolIntoSymbolTable() {
		strs->insertSymbolIntoSymbolTable();
	}

	string getIDName() {
		return strs->getIDName();
	}
};

/// TODO: Done
class RawStringAST : public decafAST {
	string decafASTString;
public:
	RawStringAST(string value) { decafASTString = value; }
	~RawStringAST() { }
	string str() { return decafASTString; }
	//string rawStr() { return decafASTString; }
	llvm::Value *Codegen() {
		llvm::GlobalVariable *GlobalString = Builder.CreateGlobalString(decafASTString, "globalstring");
		llvm::Value *stringConst = Builder.CreateConstGEP2_32(GlobalString->getValueType(), GlobalString, 0, 0, "cast");
		return stringConst;
	}
	void insertSymbolIntoSymbolTable() {

	}
};

// Assign
/// TODO: Not Done
class AssignAST : public decafAST {
	string identifierName;
	decafAST *value;
	decafAST *index;
	bool isArray;
public:
	AssignAST(string idName, decafAST *assignedValue) { identifierName = idName; value = assignedValue; index = NULL; isArray = false; }
	AssignAST(string idName, decafAST *dstIndex, decafAST *assignedValue) { identifierName = idName; index = dstIndex; value = assignedValue; isArray = true; }
	~AssignAST() { delete value; if(isArray) delete index;}
	string str() {
		if(isArray) {
			return string("AssignArrayLoc(") + identifierName + "," + getString(index) + "," + getString(value) + ")";
		} else {
			return string("AssignVar(") + identifierName + "," + getString(value) + ")";
		}
	}
	llvm::Value *Codegen() {
		if(isDebugging) cout << "Codegen for AssignAST... "<< endl;
		if(!isArray) {
			if(isDebugging) cout << "Now Assigning " << identifierName << "..."<< endl;
			descriptor *fetchedVar = getSymbolTable(identifierName);
			if(fetchedVar) {
				if(isDebugging) cout << "Fetched Variable " << identifierName << " on line " << fetchedVar->getLineNum() << endl;
				llvm::Value *rvalue = value->Codegen();
				llvm::Value *alloca = fetchedVar->getAlloca();
				llvm::PointerType *ptrTy = rvalue->getType()->getPointerTo();
				if(ptrTy == alloca->getType())
					return Builder.CreateStore(rvalue, alloca);
				else
					throw runtime_error("Semantic error: Assign wrong type value to a variable.");
			} else {
				if(isDebugging) cout << "Undefined variable " + identifierName << "..." << endl;
				throw runtime_error("Semantic error: Undefined variable:" + identifierName + ".");
				return NULL;
			}
		} else {
			// Array Assign
			descriptor *fetchedVar = getSymbolTable(identifierName);
			if(fetchedVar) {

				llvm::GlobalVariable *fetchedArray = (llvm::GlobalVariable *)fetchedVar->getAlloca();
				llvm::ArrayType *arraytype = (llvm::ArrayType *)fetchedArray->getValueType();
				llvm::Value *rvalue = value->Codegen();

				llvm::Value *arrayLoc = Builder.CreateStructGEP(arraytype, fetchedArray, 0, "arrayloc");
				llvm::Value *arrayIndex = index->Codegen();
				llvm::Value *ArrayIndexPar = Builder.CreateGEP(Builder.getInt32Ty(), arrayLoc, arrayIndex, "arrayindex");
				llvm::Value *arrayStore = Builder.CreateStore(rvalue, ArrayIndexPar);

				return arrayStore;
			} else {
				throw runtime_error("Undefined variable: " + identifierName + ".");
				return NULL;
			}
		}
	}
	void insertSymbolIntoSymbolTable() {

	}
};

// Extern Type
/// TODO: Done
class ExternType : public decafAST {
public:
	int externType;
//public:
	ExternType(int typeIndex) { externType = typeIndex; }
	~ExternType() { }
	string str() {
		return string("VarDef(") + getExternType(externType) + ")";
	}
	llvm::Value *Codegen() {
		//return getLLVMType(externType);
		return NULL;
	}
	void insertSymbolIntoSymbolTable() {

	}
};

// Extern
/// TODO: Not Done
class ExternAST : public decafAST {
	string identifierName;
	int methodTypeId;
	decafStmtList *typeList;
	int linepos;
public:
	ExternAST(string IdName, int typeId, decafStmtList *list, int pos) {
		identifierName = IdName;
		methodTypeId = typeId;
		typeList = list;
		linepos = pos;
	}
	~ExternAST() {
		identifierName = "";
		delete typeList;
	}
	string str() {
		return string("ExternFunction(") + identifierName + "," + getMethodType(methodTypeId) + "," + typeList->str() + ")";
	}
	llvm::Value *Codegen() {

		return NULL;
	}
	void insertSymbolIntoSymbolTable() {

		std::vector<llvm::Type*> args;
		if(isDebugging) printf("Debug message: Generating extern function...\nArg Type: ");
		for (list<decafAST *>::iterator i = typeList->stmts.begin(); i != typeList->stmts.end(); i++) {
			//printf("%d\n", ((ExternType *)(*i))->externType );
			if(isDebugging) printf("%d ", ((ExternType *) (*i))->externType);
			args.push_back(getLLVMType(((ExternType *) (*i))->externType));
		}
		if(isDebugging) printf("\n");
		llvm::Type *returnTy=getLLVMType(methodTypeId);
		llvm::Function *func = llvm::Function::Create(
		    llvm::FunctionType::get(returnTy, args, false),
		    llvm::Function::ExternalLinkage,
		    identifierName,
		    TheModule
		);

		descriptor *newDescp = new descriptor(identifierName, methodTypeId, linepos, (llvm::Value *)func);
		SymbolTableList.front()->insert(std::pair<string, descriptor* >(identifierName, newDescp));
	}
};

// field size
/// TODO: Not Done
class FieldSizeAST : public decafAST {
	int size;
	bool isArray;
public:
	FieldSizeAST(int sizeValue, bool array) { if(array) { size = sizeValue; isArray = array; } else { size = -1; isArray = array; } }
	~FieldSizeAST() { }
	string str() {
		if(isArray) {
			return string("Array(") + std::to_string(size) + ")";
		} else {
			return string("Scalar");
		}
	}
	llvm::Value *Codegen() {
		return llvm::ConstantInt::get(llvm::getGlobalContext(), llvm::APInt(32, size));
	}

	int getValue() {
		return size;
	}

	void insertSymbolIntoSymbolTable() {

	}
};

// Field_decl
/// TODO: Not Done
class FieldDeclAST : public decafAST {
	string identifierName;
	int decafTypeId;
	bool hasAssign;
	decafAST *expr;
	int linepos;
	bool isArray;
public:
	FieldDeclAST(string idName, int typeId, decafAST *exprNode, bool global, int pos, bool array) { identifierName = idName; decafTypeId = typeId; hasAssign = global; expr = exprNode; linepos = pos; isArray = array; }
	~FieldDeclAST() { }
	string str() {
		if(hasAssign)
			return string("AssignGlobalVar(") + identifierName + "," + getDecafType(decafTypeId) + "," + getString(expr) + ")";
		else
			if (decafTypeId != -1) return string("FieldDecl(") + identifierName + "," + getDecafType(decafTypeId) + "," + getString(expr) + ")";
			else return string("FieldDecl(") + identifierName + "," + getString(expr) + ")";
	}
	llvm::Value *Codegen() {
		if(isArray) {
			descriptor *fetchedVar=getSymbolTable(identifierName);
			if(!fetchedVar) throw runtime_error("Symbol table error. Undefined variable.");

			llvm::ArrayType *theArray;
			FieldSizeAST *temp = (FieldSizeAST *)expr;

			if(decafTypeId == 17) { // Int
				// Definition
				theArray = llvm::ArrayType::get(Builder.getInt32Ty(), temp->getValue());
			} else if(decafTypeId == 18) { // Bool
				theArray = llvm::ArrayType::get(Builder.getInt1Ty(), temp->getValue());
			} else {
				throw runtime_error("Semantic error: Invalid type for Array. Only int and bool is supported.");
			}
			// Zero Init
			llvm::Constant *zero = llvm::Constant::getNullValue(theArray);
			// Declaration
			llvm::GlobalVariable *globalArray = new llvm::GlobalVariable(*TheModule, theArray, false, llvm::GlobalValue::ExternalLinkage, zero, identifierName);

			fetchedVar->setAlloca(globalArray);

		} else {
			descriptor *fetchedVar=getSymbolTable(identifierName);
			//llvm::AllocaInst *Alloca = Builder.CreateAlloca(getLLVMType(decafTypeId), nullptr, identifierName);
			//fetchedVar->setAlloca(Alloca);

			llvm::Constant *ValueVar;

			if(hasAssign) {
				//Builder.CreateStore(expr->Codegen(), Alloca);
				ValueVar = (llvm::Constant *)expr->Codegen();
			} else {
				llvm::Value *zero;
				if(decafTypeId == 17) ValueVar = llvm::ConstantInt::get(llvm::getGlobalContext(), llvm::APInt(32, 0));
				else if(decafTypeId == 18) ValueVar = llvm::ConstantInt::get(llvm::getGlobalContext(), llvm::APInt(1, 0));
				//Builder.CreateStore(zero, Alloca);
			}

			llvm::GlobalVariable *globalVar = new llvm::GlobalVariable(*TheModule, getLLVMType(decafTypeId), false, llvm::GlobalValue::ExternalLinkage, ValueVar, identifierName);
			fetchedVar->setAlloca(globalVar);
			//return NULL;
		}
		return NULL;
	}
	void insertSymbolIntoSymbolTable() {
		descriptor *newDescp = new descriptor(identifierName, decafTypeId, linepos, NULL);
		SymbolTableList.front()->insert(std::pair<string, descriptor* >(identifierName, newDescp));
	}
};

// typed symbol
/// TODO: Not Done
class TypedSymbol : public decafAST {
	string identifierName;
	int decafTypeId;
	int linepos;
public:
	TypedSymbol(string idName, int typeId, int pos) {
		identifierName = idName;
		decafTypeId = typeId;
		linepos = pos;
	}
	~TypedSymbol() { }
	string str() {
		return string("VarDef(") + identifierName + "," + getDecafType(decafTypeId) + ")";
	}
	llvm::Value *Codegen() {
		descriptor *fetchedVar = getSymbolTable(identifierName);
		llvm::AllocaInst *Alloca = Builder.CreateAlloca(getLLVMType(decafTypeId), nullptr, identifierName);
		llvm::Value *zero;
		if(decafTypeId == 17) zero = llvm::ConstantInt::get(llvm::getGlobalContext(), llvm::APInt(32, 0));
		else if(decafTypeId == 18) zero = llvm::ConstantInt::get(llvm::getGlobalContext(), llvm::APInt(1, 0));
		Builder.CreateStore(zero, Alloca);
		if(isDebugging) cout << "Allocating " << identifierName << endl;
		fetchedVar->setAlloca(Alloca);

		return NULL;
	}

	void insertSymbolIntoSymbolTable() {
		descriptor *newDescp = new descriptor(identifierName, decafTypeId, linepos, NULL);
		SymbolTableList.front()->insert(std::pair<string, descriptor* >(identifierName, newDescp));
	}
};

/// TODO: Not Done
class MethodBlockAST : public decafAST {
	decafAST *block;
public:
	MethodBlockAST(decafAST *node) { block = node; }
	~MethodBlockAST() { delete block; }
	string str() {
		return string("Method") + getString(block);
	}
	llvm::Value *Codegen() {
		if(block != NULL) return block->Codegen();
		else return NULL;
	}
	void insertSymbolIntoSymbolTable() {
		if (block != NULL)
			block->insertSymbolIntoSymbolTable();
	}
};


// Block
/// TODO: Not Done
class BlockAST : public decafAST {
	decafStmtList *varDeclList;
	decafStmtList *statementList;
	DecafSymbolTable *currentST;
	bool isMethod;
public:
	BlockAST(decafStmtList *varDecl, decafStmtList *stmt) {
		varDeclList = varDecl;
		statementList = stmt;
		currentST = NULL;
		isMethod = false;
	}
	~BlockAST() { delete varDeclList; delete statementList; };
	void turnToMethod() { isMethod = true; }
	string str() {
		if(!isMethod) return string("Block(") + varDeclList->str() + "," + statementList->str() + ")";
		else return string("MethodBlock(") + varDeclList->str() + "," + statementList->str() + ")";
	}
	llvm::Value *Codegen() {

		if(isDebugging) cout << "Block CodeGen......" << endl;
		llvm::Value *val = NULL;

		if(!isMethod) {
			SymbolTableList.push_front(currentST);
			//if(!isMethod) checkTable(currentST);
			if(isDebugging) cout << "Block CodeGen for VarDeclList......" << endl;
			if(varDeclList != NULL) val = varDeclList->Codegen();
			if(isDebugging) cout << "Block CodeGen for StmtList......" << endl;
			if(statementList != NULL) val = statementList->Codegen();
			SymbolTableList.pop_front();
		} else {
			if(varDeclList != NULL) val = varDeclList->Codegen();
			if(statementList != NULL) val = statementList->Codegen();
		}

		if(isDebugging) cout << "Finished Block CodeGen......" << endl;

		return val;
	}

	void insertSymbolIntoSymbolTable() {
		if(isDebugging) printf("Inserting block into symbol table...\n");
		if(!isMethod) {
			this->currentST = new DecafSymbolTable;
			SymbolTableList.push_front(this->currentST);

			if(varDeclList != NULL) varDeclList->insertSymbolIntoSymbolTable();
			if(statementList != NULL) statementList->insertSymbolIntoSymbolTable();

			SymbolTableList.pop_front();
		} else {
			varDeclList->insertSymbolIntoSymbolTable();
			statementList->insertSymbolIntoSymbolTable();
		}
	}
};

// Break, Continue
/// TODO: Not Done - Not Needed
class SimpleStatement : public decafAST {
	int typeId;
public:
	SimpleStatement(int type) {
		typeId = type;
	}
	~SimpleStatement() { }
	string str() {
		if(typeId == 20) return string("BreakStmt");
		else if(typeId == 21) return string("ContinueStmt");
		else return string("");
	}
	llvm::Value *Codegen() {
		//llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();
		if(typeId == 20) {
			// Break
			descriptor *temp = getSymbolTable("while");
			if(temp == NULL) {
				throw runtime_error("Semantic error: Break must be in loops.");
			}
			llvm::BasicBlock *breakblock = (llvm::BasicBlock *)temp->getAlloca();
			Builder.CreateBr(breakblock);
		} else if(typeId == 21) {
			// Continue
			descriptor *temp = getSymbolTable("for");
			if(temp == NULL) {
				throw runtime_error("Semantic error: Continue must be in loops.");
			}
			llvm::BasicBlock *forBlock = (llvm::BasicBlock *)temp->getAlloca();
			Builder.CreateBr(forBlock);
		} else {
			throw runtime_error("Semantic error: unknown reason.");
		}
	}
	void insertSymbolIntoSymbolTable() {

	}
};

/// TODO: Done - Not Needed
class IfStmtAST : public decafAST {
	decafAST *condition;
	decafAST *ifBlock;
	decafAST *ElseBlock;
public:
	IfStmtAST(decafAST *cond, decafAST *ifBlockStmt, decafAST *elseBlockStmt) {
		condition = cond;
		ifBlock = ifBlockStmt;
		ElseBlock = elseBlockStmt;
	}
	~IfStmtAST() {
		delete ifBlock;
		delete ElseBlock;
	}
	string str() {
		return string("IfStmt(") + getString(condition) + "," + getString(ifBlock) + "," + getString(ElseBlock) + ")";
	}
	llvm::Value *Codegen() {
		// If no condition code, return null
		llvm::Value *ConditionCode = condition->Codegen();
		if(!ConditionCode)
			return NULL;

		// Check the type of the condition code and do different jobs
		if(ConditionCode->getType() == getLLVMType(17)) // Int
			ConditionCode = Builder.CreateICmpSGT(ConditionCode, llvm::ConstantInt::get(llvm::getGlobalContext(), llvm::APInt(32, 0)), "IfCond");
		else if(ConditionCode->getType() == getLLVMType(18)) // bool
			ConditionCode = Builder.CreateICmpEQ(ConditionCode, llvm::ConstantInt::get(llvm::getGlobalContext(), llvm::APInt(1, 1)), "IfCond");
		else
			throw runtime_error("Semantic error: Condition of If must be a boolean type.");

		// Create blocks for Then and Else
		llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();
		llvm::BasicBlock *ThenBlock = llvm::BasicBlock::Create(llvm::getGlobalContext(), "Then", TheFunction);
		llvm::BasicBlock *EBlock = llvm::BasicBlock::Create(llvm::getGlobalContext(), "Else");
		llvm::BasicBlock *MergeBlock = llvm::BasicBlock::Create(llvm::getGlobalContext(), "IfCont");
		Builder.CreateCondBr(ConditionCode, ThenBlock, EBlock);

		Builder.SetInsertPoint(ThenBlock);

		if(isDebugging) cout << "Preparing to do code generation for If block..." << endl;

		llvm::Value *ThenCode = ifBlock->Codegen();
		Builder.CreateBr(MergeBlock);
		ThenBlock = Builder.GetInsertBlock();

		if(isDebugging) cout << "Finished code generation for If block..." << endl;

		if(false) {
			if(isDebugging) cout << "Else block is NULL..." << endl;
		} else {
			if(isDebugging) cout << "Preparing to do code generation for Else block..." << endl;

			TheFunction->getBasicBlockList().push_back(EBlock);
			Builder.SetInsertPoint(EBlock);
			llvm::Value *ElseCode = NULL;
			if(ElseBlock) ElseCode = ElseBlock->Codegen();

			Builder.CreateBr(MergeBlock);
			EBlock = Builder.GetInsertBlock();
			TheFunction->getBasicBlockList().push_back(MergeBlock);
			Builder.SetInsertPoint(MergeBlock);
		}
		return MergeBlock;
	}

	void insertSymbolIntoSymbolTable() {
		if(ifBlock != NULL)
			ifBlock->insertSymbolIntoSymbolTable();
		if(ElseBlock != NULL)
			ElseBlock->insertSymbolIntoSymbolTable();
	}
};

/// TODO: Not Done - Not Needed
class WhileStmt : public decafAST {
	decafAST *condition;
	decafAST *whileBlock;
public:
	WhileStmt(decafAST *cond, decafAST *whileStmt) {
		condition = cond;
		whileBlock = whileStmt;
	}
	~WhileStmt() {
		delete condition;
		delete whileBlock;
	}
	string str() {
		return string("WhileStmt(") + getString(condition) + "," + getString(whileBlock) + ")";
	}
	llvm::Value *Codegen() {

		llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();
		llvm::BasicBlock *WhileCondBlock = llvm::BasicBlock::Create(llvm::getGlobalContext(), "WhileCond", TheFunction);
		llvm::BasicBlock *WhileExecBlock = llvm::BasicBlock::Create(llvm::getGlobalContext(), "While");
		llvm::BasicBlock *FinishBlock = llvm::BasicBlock::Create(llvm::getGlobalContext(), "WhileEnd");

		descriptor *tempBlock = new descriptor("while", -1, -1, FinishBlock);
		SymbolTableList.front()->insert(std::pair<string, descriptor* >("while", tempBlock));

		Builder.CreateBr(WhileCondBlock);
		Builder.SetInsertPoint(WhileCondBlock);

		llvm::Value *ConditionCode = condition->Codegen();

		if(ConditionCode->getType() == getLLVMType(17)) // Int 
			 ConditionCode = Builder.CreateICmpSGT(ConditionCode, llvm::ConstantInt::get(llvm::getGlobalContext(), llvm::APInt(32, 0)), "IfCond"); 
		else if(ConditionCode->getType() == getLLVMType(18)) // bool 
			 ConditionCode = Builder.CreateICmpEQ(ConditionCode, llvm::ConstantInt::get(llvm::getGlobalContext(), llvm::APInt(1, 1)), "IfCond");
		else
			throw runtime_error("Semantic error: Condition of While must be a boolean type.");

		Builder.CreateCondBr(ConditionCode, WhileExecBlock, FinishBlock);

		TheFunction->getBasicBlockList().push_back(WhileExecBlock);
		Builder.SetInsertPoint(WhileExecBlock);
		llvm::Value *WhileCode = whileBlock->Codegen();
		Builder.CreateBr(WhileCondBlock);

		TheFunction->getBasicBlockList().push_back(FinishBlock);
		Builder.SetInsertPoint(FinishBlock);

		SymbolTableList.front()->erase("while");
		return FinishBlock;
	}
	void insertSymbolIntoSymbolTable() {
		if(whileBlock != NULL)
			whileBlock->insertSymbolIntoSymbolTable();
	}
};

/// TODO: Not Done - Not Needed
class ForStmtAST : public decafAST {
	decafStmtList *preAssignList;
	decafStmtList *loopAssignList;
	decafAST *condition;
	decafAST *Block;
public:
	ForStmtAST(decafStmtList *pre, decafAST *cond, decafStmtList *loop, decafAST *blockNode) {
		preAssignList = pre;
		condition = cond;
		loopAssignList = loop;
		Block = blockNode;
	}
	~ForStmtAST() {
		delete preAssignList;
		delete loopAssignList;
		delete condition;
		delete Block;
	}
	string str() {
		return string("ForStmt(") + preAssignList->str() + "," + getString(condition) + "," +loopAssignList->str() + "," + getString(Block) + ")";
	}
	llvm::Value *Codegen() {
		llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();
		llvm::BasicBlock *ForBeginBlock = llvm::BasicBlock::Create(llvm::getGlobalContext(), "ForBegin", TheFunction);
		llvm::BasicBlock *ForCondBlock = llvm::BasicBlock::Create(llvm::getGlobalContext(), "ForCond");
		llvm::BasicBlock *ForExecBlock = llvm::BasicBlock::Create(llvm::getGlobalContext(), "ForExec");
		llvm::BasicBlock *ForAfterBlock = llvm::BasicBlock::Create(llvm::getGlobalContext(), "ForAfter");
		llvm::BasicBlock *FinishBlock = llvm::BasicBlock::Create(llvm::getGlobalContext(), "ForEnd");

		descriptor *tempBlock = new descriptor("while", -1, -1, FinishBlock);
		descriptor *tempABlock = new descriptor("for", -1, -1, ForAfterBlock);
		SymbolTableList.front()->insert(std::pair<string, descriptor* >("while", tempBlock));
		SymbolTableList.front()->insert(std::pair<string, descriptor* >("for", tempABlock));

		Builder.CreateBr(ForBeginBlock);
		Builder.SetInsertPoint(ForBeginBlock);

		llvm::Value *preAssignCode = preAssignList->Codegen();

		Builder.CreateBr(ForCondBlock);
		TheFunction->getBasicBlockList().push_back(ForCondBlock);
		Builder.SetInsertPoint(ForCondBlock);

		llvm::Value *ConditionCode = condition->Codegen();

		if(ConditionCode->getType() == getLLVMType(17)) // Int 
			ConditionCode = Builder.CreateICmpSGT(ConditionCode, llvm::ConstantInt::get(llvm::getGlobalContext(), llvm::APInt(32, 0)), "IfCond"); 
		else if(ConditionCode->getType() == getLLVMType(18)) // bool 
			ConditionCode = Builder.CreateICmpEQ(ConditionCode, llvm::ConstantInt::get(llvm::getGlobalContext(), llvm::APInt(1, 1)), "IfCond");
		else
			throw runtime_error("Semantic error: Condition of For must be a boolean type.");

		Builder.CreateCondBr(ConditionCode, ForExecBlock, FinishBlock);

		TheFunction->getBasicBlockList().push_back(ForExecBlock);
		Builder.SetInsertPoint(ForExecBlock);
		llvm::Value *ForBlockCode = Block->Codegen();

		Builder.CreateBr(ForAfterBlock);
		TheFunction->getBasicBlockList().push_back(ForAfterBlock);
		Builder.SetInsertPoint(ForAfterBlock);
		llvm::Value *AfterAssignCode = loopAssignList->Codegen();

		Builder.CreateBr(ForCondBlock);
		TheFunction->getBasicBlockList().push_back(FinishBlock);
		Builder.SetInsertPoint(FinishBlock);

		SymbolTableList.front()->erase("while");

		return FinishBlock;
	}

	void insertSymbolIntoSymbolTable() {
		if(Block != NULL)
			Block->insertSymbolIntoSymbolTable();
	}
};

/// TODO: Not Done Return
class ReturnStmtAST : public decafAST {
	decafAST *returnValue;
public:
	ReturnStmtAST(decafAST *returnVal) {
		returnValue = returnVal;
	}
	~ReturnStmtAST() {
		delete returnValue;
	}
	string str() {
		return string("ReturnStmt(") + getString(returnValue) + ")";
	}
	llvm::Value *Codegen() {

		if(currentFunctionCall.compare("") == 0) {
			throw runtime_error("Symantic error.");
		}

		llvm::Function *TheFunction = TheModule->getFunction(currentFunctionCall);
		if(isDebugging) cout << "Now return for function call: " << currentFunctionCall << endl;

		if(returnValue != NULL) {
			if(isDebugging) cout << "Start Return Value Code generation..." << endl;
			llvm::Value *retValue = returnValue->Codegen();
			if(retValue != NULL && TheFunction->getReturnType() == retValue->getType())
				return Builder.CreateRet(retValue);
			else
				throw runtime_error("Semantic error: Return type mismatch.");
		}
		else {
			if(TheFunction->getReturnType() == Builder.getVoidTy())
				return Builder.CreateRet(NULL);
			else
				throw runtime_error("Semantic error: Return type mismatch.");
		}
	}
	void insertSymbolIntoSymbolTable() {

	}
};

/// TODO: Done
class ConstantAST : public decafAST {
	int constantValue;
public:
	ConstantAST(int value) { constantValue = value; }
	~ConstantAST() { }
	string str() { return string("NumberExpr(") + std::to_string(constantValue) + ")"; }
	llvm::Value *Codegen() {
		if(isDebugging) cout << "Starting NumberExpr Codegen" << endl;
		llvm::Value *codegen = llvm::ConstantInt::get(llvm::getGlobalContext(), llvm::APInt(32, constantValue));
		if(isDebugging) cout << "Finished NumberExpr Codegen" << endl;
		return codegen;
	}
	void insertSymbolIntoSymbolTable() {

	}
};

/// TODO: Not Done
class StatementAST : public decafAST {
	decafAST *stmtASTNode;
public:
	StatementAST(decafAST *node) { stmtASTNode = node; }

	~StatementAST() { delete stmtASTNode; }
	string str() { return getString(stmtASTNode); }
	llvm::Value *Codegen() {
		if(stmtASTNode != NULL) return stmtASTNode->Codegen();
		else return NULL;
	}
	void insertSymbolIntoSymbolTable() {
		stmtASTNode->insertSymbolIntoSymbolTable();
	}
};

/// TODO: Done
class IDTypeList : public decafAST {
	list<IDTypeStringSpecialAST* > stmts;
public:
	IDTypeList() {}
	~IDTypeList() {
		for (list<IDTypeStringSpecialAST* >::iterator i = stmts.begin(); i != stmts.end(); i++) {
			delete *i;
		}
	}
	int size() { return stmts.size(); }
	void push_front(IDTypeStringSpecialAST *e) { stmts.push_front(e); }
	void push_back(IDTypeStringSpecialAST *e) { stmts.push_back(e); }
	string pop_front() { string value = stmts.back()->str(); stmts.pop_back(); return value; }

	string str() { return commaList<class IDTypeStringSpecialAST *>(stmts); }

	vector<llvm::Type* > getTypeList() {

		vector<llvm::Type* > args;

		for (list<IDTypeStringSpecialAST* >::iterator i = stmts.begin(); i != stmts.end(); i++) {
			args.push_back(getLLVMType((*i)->getTypeId()));
		}
		return args;
	};

	llvm::Value *Codegen() {
		for (list<IDTypeStringSpecialAST* >::iterator i = stmts.begin(); i != stmts.end(); i++) {
			if((*i) != NULL) (*i)->Codegen();
		}
		return NULL;
	}
	void insertSymbolIntoSymbolTable() {
		for (list<IDTypeStringSpecialAST* >::iterator i = stmts.begin(); i != stmts.end(); i++) {
			if((*i) != NULL) (*i)->insertSymbolIntoSymbolTable();
		}
	}

	list<IDTypeStringSpecialAST* > getArgList() {
		return stmts;
	}
};

// Method Decl
/// TODO: Not Done

class MethodDeclHeadAST : public decafAST {
	string identifierName;
	int methodTypeId;
	IDTypeList *paramList;
	int startpos;
public:
	MethodDeclHeadAST(string idName, int methodType, IDTypeList *params, int linepos) {
		identifierName = idName;
		methodTypeId = methodType;
		paramList = params;
		startpos = linepos;
	}
	~MethodDeclHeadAST() {
		delete paramList;
	}
	string str() {
		return identifierName;
		//return string("Method(") + identifierName + "," + getMethodType(methodTypeId) + "," + paramList->str()+ "," ; //+ block->str() + ")";
	}
	llvm::Value *Codegen() {
		currentFunctionCall = identifierName;
		return paramList->Codegen();
	}
	void insertSymbolIntoSymbolTable() {
		// insert the argument list into the function symbol table
		paramList->insertSymbolIntoSymbolTable();
	}
	llvm::Function* insertMethodDeclIntoParentSymbolTable() {
		if(isDebugging) printf("Inserting method decl into symbol table...\n");

		llvm::Type *returnType = getLLVMType(methodTypeId);
		// get the types for every variables
		vector<llvm::Type *> args = paramList->getTypeList();

		llvm::Function *func = llvm::Function::Create(
				llvm::FunctionType::get(returnType, args, false),
				llvm::Function::ExternalLinkage,
				identifierName,
				TheModule
		);

		descriptor *newDesp = new descriptor(identifierName, methodTypeId, startpos, (llvm::Value *)func);
		SymbolTableList.front()->insert(std::pair<string, descriptor* >(identifierName, newDesp));
		return func;
	}

	int getType() {
		return methodTypeId;
	}

	list<IDTypeStringSpecialAST* > getParamList()
	{
		return paramList->getArgList();
	}
};

class MethodDeclAST : public decafAST {
	decafAST *head;
	decafAST *block;
	DecafSymbolTable *currentST;
	llvm::Function *func;
public:
	MethodDeclAST(decafAST *headMethod, decafAST *blockMethod) {
		head = headMethod;
		block = blockMethod;
	}
	~MethodDeclAST() {
		delete head;
		delete block;
	}
	string str() {
		return head->str() + block->str() + ")";
	}
	llvm::Value *Codegen() {
		SymbolTableList.push_front(currentST);

		if(isDebugging) outputCurrentSymTabList();

		//checkTable(currentST);
		if(isDebugging) printf("Debug message: Generating MethodDecl...\n");


		llvm::BasicBlock *BB = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", func);
		// Symbol table
		Builder.SetInsertPoint(BB);

		MethodDeclHeadAST* h=(MethodDeclHeadAST *)head;

		h->Codegen();

		list<IDTypeStringSpecialAST* > argList=h->getParamList();
		list<IDTypeStringSpecialAST* >::iterator i = argList.begin();
		descriptor* fetchedVar;
		for(auto &Arg : func->args()) {
			fetchedVar = getSymbolTable((*i)->getIDName());
			Builder.CreateStore(&Arg, fetchedVar->getAlloca());
			i++;
		}

		if(block != NULL) {
			if(isDebugging) cout << "Method Block CodeGen..." << endl;
			block->Codegen();
			if(isDebugging) cout << "Finish Method Block CodeGen..." << endl;
		}

		if(func->getReturnType()==getLLVMType(17)) {
			if(isDebugging) cout << "Generating Dummy Return 0...";
			Builder.CreateRet(llvm::ConstantInt::get(llvm::getGlobalContext(), llvm::APInt(32, 0)));
			if(isDebugging) cout << "Finished..." << endl;
		}
		if(func->getReturnType()==getLLVMType(18)) {
			if(isDebugging) cout << "Generating Dummy Return true...";
			Builder.CreateRet(llvm::ConstantInt::get(llvm::getGlobalContext(), llvm::APInt(1, 1)));
			if(isDebugging) cout << "Finished..." << endl;
		}
		else if(func->getReturnType()==getLLVMType(19)) {
			if(isDebugging) cout << "Generating Dummy Return void...";
			Builder.CreateRet(NULL);
			//Builder.CreateRet(llvm::ConstantInt::get(llvm::getGlobalContext(), llvm::APInt(32, 0)));
			if(isDebugging) cout << "Finished..." << endl;
		}
		
		SymbolTableList.pop_front();

		//verifyFunction(*func);

		if(isDebugging) cout << "Finish Method CodeGen..." << endl;

		currentFunctionCall = "";

		return func;
	}
	void insertSymbolIntoSymbolTable() {

		MethodDeclHeadAST * headAST = ((MethodDeclHeadAST *)head);
		MethodBlockAST * blockAST = ((MethodBlockAST *)block);

		func = headAST->insertMethodDeclIntoParentSymbolTable();

		this->currentST = new DecafSymbolTable;
		SymbolTableList.push_front(this->currentST);

		headAST ->insertSymbolIntoSymbolTable();
		blockAST->insertSymbolIntoSymbolTable();

		SymbolTableList.pop_front();
		
	}
};

