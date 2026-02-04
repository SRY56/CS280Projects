#include <iostream>
#include <vector>
#include <queue>
#include <sstream>
#include <map>
#include "parserInterp.h"
#include <limits> 

using namespace std;

//--------------------------------------------------
// Global tables and containers
//--------------------------------------------------
map<string, bool> defVar;                   // declared vars
map<string, Token> SymTable;               // var → type
map<string, Value> TempsResults;           // var → current value
queue<string>*     Ids_List;               // helper for DeclStmt

static bool failureInDeclPart = false;
static bool inAssignStmt = false;
static string currentProcName;

//--------------------------------------------------
// Pushback token framework (from PA2)
//--------------------------------------------------
namespace Parser {
    static bool pushed_back = false;
    static LexItem pushed_token;

    static LexItem GetNextToken(istream& in, int& line) {
        if (pushed_back) {
            pushed_back = false;
            return pushed_token;
        }
        return getNextToken(in, line);
    }

    static void PushBackToken(LexItem & t) {
        if (pushed_back) abort();
        pushed_back = true;
        pushed_token = t;
    }
}

//--------------------------------------------------
// Error counting / reporting
//--------------------------------------------------
static int error_count = 0;

int ErrCount() {
    return error_count;
}

void ParseError(int line, string msg) {
    ++error_count;
    cout << line << ": " << msg << endl;
}

// Just checks declaration (no init check)
// static bool VarDeclared(istream& in, int& line, LexItem & idtok) {
//     LexItem tok = Parser::GetNextToken(in, line);
//     if (tok.GetToken() == IDENT) {
//         idtok = tok;
//         if (!defVar[tok.GetLexeme()]) {
//             ParseError(line, "Undeclared Variable");
//             return false;
//         }
//         return true;
//     }
//     Parser::PushBackToken(tok);
//     return false;
// }


//--------------------------------------------------
// (Optional) dump symbol & value tables at end
//--------------------------------------------------
void DumpTables() {
    auto tokenToString = [](Token tok)->string {
        switch(tok) {
            case INT:    return "INTEGER";
            case FLOAT:  return "FLOAT";
            case STRING: return "STRING";
            case CHAR:   return "CHARACTER";
            case BOOL:   return "BOOLEAN";
            default:     return "UNKNOWN";
        }
    };
    cout << "\nSymbol Table:\n";
    for (auto &p : SymTable)
        cout << "  " << p.first << " : " << tokenToString(p.second) << "\n";

    cout << "\nValue Table:\n";
    for (auto &p : TempsResults)
        cout << "  " << p.first << " = " << p.second << "\n";
}

//--------------------------------------------------
// Forward declarations (in header)
//--------------------------------------------------
bool Prog(istream& in, int& line);
bool ProcBody(istream& in, int& line);
bool DeclPart(istream& in, int& line);
bool DeclStmt(istream& in, int& line);
bool Type(istream& in, int& line);
bool IdentList(istream& in, int& line);
bool StmtList(istream& in, int& line);
bool Stmt(istream& in, int& line);
bool PrintStmts(istream& in, int& line);
bool GetStmt(istream& in, int& line);
bool IfStmt(istream& in, int& line);
bool AssignStmt(istream& in, int& line);
bool Var(istream& in, int& line, LexItem & idtok);
bool Expr(istream& in, int& line, Value & retVal);
bool Relation(istream& in, int& line, Value & retVal);
bool SimpleExpr(istream& in, int& line, Value & retVal);
bool STerm(istream& in, int& line, Value & retVal);
bool Term(istream& in, int& line, int sign, Value & retVal);
bool Factor(istream& in, int& line, int sign, Value & retVal);
bool Primary(istream& in, int& line, int sign, Value & retVal);
bool Name(istream& in, int& line, int sign, Value & retVal);
bool Range(istream& in, int& line, Value & retVal1, Value & retVal2);

//--------------------------------------------------
// Grammar functions
//--------------------------------------------------

