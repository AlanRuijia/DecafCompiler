
#include "decafexpr-defs.h"
#include <list>
#include <ostream>
#include <iostream>
#include <sstream>

#ifndef YYTOKENTYPE
#include "decafexpr.tab.h"
#endif

using namespace std;

/// decafAST - Base class for all abstract syntax tree nodes.
/// TODO:Done
class decafAST {
public:
  virtual ~decafAST() {}
  virtual string str() { return string(""); }
	virtual llvm::Value *Codegen() = 0;
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
	list<decafAST *> stmts;
public:
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
};

/// TODO:Done
// Package
class PackageAST : public decafAST {
	string Name;
	decafStmtList *FieldDeclList;
	decafStmtList *MethodDeclList;
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
		llvm::Value *val = NULL;
		TheModule->setModuleIdentifier(llvm::StringRef(Name));
		if (NULL != FieldDeclList) {
			val = FieldDeclList->Codegen();
		}
		if (NULL != MethodDeclList) {
			val = MethodDeclList->Codegen();
		}
		// Q: should we enter the class name into the symbol table?
		return val;
	}
};

/// TODO: Not Done
// Expr
class ExprAST : public decafAST {
	decafAST *decafASTNode;
public:
	ExprAST (decafAST *node) { decafASTNode = node; }
	~ExprAST() { delete decafASTNode; }
	string str() { return getString(decafASTNode); }
	llvm::Value *Codegen() {
		llvm::Value *val = NULL;
		return val;
	}
};

