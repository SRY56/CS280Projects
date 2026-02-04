/* Implementation of Recursive-Descent Parser
	for the Simple Ada-Like (SADAL) Language
 * parser.cpp
 * Programming Assignment 2
 * Spring 2025
 */
#include <queue>
#include "parser.h"


map<string, bool> defVar;

namespace Parser {
bool pushed_back = false;
LexItem	pushed_token;

static LexItem GetNextToken(istream& in, int& line) {
	if( pushed_back ) {
		pushed_back = false;
		return pushed_token;
	}
	LexItem result = getNextToken(in, line);
	return result;
}

static void PushBackToken(LexItem & t) {
	if( pushed_back ) {
		abort();
	}
	pushed_back = true;
	pushed_token = t;
}

}

static int error_count = 0;

int ErrCount()
{
	return error_count;
}

void ParseError(int line, string msg)
{
	++error_count;
	cout << line << ": " << msg << endl;
}

//StmtList ::= Stmt { Stmt }
bool StmtList(istream& in, int& line)
{
	bool status;
	LexItem tok;
	status = Stmt(in, line);
	tok = Parser::GetNextToken(in, line);
	while(status && (tok != END && tok != ELSIF && tok != ELSE))
	{
		Parser::PushBackToken(tok);
		status = Stmt(in, line);
		tok = Parser::GetNextToken(in, line);
	}
	if(!status)
	{
		ParseError(line, "Syntactic error in statement list.");
		return false;
	}
	Parser::PushBackToken(tok); //push back the END token
	return true;
}//End of StmtList

//DeclPart ::= DeclStmt { DeclStmt }
bool DeclPart(istream& in, int& line) {
	bool status = false;
	LexItem tok;
	status = DeclStmt(in, line);
	if(status)
	{
		tok = Parser::GetNextToken(in, line);
		if(tok == BEGIN )
		{
			Parser::PushBackToken(tok);
			return true;
		}
		else
		{
			Parser::PushBackToken(tok);
			status = DeclPart(in, line);
		}
	}
	else
	{
		ParseError(line, "Non-recognizable Declaration Part.");
		return false;
	}
	return true;
}//end of DeclPart function

bool Prog(istream& in, int& line) {
	LexItem procItem = Parser::GetNextToken(in,line);
	if (procItem.GetLexeme() != "procedure") {
		ParseError(line,"Incorrect compilation file.");
		return false;
	}
	LexItem procName = Parser::GetNextToken(in,line);
	if (procName.GetToken() != IDENT) {
		ParseError(line,"Missing Procedure Name.");
		return false;
	}
	defVar.insert({procName.GetLexeme(), false});
	LexItem isKW = Parser::GetNextToken(in,line);
	if (isKW.GetLexeme() != "is") {
		ParseError(line,"Missing is Keyword.");
		return false;
	}
	if(!ProcBody(in,line)) {
		ParseError(line, "Incorrect procedure Definition.");
		return false;
	}

	cout<<"Declared variables:\n";
	map<string, bool>::iterator it;
	int size = defVar.size();
	int i = 0;
	for (it = defVar.begin(); it != defVar.end(); it++)
	{
		cout << it->first;
		if (i<size-1) {
			cout<<", ";
		}
		i++;
	}
	cout<<"\n\n";
	cout<<"(DONE)\n";
	return true;
}

bool ProcBody(istream& in, int& line) {
	bool result = DeclPart(in,line);
	LexItem item;
	if (result) {
		item = Parser::GetNextToken(in,line);
		if (item.GetToken() == BEGIN) {
			result = StmtList(in,line);
			if (result) {
				item = Parser::GetNextToken(in,line);
				if (item.GetToken() == END) {
					item = Parser::GetNextToken(in,line);
					if (item.GetToken() == IDENT) {
						return true;
					}
				}
			}
		}
	}
	ParseError(line, "Incorrect procedure body.");
	return false;
}