// Prog ::= PROCEDURE ProcName IS ProcBody
bool Prog(istream& in, int& line)
{
    // 1) PROCEDURE
    LexItem tok = Parser::GetNextToken(in, line);
    if (tok.GetToken() != PROCEDURE) {
        ParseError(line, "Incorrect compilation file.");
        return false;
    }

    // 2) ProcName
    tok = Parser::GetNextToken(in, line);
    if (tok.GetToken() != IDENT) {
        ParseError(line, "Missing Procedure Name.");
        return false;
    }
    currentProcName = tok.GetLexeme();
    defVar[currentProcName] = true;

    // 3) IS
    tok = Parser::GetNextToken(in, line);
    if (tok.GetToken() != IS) {
        ParseError(line, "Incorrect Procedure Header Format.");
        return false;
    }

    // 4) ProcBody
    bool bodyOk = ProcBody(in, line);
    if (!bodyOk) {
        if (failureInDeclPart) {
            ParseError(line + 1, "Incorrect compilation file.");
        } else {
            ParseError(line, "Incorrect Procedure Definition.");
        }
        return false;
    }

    // 5) DONE token
    tok = Parser::GetNextToken(in, line);
    if (tok.GetToken() != DONE) {
        ParseError(line, "Incorrect compilation file.");
        return false;
    }

    // ** Success: just print DONE **
    cout << endl;
    cout << "(DONE)" << endl;
    return true;
}



// ProcBody ::= DeclPart BEGIN StmtList END ProcName ;
bool ProcBody(istream& in, int& line)
{
    // 1) parse declarations
    if (!DeclPart(in, line)) {
        failureInDeclPart = true;
        return false;
    }

    // 2) expect BEGIN
    LexItem tok = Parser::GetNextToken(in, line);
    if (tok.GetToken() != BEGIN) {
        ParseError(line, "Incorrect procedure body.");
        return false;
    }

    // 3) parse statements
    if (!StmtList(in, line)) {
        ParseError(line, "Incorrect Proedure Body.");
        return false;
    }

    // 4) expect END procName ;
	tok = Parser::GetNextToken(in, line);
	if (tok.GetToken() != END) {
		ParseError(line, "Missing END of Procedure Keyword.");
		return false;
	}

	// read the closing identifier
	tok = Parser::GetNextToken(in, line);
	if (tok.GetToken() != IDENT) {
		ParseError(line, "Missing END of procedure name.");
		return false;
	}
	// **mismatch check**:
	if (tok.GetLexeme() != currentProcName) {
		ParseError(line, "Procedure name mismatch in closing end identifier.");
		return false;
	}

	// then the semicolon
	tok = Parser::GetNextToken(in, line);
	if (tok.GetToken() != SEMICOL) {
		ParseError(line, "Missing end of procedure semicolon.");
		return false;
	}

	// reset the flag (optional cleanup)
	failureInDeclPart = false;
	return true;
}


// StmtList ::= Stmt { Stmt }
bool StmtList(istream& in, int& line) {
    if (!Stmt(in, line)) {
        ParseError(line, "Syntactic error in statement list.");
        return false;
    }
    LexItem tok = Parser::GetNextToken(in, line);
    while (tok != END && tok != ELSIF && tok != ELSE && tok.GetToken() != DONE) {
        Parser::PushBackToken(tok);
        if (!Stmt(in, line)) {
            ParseError(line, "Syntactic error in statement list.");
            return false;
        }
        tok = Parser::GetNextToken(in, line);
    }
    Parser::PushBackToken(tok);
    return true;
}

// DeclPart ::= DeclStmt { DeclStmt }
bool DeclPart(istream& in, int& line) {
    if (!DeclStmt(in, line)) {
        ParseError(line, "Non-recognizable Declaration Part.");
        return false;
    }
    // if next token is not BEGIN, must be another DeclStmt
    LexItem tok = Parser::GetNextToken(in, line);
    if (tok.GetToken() == BEGIN) {
        Parser::PushBackToken(tok);
        return true;
    }
    Parser::PushBackToken(tok);
    return DeclPart(in, line);
}