/// TODO:Done
/// ProgramAST - the decaf program
class ProgramAST : public decafAST {
	decafStmtList *ExternList;
	PackageAST *PackageDef;
public:
	ProgramAST(decafStmtList *externs, PackageAST *c) : ExternList(externs), PackageDef(c) {}
	~ProgramAST() { 
		if (ExternList != NULL) { delete ExternList; } 
		if (PackageDef != NULL) { delete PackageDef; }
	}
	string str() { return string("Program") + "(" + getString(ExternList) + "," + getString(PackageDef) + ")"; }
	llvm::Value *Codegen() {
		llvm::Value *val = NULL;
		if (NULL != ExternList) {
			val = ExternList->Codegen();
		}
		if (NULL != PackageDef) {
			val = PackageDef->Codegen();
		} else {
			throw runtime_error("no package definition in decaf program");
		}
		return val;
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

llvm::Value* BinaryOpExpr(int opId, llvm::Value *L, llvm::Value *R) {
	switch(opId) {
		case 0: return Builder.CreateAdd(L, R, "AddTmp");
		case 1: return Builder.CreateSub(L, R, "SubTmp");
		case 2: return Builder.CreateMul(L, R, "MulTmp");
		case 3: return Builder.CreateSDiv(L, R, "DivTmp");
		case 4: return Builder.CreateShl(L, R, "LShiftTmp");
		case 5: return Builder.CreateLShr(L, R, "RShitTmp");
		case 6: return Builder.CreateSRem(L, R, "RemainTmp");
		case 7: return Builder.CreateICmpSLT(L, R, "LTTmp");
		case 8: return Builder.CreateICmpSGT(L, R, "GTTmp");
		case 9: return Builder.CreateICmpSLE(L, R, "LEQTmp");
		case 10: return Builder.CreateICmpSGE(L, R, "GEQTmp");
		case 11: return Builder.CreateICmpEQ(L, R, "EQTmp");
		case 12: return Builder.CreateICmpNE(L, R, "NEQTmp");
		case 13: return Builder.CreateAnd(L, R, "AndTmp");
		case 14: return Builder.CreateOr(L, R, "OrTmp");
		default: return NULL;
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

llvm::Type* getLLVMType(int typeIndex) {
	switch (typeIndex) {
		case 17: return Builder.getInt32Ty();
		case 18: return Builder.getInt1Ty();
		case 19: return Builder.getVoidTy();
		default: return NULL;
	}
}

/// TODO: Not Done
class BoolConstantAST : public decafAST {
	bool value;
public:
	BoolConstantAST(bool type) { value = type; };
	~BoolConstantAST() { }
	string str() { if(value) return string("BoolExpr(True)"); else return string("BoolExpr(False)"); }
	llvm::Value *Codegen() {
		llvm::Value *val = NULL;
		return val;
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
		llvm::Value *val = NULL;
		return val;
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
		llvm::Value *L = leftValueNode->Codegen();
		llvm::Value *R = rightValueNode->Codegen();
		if(L == 0 || R == 0) return 0;

		return BinaryOpExpr(binaryOperator, L, R);
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
		llvm::Value *val = NULL;
		return val;
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
		llvm::Value *val = NULL;
		return val;
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
		llvm::Value *val = NULL;
		return val;
	}
};

// String
/// TODO: Not Done
class StringAST : public decafAST {
	string decafASTString;
public:
	StringAST(string value) { decafASTString = value; }
	~StringAST() { }
	string str() { return string("StringConstant") + "(" + decafASTString + ")"; }
	//string rawStr() { return decafASTString; }
	llvm::Value *Codegen() {
		llvm::Value *val = NULL;
		return val;
	}
};

/// TODO: Done
class IDTypeStringAST : public decafAST {
	string decafASTString;
	int methodType;
public:
	IDTypeStringAST(string value, int typeId) { decafASTString = value; methodType = typeId; }
	~IDTypeStringAST() { }
	string str() { return decafASTString + "," + getMethodType(methodType); }
	string getName() { return decafASTString; }
	int getType() { return methodType; }
	//string rawStr() { return decafASTString; }
	llvm::Value *Codegen() {
		llvm::Value *val = NULL;
		return val;
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
		llvm::Value *val = NULL;
		return val;
	}
};

/// TODO: Not Done
class RawStringAST : public decafAST {
	string decafASTString;
public:
	RawStringAST(string value) { decafASTString = value; }
	~RawStringAST() { }
	string str() { return decafASTString; }
	//string rawStr() { return decafASTString; }
	llvm::Value *Codegen() {
		llvm::Value *val = NULL;
		return val;
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
		llvm::Value *val = NULL;
		return val;
	}
};

// Extern Type
/// TODO: Not Done
class ExternType : public decafAST {
	int externType;
public:
	ExternType(int typeIndex) { externType = typeIndex; }
	~ExternType() { }
	string str() {
		return string("VarDef(") + getExternType(externType) + ")";
	}
	llvm::Value *Codegen() {
		llvm::Value *val = NULL;
		return val;
	}
};

// Extern
/// TODO: Not Done
class ExternAST : public decafAST {
	string identifierName;
	int methodTypeId;
	decafStmtList *typeList;
public:
	ExternAST(string IdName, int typeId, decafStmtList *list) {
		identifierName = IdName;
		methodTypeId = typeId;
		typeList = list;
	}
	~ExternAST() {
		identifierName = "";
		delete typeList;
	}
	string str() {
		return string("ExternFunction(") + identifierName + "," + getMethodType(methodTypeId) + "," + typeList->str() + ")";
	}
	llvm::Value *Codegen() {
		llvm::Value *val = NULL;
		return val;
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
		llvm::Value *val = NULL;
		return val;
	}
};

// Field_decl
/// TODO: Not Done
class FieldDeclAST : public decafAST {
	string identifierName;
	int decafTypeId;
	bool isGlobal;
	decafAST *expr;
public:
	FieldDeclAST(string idName, int typeId, decafAST *exprNode, bool global) { identifierName = idName; decafTypeId = typeId; isGlobal = global; expr = exprNode; }
	~FieldDeclAST() { }
	string str() {
		if(isGlobal)
			return string("AssignGlobalVar(") + identifierName + "," + getDecafType(decafTypeId) + "," + getString(expr) + ")";
		else
			if (decafTypeId != -1) return string("FieldDecl(") + identifierName + "," + getDecafType(decafTypeId) + "," + getString(expr) + ")";
			else return string("FieldDecl(") + identifierName + "," + getString(expr) + ")";
	}
	llvm::Value *Codegen() {
		llvm::Value *val = NULL;
		return val;
	}
};

// typed symbol
/// TODO: Not Done
class TypedSymbol : public decafAST {
	string identifierName;
	int decafTypeId;
public:
	TypedSymbol(string idName, int typeId) {
		identifierName = idName;
		decafTypeId = typeId;
	}
	~TypedSymbol() { }
	string str() {
		return string("VarDef(") + identifierName + "," + getDecafType(decafTypeId) + ")";
	}
	llvm::Value *Codegen() {
		llvm::Value *val = NULL;
		return val;
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
		llvm::Value *val = NULL;
		return val;
	}
};


// Block
/// TODO: Not Done
class BlockAST : public decafAST {
	decafStmtList *varDeclList;
	decafStmtList *statementList;
	bool isMethod;
public:
	BlockAST(decafStmtList *varDecl, decafStmtList *stmt) {
		varDeclList = varDecl;
		statementList = stmt;
	}
	~BlockAST() { delete varDeclList; delete statementList; };
	void turnToMethod() { isMethod = true; }
	string str() {
		if(!isMethod) return string("Block(") + varDeclList->str() + "," + statementList->str() + ")";
		else return string("MethodBlock(") + varDeclList->str() + "," + statementList->str() + ")";
	}
	llvm::Value *Codegen() {
		llvm::Value *val = NULL;
		return val;
	}
};

// Break, Continue
/// TODO: Not Done Break Continue
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
		llvm::Value *val = NULL;
		return val;
	}
};

/// TODO: Not Done
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

		llvm::Value *ConditionCode = condition->Codegen();
		if(!ConditionCode)
			return NULL;

		ConditionCode = Builder.CreateFCmpONE(ConditionCode, llvm::ConstantFP::get(llvm::getGlobalContext(), llvm::APFloat(0.0)), "IfCond");
		llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();

		llvm::BasicBlock *ThenBlock = llvm::BasicBlock::Create(llvm::getGlobalContext(), "Then", TheFunction);
		llvm::BasicBlock *EBlock = llvm::BasicBlock::Create(llvm::getGlobalContext(), "Else");
		llvm::BasicBlock *MergeBlock = llvm::BasicBlock::Create(llvm::getGlobalContext(), "IfCont");
		Builder.CreateCondBr(ConditionCode, ThenBlock, EBlock);

		Builder.SetInsertPoint(ThenBlock);

		llvm::Value *ThenCode = ifBlock->Codegen();
		if(!ThenCode) return NULL;
		Builder.CreateBr(MergeBlock);
		ThenBlock = Builder.GetInsertBlock();
		TheFunction->getBasicBlockList().push_back(EBlock);
		Builder.SetInsertPoint(EBlock);

		llvm::Value *ElseCode = ElseBlock->Codegen();
		if(!ElseCode) return NULL;
		Builder.CreateBr(MergeBlock);
		EBlock = Builder.GetInsertBlock();
		TheFunction->getBasicBlockList().push_back(MergeBlock);
		Builder.SetInsertPoint(MergeBlock);

		llvm::PHINode *node = Builder.CreatePHI(llvm::Type::getDoubleTy(llvm::getGlobalContext()), 2, "IfTmp");
		node->addIncoming(ThenCode, ThenBlock);
		node->addIncoming(ElseCode, EBlock);

		return node;
	}
};