//DeclStmt ::= IDENT {, IDENT } : [CONSTANT] Type [(Range)] [:= Expr] ;
bool DeclStmt(istream& in, int& line) {
	LexItem item = Parser::GetNextToken(in,line);
	if (item.GetToken() != IDENT) {
		ParseError(line,"Incorrect Declaration Statement Syntax.");
		return false;
	}
	try {
		bool flag =  defVar.at(item.GetLexeme());
		ParseError(line,"Variable Redefinition");
		ParseError(line,"Incorrect identifiers list in Declaration Statement.");
		return false;
	}
	catch(...) {
		defVar.insert({item.GetLexeme(), false});
	}

	item = Parser::GetNextToken(in,line);

	while (item.GetToken() != COLON) {
		if (item.GetToken() == COMMA) {
			item = Parser::GetNextToken(in,line);
			if (item.GetToken() == IDENT) {
				try {
					bool flag =  defVar.at(item.GetLexeme());
					ParseError(line,"Variable Redefinition");
					ParseError(line,"Incorrect identifiers list in Declaration Statement.");
					return false;
				}
				catch(...) {
					defVar.insert({item.GetLexeme(), false});
				}
			}
			else{
				ParseError(line,"IDENT expected after COMMA in decl part:"+ item.GetLexeme());
				return false;
			}
		}
		else{
			if (item.GetToken() == STRING || item.GetToken() == INT ||
					item.GetToken() == CHAR || item.GetToken() == FLOAT
					|| item.GetToken() == BOOL
			) {
				ParseError(line,"Invalid name for an Identifier:\n("+ item.GetLexeme()+")");
			}
			else {
				ParseError(line,"Missing comma in declaration statement.");
			}
			ParseError(line,"Incorrect identifiers list in Declaration Statement.");
			return false;
		}

		item = Parser::GetNextToken(in,line);
	}
	item = Parser::GetNextToken(in,line);
	if (item.GetToken() != CONST) {
		Parser::PushBackToken(item);
	}

	bool result = Type(in,line);
	if (!result) {
		return false;
	}

	item = Parser::GetNextToken(in,line);
	if (item.GetToken() == ASSOP) {
		result = Expr(in,line);
		if (result) {
			item = Parser::GetNextToken(in,line);
			if (item.GetToken() == SEMICOL) {
				return true;
			}
			else{
				return false;
			}
		}
		return result;
	}
	else if (item.GetToken() == LPAREN) {
		if (!Range(in,line)) {
			return false;
		}
		item = Parser::GetNextToken(in,line);
		if (item.GetToken() != RPAREN) {
			return false;
		}
		item = Parser::GetNextToken(in,line);
		if (item.GetToken() != SEMICOL) {
			return false;
		}
		return true;
	}
	else if (item.GetToken() == SEMICOL) {
		return true;
	}
	else {
		Parser::PushBackToken(item);
		ParseError(line,"Semicolon expected at the end of decl part:"+ item.GetLexeme());
		return false;
	}

}

bool Type(istream& in, int& line){
	LexItem item = Parser::GetNextToken(in,line);
	if (item.GetToken() == INT || item.GetToken() == FLOAT ||
			item.GetToken() == CHAR || item.GetToken() == BOOL ||
			item.GetToken() == STRING) {
		return true;
	}
	ParseError(line,"Incorrect Declaration Type.");
	Parser::PushBackToken(item);
	return false;
}


bool Stmt(istream& in, int& line){
	if (AssignStmt(in,line)) {
		return true;
	}
	else if (PrintStmts(in,line)) {
		return true;
	}
	else if (GetStmt(in,line)) {
		return true;
	}
	else if (IfStmt(in,line)) {
		return true;
	}
	else {
		return false;
	}
}

bool PrintStmts(istream& in, int& line){
	LexItem item = Parser::GetNextToken(in,line);
	if (item.GetToken() == PUT) {
		item = Parser::GetNextToken(in,line);
		if (item.GetToken() != LPAREN) {
			ParseError(line,"Missing Left Parenthesis");
			ParseError(line,"Invalid put statement.");
			return false;
		}
		if (!Expr(in,line)){
			Parser::PushBackToken(item);
			ParseError(line,"Invalid put statement.");
			return false;
		}
		item = Parser::GetNextToken(in,line);
		if (item.GetToken() != RPAREN) {
			ParseError(line,"Missing Right Parenthesis");
			ParseError(line,"Invalid put statement.");
			return false;
		}
		item = Parser::GetNextToken(in,line);
		if (item.GetToken() != SEMICOL) {
			ParseError(line,"Missing semicolon at end of statement");
			ParseError(line,"Invalid put statement.");
			return false;
		}
		return true;
	}
	else if (item.GetToken() == PUTLN) {
		item = Parser::GetNextToken(in,line);
		if (item.GetToken() != LPAREN) {
			ParseError(line,"Missing Left Parenthesis");
			ParseError(line,"Invalid put statement.");
			return false;
		}
		if (!Expr(in,line)){
			Parser::PushBackToken(item);
			ParseError(line,"Invalid put statement.");
			return false;
		}
		item = Parser::GetNextToken(in,line);
		if (item.GetToken() != RPAREN) {
			ParseError(line,"Missing Right Parenthesis");
			ParseError(line,"Invalid put statement.");
			return false;
		}
		item = Parser::GetNextToken(in,line);
		if (item.GetToken() != SEMICOL) {
			ParseError(line,"Missing semicolon at end of statement");
			ParseError(line,"Invalid put statement.");
			return false;
		}
		return true;
	}
	else {
		Parser::PushBackToken(item);
		return false;
	}
}