// DeclStmt ::= IDENT {, IDENT } : Type [ := Expr ] ;
bool DeclStmt(istream& in, int& line) {
    // collect identifiers
    Ids_List = new queue<string>();
    if (!IdentList(in, line)) {
        ParseError(line, "Incorrect identifiers list in Declaration Statement.");
        return false;
    }
    vector<string> names;
    while (!Ids_List->empty()) {
        names.push_back(Ids_List->front());
        Ids_List->pop();
    }

    // colon
    LexItem t = Parser::GetNextToken(in, line);
    if (t.GetToken() != COLON) {
        Parser::PushBackToken(t);
        ParseError(line, "Incorrect Declaration Statement Syntax.");
        return false;
    }

    // optional CONST
    t = Parser::GetNextToken(in, line);
    if (t.GetToken() == CONST) {
        t = Parser::GetNextToken(in, line);
    }

    // must be a type
    if (t.GetToken() != INT && t.GetToken() != FLOAT &&
        t.GetToken() != STRING && t.GetToken() != BOOL &&
        t.GetToken() != CHAR) {
        ParseError(line, "Incorrect Declaration Type.");
        return false;
    }
    Token typeTok = t.GetToken();
    // record declarations
    for (auto &v : names) {
        SymTable[v] = typeTok;
        defVar[v]    = true;
    }

    // optional range
    Value retVal1, retVal2;
    t = Parser::GetNextToken(in, line);
    if (t.GetToken() == LPAREN) {
        if (!Range(in, line, retVal1, retVal2)) {
            ParseError(line, "Incorrect definition of a range in declaration statement");
            return false;
        }
        t = Parser::GetNextToken(in, line);
        if (t.GetToken() != RPAREN) {
            ParseError(line, "Incorrect syntax for a range in declaration statement");
            return false;
        }
        t = Parser::GetNextToken(in, line);
    }

    // optional initializer
    if (t.GetToken() == ASSOP) {
        Value initVal;
        if (!Expr(in, line, initVal)) {
            ParseError(line, "Incorrect initialization expression.");
            return false;
        }
        for (auto &v : names) {
            TempsResults[v] = initVal;
        }
        t = Parser::GetNextToken(in, line);
    } else {
        Parser::PushBackToken(t);
        t = Parser::GetNextToken(in, line);
    }

    // semicolon
    if (t.GetToken() != SEMICOL) {
        --line;
        ParseError(line, "Missing semicolon at end of statement");
        return false;
    }
    return true;
}

// IdentList ::= IDENT { , IDENT }
bool IdentList(istream& in, int& line) {
    LexItem tok = Parser::GetNextToken(in, line);
    if (tok.GetToken() != IDENT) {
        Parser::PushBackToken(tok);
        // no identifiers is okay
        return true;
    }
    string name = tok.GetLexeme();
    if (defVar[name]) {
        ParseError(line, "Variable Redefinition");
        return false;
    }
    defVar[name] = true;
    Ids_List->push(name);

    tok = Parser::GetNextToken(in, line);
    if (tok.GetToken() == COMMA) {
        return IdentList(in, line);
    }
    Parser::PushBackToken(tok);
    return true;
}

// Stmt ::= AssignStmt | PrintStmts | GetStmt | IfStmt
bool Stmt(istream& in, int& line) {
    LexItem t = Parser::GetNextToken(in, line);
    Parser::PushBackToken(t);

    switch (t.GetToken()) {
        case IDENT: {
            bool ok = AssignStmt(in, line);
            if (!ok) {
                ParseError(line, "Invalid assignment statement.");
                return false;
            }
            return true;
        }

        case IF: {
            bool ok = IfStmt(in, line);
            if (!ok) {
                ParseError(line, "Invalid If statement.");
                return false;
            }
            return true;
        }

        case PUT: case PUTLN: {
            bool ok = PrintStmts(in, line);
            if (!ok) {
                ParseError(line, "Invalid put statement.");
                return false;
            }
            return true;
        }

        case GET: {
            bool ok = GetStmt(in, line);
            if (!ok) {
                ParseError(line, "Invalid get statement.");
                return false;
            }
            return true;
        }

        default:
            return false;
    }
}



//PrintStmts ::= (PutLine | Put) ( Expr ) ;
bool PrintStmts(istream& in, int& line) {
    LexItem t = Parser::GetNextToken(in, line);
    bool isLine = false;
    if (t == PUT) {
        isLine = false;
    }
    else if (t == PUTLN) {
        isLine = true;
    }
    else {
        ParseError(line, "Missing Put or PutLine Keyword");
        return false;
    }

    t = Parser::GetNextToken(in, line);
    if (t != LPAREN) {
        ParseError(line, "Missing Left Parenthesis");
        return false;
    }

    // Evaluate the expression into 'val'
    Value val;
    if (!Expr(in, line, val)) {
        // extra “incorrect operand” before the generic “missing expression”
        ParseError(line, "Incorrect operand");
        ParseError(line, "Missing expression for an output statement");
        return false;
    }

    t = Parser::GetNextToken(in, line);
    if (t != RPAREN) {
        ParseError(line, "Missing Right Parenthesis");
        return false;
    }

    t = Parser::GetNextToken(in, line);
    if (t != SEMICOL) {
        --line;
        ParseError(line, "Missing semicolon at end of statement");
        return false;
    }

    // *** Actual output ***
    std::cout << val;
    if (isLine) std::cout << std::endl;

    return true;
}