/// TODO: Not Done
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
		llvm::Value *val = NULL;
		return val;
	}
};

/// TODO: Not Done
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
		llvm::Value *val = NULL;
		return val;
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
		llvm::Value *val = NULL;
		return val;
	}
};

/// TODO: Not Done
class ConstantAST : public decafAST {
	int constantValue;
public:
	ConstantAST(int value) { constantValue = value; }
	~ConstantAST() { }
	string str() { return string("NumberExpr(") + std::to_string(constantValue) + ")"; }
	llvm::Value *Codegen() {
		llvm::Value *val = NULL;
		return val;
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
		llvm::Value *val = NULL;
		return val;
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
			int tempTypeId = (*i)->getTypeId();
			llvm::Type *tempType = getLLVMType(tempTypeId);
			args.push_back(tempType);
		}
		return args;
	};

	llvm::Value *Codegen() {
		llvm::Value *val = NULL;
		return val;
	}
};

// Method Decl
/// TODO: Not Done

class MethodDeclHeadAST : public decafAST {
	string identifierName;
	int methodTypeId;
	IDTypeList *paramList;
public:
	MethodDeclHeadAST(string idName, int methodType, IDTypeList *params) {
		identifierName = idName;
		methodTypeId = methodType;
		paramList = params;
	}
	~MethodDeclHeadAST() {
		delete paramList;
	}
	string str() {
		return string("Method(") + identifierName + "," + getMethodType(methodTypeId) + "," + paramList->str()+ "," ; //+ block->str() + ")";
	}
	llvm::Value *Codegen() {
		llvm::Type *returnType = getLLVMType(methodTypeId);
		// get the types for every variables
		vector<llvm::Type *> args = paramList->getTypeList();

		llvm::Function *func = llvm::Function::Create(
				llvm::FunctionType::get(returnType, args, false),
				llvm::Function::ExternalLinkage,
				identifierName,
				TheModule
		);

		return func;
	}
};

class MethodDeclAST : public decafAST {
	decafAST *head;
	decafAST *block;
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
		llvm::Function *func = (llvm::Function *)head->Codegen();

		llvm::BasicBlock *BB = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", func);
		// Symbol table
		Builder.SetInsertPoint(BB);
		return NULL;
	}
};

class descriptor {
	string identifierName;
	int type;
	int lineNumber;
	//llvm::AllocaInst *Alloca;
	void *ptr;
	bool isFunction;

public:
	descriptor(string idName, int targetType, int lineNo, llvm::AllocaInst *allocai) {
		identifierName = idName;
		type = targetType;
		lineNumber = lineNo;
		//Alloca = allocai;
		ptr = allocai;
		isFunction = false;
		//cout << "Defined variable in line " << lineNumber << " : " << identifierName << endl;
	}
	descriptor(string idName, int targetType, int lineNo, llvm::Function *func) {
		identifierName = idName;
		type = targetType;
		lineNumber = lineNo;
		ptr = func;
		isFunction = true;
		//cout << "Defined variable in line " << lineNumber << " : " << identifierName << endl;
	}
	~descriptor() { }
	void debug() {
		cout << "Defined variable in line " << lineNumber << " : " << identifierName << endl;
	}
	llvm::Type *getType() {
		/*if(type == 17) return Builder.getInt32Ty();
		if(type == 18) return Builder.getInt1Ty();
		if(type == 19) return Builder.getVoidTy();
		else return NULL;*/
		return getLLVMType(type);
		//return Alloca->getType();
	}

};