bool GetStmt(istream& in, int& line){
	LexItem item = Parser::GetNextToken(in,line);
	if (item.GetToken() == GET) {
		item = Parser::GetNextToken(in,line);
		if (item.GetToken() != LPAREN) {
			return false;
		}
		if (!Var(in,line)) {
			return false;
		}
		item = Parser::GetNextToken(in,line);
		if (item.GetToken() != RPAREN) {
			return false;
		}
		item = Parser::GetNextToken(in,line);
		if (item.GetToken() != SEMICOL) {
			return false;
		}
		return true;
	}
	Parser::PushBackToken(item);
	return false;
}

bool IfStmt(istream& in, int& line){
	LexItem item = Parser::GetNextToken(in,line);
	if (item.GetToken() == IF) {
		if (!Expr(in,line)) {
			ParseError(line, "Missing if statement condition");
			ParseError(line, "Invalid If statement.");
			return false;
		}
		item = Parser::GetNextToken(in,line);
		if (item.GetToken() != THEN) {
			ParseError(line, "Missing Statement for If-Stmt Then-clause");
			ParseError(line, "Invalid If statement.");
			return false;
		}
		if (!StmtList(in,line)) {
			ParseError(line, "Missing Statement for If-Stmt Then-clause");
			ParseError(line, "Invalid If statement.");
			return false;
		}
		item = Parser::GetNextToken(in,line);
		while(1) {
			if (item.GetToken() == ELSIF) {
				if (!Expr(in,line)) {
					return false;
				}
				item = Parser::GetNextToken(in,line);
				if (item.GetToken() != THEN) {
					return false;
				}
				if (!StmtList(in,line)) {
					return false;
				}
			}
			else {
				Parser::PushBackToken(item);
				break;
			}
			item = Parser::GetNextToken(in,line);
		}
		item = Parser::GetNextToken(in,line);
		if (item.GetToken() == ELSE) {
			if (!StmtList(in,line)) {
				return false;
			}
			item = Parser::GetNextToken(in,line);
		}
		if (item.GetToken() != END) {
			ParseError(line, "Missing closing END IF for If-statement.");
			ParseError(line, "Invalid If statement.");
			return false;
		}
		item = Parser::GetNextToken(in,line);
		if (item.GetToken() != IF) {
			Parser::PushBackToken(item);
			return false;
		}
		item = Parser::GetNextToken(in,line);
		if (item.GetToken() != SEMICOL) {
			Parser::PushBackToken(item);
			return false;
		}
	}
	else{
		Parser::PushBackToken(item);
		return false;
	}
	return true;
}

bool AssignStmt(istream& in, int& line){
	if (!Var(in,line)) {
		return false;
	}
	LexItem item = Parser::GetNextToken(in,line);
	if (item.GetToken() != ASSOP) {
		ParseError(line,"Missing Assignment Operator");
		ParseError(line,"Invalid assignment statement.");
		Parser::PushBackToken(item);
		return false;
	}
	if (!Expr(in,line)) {
		ParseError(line, "Missing operand after operator");
		ParseError(line, "Missing Expression in Assignment Statement");
		ParseError(line, "Invalid assignment statement.");
		return false;
	}
	item = Parser::GetNextToken(in,line);
	if (item.GetToken() != SEMICOL) {
		ParseError(line,"Illegal expression for an assignment statement");
		ParseError(line,"Invalid assignment statement.");
		Parser::PushBackToken(item);
		return false;
	}
	return true;
}


bool Var(istream& in, int& line){
	LexItem item = Parser::GetNextToken(in,line);
	if (item.GetToken() == IDENT) {
		return true;
	}
	Parser::PushBackToken(item);
	return false;
}

bool Expr(istream& in, int& line){
	if (!Relation(in,line)) {
		return false;
	}

	LexItem item = Parser::GetNextToken(in,line);
	while(1) {
		if (item.GetToken() == AND || item.GetToken() == OR) {
			if (!Relation(in,line)) {
				return false;
			}
		}
		else {
			Parser::PushBackToken(item);
			break;
		}
		item = Parser::GetNextToken(in,line);
	}
	return true;
}

bool Relation(istream& in, int& line){
	if (!SimpleExpr(in,line)) {
		return false;
	}
	LexItem item = Parser::GetNextToken(in,line);

	if (item.GetToken() == EQ || item.GetToken() == LTHAN ||
			item.GetToken() == NEQ || item.GetToken() == LTE ||
			item.GetToken() == GTHAN || item.GetToken() == GTE) {
		if (!SimpleExpr(in,line)) {
			return false;
		}
	}
	else{
		Parser::PushBackToken(item);
	}
	return true;
}