// IfStmt ::= IF Expr THEN StmtList { ELSIF Expr THEN StmtList } [ ELSE StmtList ] END IF ;
bool IfStmt(istream& in, int& line) {
    // 1) IF
    LexItem t = Parser::GetNextToken(in, line);
    if (t.GetToken() != IF) {
        ParseError(line, "Missing IF Keyword");
        return false;
    }

    // 2) parse & boolean‐check the condition
    Value cond;
    if (!Expr(in, line, cond) || !cond.IsBool()) {
        ParseError(line, "Invalid expression type for an If condition");
        return false;
    }
    bool takeThen = cond.GetBool();

    // 3) expect THEN
    t = Parser::GetNextToken(in, line);
    if (t.GetToken() != THEN) {
        ParseError(line, "If-Stmt Syntax Error");
        return false;
    }

    if (takeThen) {
        // 4a) RUN the then‐block
        if (!StmtList(in, line)) {
            ParseError(line, "Missing Statement for If-Stmt Then-clause");
            return false;
        }
        // 4b) SKIP everything up to END IF
        do {
            t = Parser::GetNextToken(in, line);
        } while (t.GetToken() != END && t.GetToken() != DONE);
        Parser::PushBackToken(t);
    }
    else {
        bool branchTaken = false;

        // 5) skip the THEN‐block entirely
        do {
            t = Parser::GetNextToken(in, line);
        } while (t.GetToken()!=ELSIF && t.GetToken()!=ELSE && t.GetToken()!=END && t.GetToken()!=DONE);
        Parser::PushBackToken(t);

        // 6) zero or more ELSIF clauses
        while (true) {
            t = Parser::GetNextToken(in, line);
            if (t.GetToken() != ELSIF) {
                Parser::PushBackToken(t);
                break;
            }
            // read the condition
            Value elifVal;
            if (!Expr(in, line, elifVal) || !elifVal.IsBool()) {
                ParseError(line, "Invalid expression type for an Elsif condition");
                return false;
            }
            t = Parser::GetNextToken(in, line);
            if (t.GetToken() != THEN) {
                ParseError(line, "Elsif-Stmt Syntax Error");
                return false;
            }
            // execute only the first true branch
            if (!branchTaken && elifVal.GetBool()) {
                if (!StmtList(in, line)) {
                    ParseError(line, "Missing Statement for If-Stmt Else-If-clause");
                    return false;
                }
                branchTaken = true;
            } else {
                // skip this clause’s statements
                do {
                    t = Parser::GetNextToken(in, line);
                } while (t.GetToken()!=ELSIF && t.GetToken()!=ELSE && t.GetToken()!=END && t.GetToken()!=DONE);
                Parser::PushBackToken(t);
            }
        }

        // 7) optional ELSE
        t = Parser::GetNextToken(in, line);
        if (t.GetToken() == ELSE) {
            if (!branchTaken) {
                if (!StmtList(in, line)) {
                    ParseError(line, "Missing Statement for If-Stmt Else-clause");
                    return false;
                }
            } else {
                // skip else‐block if we already took a branch
                do {
                    t = Parser::GetNextToken(in, line);
                } while (t.GetToken()!=END && t.GetToken()!=DONE);
                Parser::PushBackToken(t);
            }
        } else {
            Parser::PushBackToken(t);
        }
    }

    // 8) consume END IF ;
    t = Parser::GetNextToken(in, line);
    if (t.GetToken() != END) {
        ParseError(line, "Missing closing END IF for If-statement.");
        return false;
    }
    t = Parser::GetNextToken(in, line);
    if (t.GetToken() != IF) {
        ParseError(line, "Missing closing END IF for If-statement.");
        return false;
    }
    t = Parser::GetNextToken(in, line);
    if (t.GetToken() != SEMICOL) {
        --line;
        ParseError(line, "Missing semicolon at end of statement");
        return false;
    }
    return true;
}


// GetStmt ::= GET ( Var ) ;
bool GetStmt(istream& in, int& line) {
    // 1) GET
    LexItem t = Parser::GetNextToken(in, line);
    if (t.GetToken() != GET) {
        ParseError(line, "Missing Get Keyword");
        return false;
    }
    // 2) (
    t = Parser::GetNextToken(in, line);
    if (t.GetToken() != LPAREN) {
        ParseError(line, "Missing Left Parenthesis");
        return false;
    }
    // 3) Var
    LexItem idtok;
    if (!Var(in, line, idtok)) {
        ParseError(line, "Missing a variable for an input statement");
        return false;
    }
    // 4) )
    t = Parser::GetNextToken(in, line);
    if (t.GetToken() != RPAREN) {
        ParseError(line, "Missing Right Parenthesis");
        return false;
    }
    // 5) ;
    t = Parser::GetNextToken(in, line);
    if (t.GetToken() != SEMICOL) {
        --line;
        ParseError(line, "Missing semicolon at end of statement");
        return false;
    }

    // === 6) ACTUAL INPUT: read from cin and store into TempsResults ===
    {
        const string varName = idtok.GetLexeme();
        Token varType = SymTable[varName];
        if (varType == INT) {
            int v;
            std::cin >> v;
            TempsResults[varName] = Value(v);
        }
        else if (varType == FLOAT) {
            double v;
            std::cin >> v;
            TempsResults[varName] = Value(v);
        }
        else if (varType == STRING) {
            // read a single word (no spaces)
            string s;
            std::cin >> s;
            TempsResults[varName] = Value(s);
        }
        else if (varType == CHAR) {
            char c;
            std::cin >> c;
            TempsResults[varName] = Value(c);
        }
        else if (varType == BOOL) {
            string tok;
            std::cin >> tok;
            bool b = (tok == "true" || tok == "TRUE");
            TempsResults[varName] = Value(b);
        }
        else {
            ParseError(line, "Illegal input type for variable in GET");
            return false;
        }
    }

    return true;
}
// AssignStmt ::= Var := Expr ;
bool AssignStmt(istream& in, int& line)
{
    inAssignStmt = true;

    // 1) capture the LHS variable (only decl‐check, no uninit here)
    LexItem idtok;
    if (!Var(in, line, idtok)) {
        ParseError(line, "Missing Left-Hand Side Variable in Assignment statement");
        inAssignStmt = false;
        return false;
    }

    // 2) expect :=
    LexItem t = Parser::GetNextToken(in, line);
    if (t.GetToken() != ASSOP) {
        ParseError(line, "Missing Assignment Operator");
        inAssignStmt = false;
        return false;
    }

    // 3) parse RHS
    Value rhs;
    bool ex = Expr(in, line, rhs);
    if (!ex) {
        // 3a) if the LHS was never initialized, report it now
        if (TempsResults.find(idtok.GetLexeme()) == TempsResults.end()) {
            ParseError(line, "Invalid use of an unintialized variable.");
            // 3b) now emit the operand‐error
            ParseError(line, "Incorrect operand");
        }
        // 3c) lastly the generic missing‐expr
        ParseError(line, "Missing Expression in Assignment Statement");
        inAssignStmt = false;
        return false;
    }

    // 4) type‐check (must exactly match)
    Token declType = SymTable[idtok.GetLexeme()];
    bool typeOK = (declType == INT   && rhs.IsInt())
               || (declType == FLOAT && rhs.IsReal())
               || (declType == STRING&& rhs.IsString())
               || (declType == CHAR  && rhs.IsChar())
               || (declType == BOOL  && rhs.IsBool());
    if (!typeOK) {
        ParseError(line, "Illegal Expression type for the assigned variable");
        inAssignStmt = false;
        return false;
    }

    // 5) do the store
    TempsResults[idtok.GetLexeme()] = rhs;

    // 6) consume ;
    t = Parser::GetNextToken(in, line);
    if (t.GetToken() != SEMICOL) {
        ParseError(line, "Missing semicolon at end of statement");
        inAssignStmt = false;
        return false;
    }

    inAssignStmt = false;
    return true;
}