bool SimpleExpr(istream& in, int& line){
	if (!STerm(in,line)) {
		return false;
	}
	LexItem item = Parser::GetNextToken(in,line);
	int count = 0;
	while(1) {
		if (item.GetToken() == PLUS || item.GetToken() == MINUS
				|| item.GetToken() == CONCAT) {
			if (!STerm(in,line)) {
				return false;
			}
			count++;
		}
		else {
			Parser::PushBackToken(item);
			break;
		}
		item = Parser::GetNextToken(in,line);
	}
	return true;
}

bool STerm(istream& in, int& line){
	LexItem item = Parser::GetNextToken(in,line);
	Token sign = PLUS;
	if (item.GetToken() == PLUS || item.GetToken() == MINUS) {
		sign = item.GetToken();
		if (!Term(in,line,sign)) {
			return false;
		}
		else{
			return true;
		}
	}
	else{
		Parser::PushBackToken(item);
	}
	if(!Term(in,line, sign)) {
		return false;
	}
	return true;
}

bool Term(istream& in, int& line, int sign){
	if (!Factor(in,line,sign)) {
		ParseError(line, "Incorrect Operand");
		return false;
	}
	LexItem item = Parser::GetNextToken(in,line);
	int count = 0;
	while(1) {
		if (item.GetToken() == MULT || item.GetToken() == DIV
				|| item.GetToken() == MOD) {
			if (!Factor(in,line,sign)) {
				ParseError(line, "Incorrect Operand");
				return false;
			}
			count++;
		}
		else {
			Parser::PushBackToken(item);
			break;
		}
		item = Parser::GetNextToken(in,line);
	}
	return true;
}

bool Factor(istream& in, int& line, int sign){
	if (Primary(in,line,sign)) {
		LexItem item = Parser::GetNextToken(in,line);
		if (item.GetToken() == EXP) {
			if (!Primary(in,line,sign)) {
				return false;
			}
			else {
				return true;
			}
		}
		else {
			Parser::PushBackToken(item);
		}
		return true;
	}
	else {
		LexItem item = Parser::GetNextToken(in,line);
		if (item.GetToken() == NOT) {
			if (!Primary(in,line,sign)) {
				return false;
			}
		}
		else {
			Parser::PushBackToken(item);
			return false;
		}
	}
	return true;
}

bool Primary(istream& in, int& line, int sign){
	LexItem item = Parser::GetNextToken(in,line);
	if (item.GetToken() == ICONST || item.GetToken() == FCONST || item.GetToken() == BCONST || item.GetToken() == SCONST ||
			item.GetToken() == CCONST) {
		return true;
	}
	Parser::PushBackToken(item);
	if (Name(in,line)) {
		return true;
	}

	item = Parser::GetNextToken(in,line);
	if (item.GetToken() == LPAREN) {
		if (!Expr(in,line)) {
			return false;
		}
		item = Parser::GetNextToken(in,line);
		if (item.GetToken() != RPAREN) {
			Parser::PushBackToken(item);
			ParseError(line, "Missing right parenthesis after expression");
			return false;
		}
		return true;
	}
	else {
		Parser::PushBackToken(item);
	}
	return false;
}

bool Name(istream& in, int& line){
	LexItem item = Parser::GetNextToken(in,line);
	if (item.GetToken() == IDENT) {
		try {
			bool flag =  defVar.at(item.GetLexeme());
		}
		catch(...) {
			ParseError(line,"Using Undefined Variable");
			ParseError(line,"Invalid reference to a variable.");
			return false;
		}

		item = Parser::GetNextToken(in,line);
		if (item.GetToken() == LPAREN) {
			if (!Range(in,line)) {
				return false;
			}
			item = Parser::GetNextToken(in,line);
			if (item.GetToken() != RPAREN) {
				return false;
			}
		}
		else {
			Parser::PushBackToken(item);
		}
		return true;
	}
	else {
		Parser::PushBackToken(item);
		if (item.GetToken() == SEMICOL)
			ParseError(line, "Invalid expression");
		return false;
	}
}

bool Range(istream& in, int& line){
	if (!SimpleExpr(in,line)) {
		return false;
	}
	LexItem item = Parser::GetNextToken(in,line);
	if (item.GetToken() == DOT){
		item = Parser::GetNextToken(in,line);
		if (item.GetToken() != DOT){
			Parser::PushBackToken(item);
			return false;
		}
		if (!SimpleExpr(in,line)) {
			return false;
		}
	}
	else{
		Parser::PushBackToken(item);
	}
	return true;
}