// Var ::= IDENT
bool Var(istream& in, int& line, LexItem & idtok) {
    LexItem tok = Parser::GetNextToken(in, line);
    if (tok.GetToken() == IDENT) {
        idtok = tok;
        string name = tok.GetLexeme();
        if (!defVar[name]) {
            ParseError(line, "Undeclared Variable");
            return false;
        }
        return true;
    }
    if (tok.GetToken() == ERR) {
        ParseError(line, "Unrecognized Input Pattern");
        cout << "(" << tok.GetLexeme() << ")\n";
        return false;
    }
    return false;
}

// Expr ::= Relation { ( AND | OR ) Relation }
bool Expr(istream& in, int& line, Value & retVal) {
    if (!Relation(in, line, retVal)) return false;
    LexItem tok = Parser::GetNextToken(in, line);
    while (tok.GetToken() == AND || tok.GetToken() == OR) {
        if (!Relation(in, line, retVal)) {
            ParseError(line, "Missing operand after operator");
            return false;
        }
        tok = Parser::GetNextToken(in, line);
    }
    Parser::PushBackToken(tok);
    return true;
}

// Relation ::= SimpleExpr [ ( = | /= | < | <= | > | >= ) SimpleExpr ]
bool Relation(istream& in, int& line, Value & retVal) {
    // 1) parse the left‐hand simple expression
    Value leftVal;
    if (!SimpleExpr(in, line, leftVal)) return false;

    // 2) check if there's a relational operator
    LexItem opTok = Parser::GetNextToken(in, line);
    if (opTok.GetToken() == EQ || opTok.GetToken() == NEQ ||
        opTok.GetToken() == LTHAN || opTok.GetToken() == GTHAN ||
        opTok.GetToken() == LTE || opTok.GetToken() == GTE) {

        // 3) parse the right‐hand simple expression
        Value rightVal;
        if (!SimpleExpr(in, line, rightVal)) {
            ParseError(line, "Missing operand after operator");
            return false;
        }

        // 4) perform the comparison and produce a Boolean Value
        switch (opTok.GetToken()) {
            case EQ:   retVal = leftVal == rightVal; break;
            case NEQ:  retVal = leftVal != rightVal; break;
            case LTHAN:retVal = leftVal <  rightVal; break;
            case LTE:  retVal = leftVal <= rightVal; break;
            case GTHAN:retVal = leftVal >  rightVal; break;
            case GTE:  retVal = leftVal >= rightVal; break;
            default:   /* unreachable */           break;
        }

    } else {
        // no relational operator: push it back and return the numeric/string result
        Parser::PushBackToken(opTok);
        retVal = leftVal;
    }

    return true;
}

//SimpleExpr ::= STerm { ( + | - | & ) STerm }
bool SimpleExpr(istream& in, int& line, Value & retVal) {
    // 1) parse the first term
    Value left;
    if (!STerm(in, line, left)) return false;
    retVal = left;

    // 2) loop over any +, - or concatenation
    LexItem tok = Parser::GetNextToken(in, line);
    while (tok.GetToken() == PLUS || tok.GetToken() == MINUS || tok.GetToken() == CONCAT) {
        // parse the next term
        Value right;
        if (!STerm(in, line, right)) {
            ParseError(line, "Missing operand after operator");
            return false;
        }

        // 3) if it’s + or -, enforce same‐type numeric operands
        if (tok.GetToken() == PLUS || tok.GetToken() == MINUS) {
            bool bothInt   = left.IsInt()  && right.IsInt();
            bool bothReal  = left.IsReal() && right.IsReal();
            if (!(bothInt || bothReal)) {
                ParseError(line, "Illegal operand type for the operation.");
                return false;
            }
            // perform the arithmetic
            retVal = (tok.GetToken() == PLUS)
                     ? (left + right)
                     : (left - right);
        }
        // 4) handle concatenation (&) for strings/chars
        else if (tok.GetToken() == CONCAT) {
            retVal = left.Concat(right);
        }

        // 5) shift for next iteration
        left = retVal;
        tok = Parser::GetNextToken(in, line);
    }

    // 6) push back the token that broke the loop
    Parser::PushBackToken(tok);
    return true;
}

// STerm ::= [ ( + | - | NOT ) ] Term
//STerm ::= [( - | + )] Term
bool STerm(istream& in, int& line, Value & retVal)
{
    // 1) grab optional sign
    LexItem t = Parser::GetNextToken(in, line);
    int sign = 0;
    if (t.GetToken() == MINUS)      sign = -1;
    else if (t.GetToken() == PLUS)  sign = +1;
    else                             Parser::PushBackToken(t);

    // 2) parse the rest of the term
    if (!Term(in, line, sign, retVal))
        return false;

    // 3) if there *was* a sign, only INT or REAL are legal
    if (sign != 0 && !(retVal.IsInt() || retVal.IsReal())) {
        ParseError(line, "Illegal Operand Type for Sign Operator");
        ParseError(line, "Incorrect operand");
        return false;
    }

    return true;
}


// Term ::= Factor {( * | / | MOD) Factor}
bool Term(istream& in, int& line, int sign, Value & retVal) {
    // 1) parse first factor
    Value left;
    if (!Factor(in, line, sign, left)) return false;
    retVal = left;

    // 2) operator‐factor loop
    LexItem tok = Parser::GetNextToken(in, line);
    while (tok == MULT || tok == DIV || tok == MOD) {
        // parse right operand
        Value right;
        if (!Factor(in, line, sign, right)) {
            // Factor already printed its error
            return false;
        }

        // 3a) division or multiply
        if (tok == MULT || tok == DIV) {
            // must be same numeric type
            bool bothInt  = left.IsInt()  && right.IsInt();
            bool bothReal = left.IsReal() && right.IsReal();
            if (!(bothInt || bothReal)) {
                ParseError(line, "Illegal operand type for the operation.");
                return false;
            }
            // **runtime** check: division by zero
            if (tok == DIV) {
                if (bothInt) {
                    if (right.GetInt() == 0) {
                        ParseError(line, "Run-Time Error-Illegal division by Zero");
                        return false;
                    }
                } else {
                    if (right.GetReal() == 0.0) {
                        ParseError(line, "Run-Time Error-Illegal division by Zero");
                        return false;
                    }
                }
            }
            // perform the op
            retVal = (tok == MULT) ? (left * right) : (left / right);
        }
        // 3b) modulus
        else {
            if (!(left.IsInt() && right.IsInt())) {
                ParseError(line, "Illegal operand type for the operation.");
                return false;
            }
            if (right.GetInt() == 0) {
                ParseError(line, "Run-Time Error-Illegal division by Zero");
                return false;
            }
            retVal = left % right;
        }

        // 4) shift and continue
        left = retVal;
        tok = Parser::GetNextToken(in, line);
    }

    Parser::PushBackToken(tok);
    return true;
}



// Factor ::= Primary [ ** Primary ] | NOT Primary
bool Factor(istream& in, int& line, int sign, Value & retVal) {
    LexItem tok = Parser::GetNextToken(in, line);
    if (tok.GetToken() == NOT) {
        if (!Primary(in, line, sign, retVal)) {
            ParseError(line, "Incorrect operand for NOT operator");
            return false;
        }
        // apply the NOT
        retVal = !retVal;
        return true;
    }
    Parser::PushBackToken(tok);

    // if the primary fails, just return false—no extra error here
    if (!Primary(in, line, sign, retVal)) {
        return false;
    }

    // exponentiation
    tok = Parser::GetNextToken(in, line);
    if (tok.GetToken() == EXP) {
        Value exponent;
        if (!Primary(in, line, sign, exponent)) {
            ParseError(line, "Missing raised power for exponent operator");
            return false;
        }
        // only floats allowed
        if (!retVal.IsReal() || !exponent.IsReal()) {
            ParseError(line, "Illegal operand type for the operation.");
            return false;
        }
        retVal = retVal.Exp(exponent);
    } else {
        Parser::PushBackToken(tok);
    }
    return true;
}




// Primary ::= Name | ICONST | FCONST | SCONST | BCONST | CCONST | ( Expr )
bool Primary(istream& in, int& line, int sign, Value & retVal) {
    LexItem tok = Parser::GetNextToken(in, line);
    switch (tok.GetToken()) {
        case IDENT:
			Parser::PushBackToken(tok);
			return Name(in, line, sign, retVal);
        case ICONST:
            retVal = Value(stoi(tok.GetLexeme()));
            return true;
        case FCONST:
            retVal = Value(stod(tok.GetLexeme()));
            return true;
        case SCONST:
            retVal = Value(tok.GetLexeme());
            return true;
        case BCONST:
            retVal = Value(tok.GetLexeme() == "true");
            return true;
        case CCONST:
            retVal = Value(tok.GetLexeme()[0]);
            return true;
        case LPAREN:
            if (!Expr(in, line, retVal)) {
                ParseError(line, "Invalid expression after left parenthesis");
                return false;
            }
            if (Parser::GetNextToken(in, line).GetToken() != RPAREN) {
                ParseError(line, "Missing right parenthesis after expression");
                return false;
            }
            return true;
        default:
            ParseError(line, "Invalid Expression");
            return false;
    }
}

// Name ::= IDENT [ ( Range ) ]
bool Name(istream& in, int& line, int sign, Value & retVal)
{
    // 1) read the identifier
    LexItem tok = Parser::GetNextToken(in, line);
    if (tok.GetToken() != IDENT) {
        ParseError(line, "Invalid reference to a variable.");
        return false;
    }
    string nm = tok.GetLexeme();

    // 2) must have been declared
    if (!defVar[nm]) {
        ParseError(line, "Using Undefined Variable");
        return false;
    }

    // 3) must have been initialized before use
    if (TempsResults.find(nm) == TempsResults.end()) {
        ParseError(line, "Invalid use of an unintialized variable.");
        return false;
    }

    // 4) fetch its current value
    Value baseVal = TempsResults[nm];

    // 5) check for substring/index syntax
    tok = Parser::GetNextToken(in, line);
    if (tok.GetToken() == LPAREN) {
        // parse the two bounds into loVal and hiVal
        Value loVal, hiVal;
        if (!Range(in, line, loVal, hiVal)) {
            // Range already emitted its own error
            return false;
        }
        // expect the closing ')'
        if (Parser::GetNextToken(in, line).GetToken() != RPAREN) {
            ParseError(line, "Invalid syntax for an index or range definition.");
            return false;
        }

        // must be a string variable
        if (!baseVal.IsString()) {
            ParseError(line, "Invalid range operation for non-string variable.");
            return false;
        }
        string s = baseVal.GetString();
        int len = (int)s.size();
        int lo = loVal.GetInt(), hi = hiVal.GetInt();

        // out‑of‑bounds?
        if (lo < 0 || hi >= len) {
            if (lo == hi) {
                ParseError(line, "Out of range index value.");
            } else {
                ParseError(line, "Invalid lowerbound or upperbound value of a range.");
            }
            return false;
        }

        // single‑character
        if (lo == hi) {
            retVal = Value(s[lo]);
        } else {
            retVal = Value(s.substr(lo, hi - lo + 1));
        }
        return true;
    }

    // 6) not a range: push back and just return the variable’s value
    Parser::PushBackToken(tok);
    retVal = baseVal;
    return true;
}


// Range ::= SimpleExpr [.. SimpleExpr]
bool Range(istream& in, int& line, Value & loVal, Value & hiVal)
{
    // parse lower bound
    if (!SimpleExpr(in, line, loVal)) {
        ParseError(line, "Invalid expression for a lower bound definition of a range.");
        return false;
    }
    if (!loVal.IsInt()) {
        ParseError(line, "Invalid lowerbound or upperbound value of a range.");
        return false;
    }

    // check for dot-dot
    LexItem tok = Parser::GetNextToken(in, line);
    if (tok.GetToken() == DOT) {
        // expect second '.'
        tok = Parser::GetNextToken(in, line);
        if (tok.GetToken() != DOT) {
            ParseError(line, "Invalid definition of a range.");
            return false;
        }
        // parse upper bound
        if (!SimpleExpr(in, line, hiVal)) {
            ParseError(line, "Invalid expression for an upper bound definition of a range.");
            return false;
        }
        if (!hiVal.IsInt()) {
            ParseError(line, "Invalid lowerbound or upperbound value of a range.");
            return false;
        }
        int lo = loVal.GetInt(), hi = hiVal.GetInt();
        if (lo > hi) {
            ParseError(line, "Invalid lowerbound or upperbound value of a range.");
            return false;
        }
        return true;
    }

    // single‐index case: push back the token and copy loVal→hiVal
    Parser::PushBackToken(tok);
    hiVal = loVal;
    return true;
}
