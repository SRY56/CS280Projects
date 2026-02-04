#include "lex.h"
#include <cstring>

/**
 * This function is used to check whether the given character is alpha numeric,
 * 0-9, a-z, A-Z, _
 */
bool is_alpha_numeric(char c) noexcept {
	switch (c) {
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case '_':
		return true;
	default:
		return false;
	}
}

/**
 * This function is used to check whether
 * given character is space or \t or \r
 */
bool is_space(char c)  {
	switch (c) {
	case ' ':
	case '\t':
	case '\r':
		return true;
	default:
		return false;
	}
}


/**
 * This function is used to check whether
 * given character is numeric 0 to 9
 */
bool is_numeric(char c)  {
	switch (c) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		return true;
	default:
		return false;
	}
}

/**
 * This function is used to whether the current stream is
 * a valid identifier of keyword in SADAL language,
 * the allowed ids or keywords are mentioned in the
 * requirement document
 */
LexItem identifier(istream& in, int& linenum)  {
	string sequence;
	char inp = in.get();
	char prev = '\0';
	sequence += inp;
	// loop until we find alpha numeric chars
	while (is_alpha_numeric(in.peek())){
		inp = in.get();
		//if we happen to find consecutive underscore, then it may be an error
		if (inp == '_' && prev == '_') {
			in.putback(inp);
			break;
		}
		sequence+= inp;
		prev = inp;
	}
	//convert to lower case , as its important to print in lowers
	string lower ="";
	for(auto letter : sequence)
		lower += std::tolower(letter);
	return id_or_kw(lower,linenum);
}

/**
 * Function used to check whether the current stream is
 * a string constant , starts with a double quote and ends with
 * a double quote.
 */
LexItem stringconst(istream& in, int& linenum)  {
	string sequence;
	char inp = in.get();
	sequence += inp;
	//loop until we find a new line or eof or double quotes
	while (in.peek()!= '"' && in.peek()!= '\n' && !in.eof()){
		inp = in.get();
		sequence+= inp;
	}
	sequence +=in.get();
	char last = sequence[sequence.length()-1];
	if (last !='"') { // if last char is not a double quote, then its invalid
		sequence[sequence.length()-1] = '\0';
		return LexItem(Token::ERR, " Invalid string constant "+sequence,linenum);
	}
	return LexItem(Token::SCONST, sequence,linenum);
}

/**
 * Function used to check whether the current stream is
 * a char constant , starts with a single quote and ends with
 * a single quote.
 * only one char allowed, new line is not allowed
 */
LexItem charconst(istream& in, int& linenum)  {
	string sequence;
	char inp = in.get();
	inp = in.get();
	if (inp == '\n'){ // new line is invalid
		return LexItem(Token::ERR, "New line is an invalid character constant.",linenum);
	}
	sequence += inp;
	inp = in.get();
	if (inp !='\''){ // if last char is not a single quote, then its invalid
		sequence +=inp;
		string msg = "Invalid character constant '" + sequence + "'";
		return LexItem(Token::ERR, msg,linenum);
	}
	return LexItem(Token::CCONST, sequence,linenum);
}

/**
 * Function used to check whether the current stream is
 * a numeric constant , start with a numeric and ends with
 * a numeric, only one dot is allowed and exp is allowed, with -, +
 * only one char allowed, new line is not allowed
 */
LexItem number(istream& in,int& linenum)  {
	string sequence;
	sequence += in.get();
	bool fconst = false;
	int dotCount = 0;
	// loop through the stream until we find 0-9, - , +, e, E, .
	while (is_numeric(in.peek()) || in.peek() == '.'
			|| in.peek() == 'E' || in.peek() == 'e' || in.peek() == '-' || in.peek() == '+'){
		if (in.peek() == '.') {
			fconst = true;
		}
		sequence +=in.get();
	}
	//this block of code will remove 'e' or 'E' at the end of accumulated sequence
	//and put back into the stream
	int ss = sequence.length();
	if (sequence[ss-1] == 'e' || sequence[ss-1] == 'E') {
		char last = sequence[ss-1];
		sequence[ss-1] = '\0';
		in.putback(last);
	}

	//this block of code will remove '.' at the end of accumulated sequence
	//and put back into the stream
	string temp = sequence;
	while(true) {
		int initial = sequence.length();
		if (sequence[initial-1] == '.') {
			sequence[initial-1] = '\0';
			sequence.resize(initial-1);
			in.putback('.');
		}
		else
			break;
		fconst = false;
	}

	//this block of code will analyze ,sequence has got more than one dot
	//and if so, return error
	string errseq;
	for (int k=0; k <sequence.length(); k++) {
		errseq += sequence[k];
		if (sequence[k] == '.') {
			if (dotCount>=1) {
				return LexItem(Token::ERR, errseq, linenum);
			}
			dotCount++;
		}
	}

	//if we reach here, its assumed that the sequence is a valid fconst or iconst
	if (fconst) {
		return LexItem(Token::FCONST, sequence, linenum);
	}
	else
		return LexItem(Token::ICONST,sequence, linenum);
}

/**
 * Function used to handle the *, which may be *
 * or **
 */
LexItem handle_star(istream &in, int& linenum) {
	char inp = in.get();
	char next = in.peek();
	if (next == '*' && inp =='*') {
		in.get();
		return LexItem(Token::EXP, string("EXP"), linenum);
	}
	return LexItem(Token::MULT, string("MULT"), linenum);;
}

/**
 * Function used to handle the <, which may be <
 * or <=
 */
LexItem handle_lessthan(istream &in, int& linenum) {
	char first = in.get();
	char next = in.peek();
	if (next == '=' && first =='<') {
		in.get();
		return LexItem(Token::LTE, string("LTE"), linenum);
	}
	return LexItem(Token::LTHAN, string("LTHAN"), linenum);;
}

/**
 * Function used to handle the >, which may be >
 * or >=
 */
LexItem handle_greaterthan(istream &in, int& linenum) {
	char first = in.get();
	char next = in.peek();
	if (next == '=' && first =='>') {
		in.get();
		return LexItem(Token::GTE, string("GTE"), linenum);
	}
	return LexItem(Token::GTHAN, string("GTHAN"), linenum);;
}

/**
 * Function used to handle the comments or minus
 */
LexItem handle_comments(istream &in, int& linenum) {
	char first = in.peek();
	char next = in.get();
	string sequence = "";
	//consecutive - signifies ,its a comment
	if (next == '-' && first =='-') {
		next = in.get();
		//consume all the letters
		while(next != '\n') {
			sequence +=next;
			next = in.get();

		}
		linenum++;
		return LexItem(Token::SCONST, sequence, linenum);;
	}
	in.putback(next); // we put back excess char here
	return LexItem(Token::MINUS, string("MINUS"), linenum);;
}

/**
 * Function used to handle the colon, which may be colon
 * or ASSOP
 */
LexItem handle_colon(istream &in, int& linenum) {
	char first = in.get();
	char next = in.peek();
	if (next == '=' && first ==':') {
		in.get();
		return LexItem(Token::ASSOP, string("ASSOP"), linenum);
	}
	return LexItem(Token::COLON, string("COLON"), linenum);;
}

/**
 * Function used to handle the slash, which may be divide
 * or NEQ
 */
LexItem handle_slash(istream &in, int& linenum) {
	char first = in.get();
	char next = in.peek();
	if (next == '=' && first =='/') {
		in.get();
		return LexItem(Token::NEQ, string("NEQ"), linenum);
	}
	return LexItem(Token::DIV, string("DIV"), linenum);;
}

/**
 * Function used to retrieve the next token from the stream and
 * construct the LexItem accordingly, handles the error lex items also
 */
LexItem getNextToken(istream& in, int& linenum) {
	char input;
	// this loop is used to handle the spaces or new lines and comments
	while (is_space(in.peek()) || in.peek() == '\n' || in.peek() == '-') {
		input = in.get();
		//in case a new line, just increment the count
		if (input == '\n')
			linenum++;
		// hiphen signifies either a comment or minus
		if (input == '-') {
			LexItem item = handle_comments(in, linenum);
			// we just ignore the comment string here, in order to distinguish
			// comment from a minus symbol, we do so
			if (item.GetToken() == Token::SCONST)
				continue;
			else
				return item;
		}
	}

	// at the end of file, we happen to find a -1 , which signifies done
	int val = (int) in.peek();
	if (val == -1) {
		in.get();
		return LexItem(Token::DONE , "DONE", linenum);
	}
	//handle comma
	if (in.peek() == ',') {
		in.get();
		return LexItem(Token::COMMA , "COMMA", linenum);
	}

	//handle others
	switch (in.peek()) {
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
		return identifier(in, linenum);
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		return number(in,linenum);
	case '(':
		in.get();
		return LexItem(Token::LPAREN , "LPAREN", linenum);
	case ')':
		in.get();
		return LexItem(Token::RPAREN , "RPAREN", linenum);
	case '<':
		return handle_lessthan(in, linenum);
	case '>':
		return handle_greaterthan(in, linenum);
	case '=':
		in.get();
		return LexItem(Token::EQ , "EQ", linenum);
	case '+':
		in.get();
		return LexItem(Token::PLUS, "PLUS", linenum);
	case '&':
		in.get();
		return LexItem(Token::CONCAT, "CONCAT", linenum);
	case '*':
		return handle_star(in,linenum);
	case '.':
		in.get();
		return LexItem(Token::DOT , "DOT", linenum);
	case '"':
		return stringconst(in,linenum);
	case '\'':
		return charconst(in,linenum);
	case ':':
		return handle_colon(in, linenum);
	case '/':
		return handle_slash(in, linenum);
	case ';':
		in.get();
		return LexItem(Token::SEMICOL , "SEMICOL", linenum);
	default:
		string lexeme;
		lexeme.push_back(in.peek());
		return LexItem(Token::ERR , lexeme, linenum);
	}
}

/**
 * Overloaded Function used to print the given token
 */
ostream& operator<<(ostream& out, const LexItem& tok) {
	if (tok.GetToken() == Token::ICONST) {
		out << "ICONST: (" << tok.GetLexeme() << ")" <<endl;
	}
	else if (tok.GetToken() == Token::BCONST) {
		out << "BCONST: (" << tok.GetLexeme() << ")" <<endl;
	}
	else if (tok.GetToken() == Token::FCONST) {
		out << "FCONST: (" << tok.GetLexeme() << ")" <<endl;
	}
	else if (tok.GetToken() == Token::IDENT) {
		out << "IDENT: <" << tok.GetLexeme() << ">" <<endl;
	}
	else if (tok.GetToken() == Token::SCONST) {
		out << "SCONST: " << tok.GetLexeme() << endl;
	}
	else if (tok.GetToken() == Token::CCONST) {
		out << "CCONST: '" << tok.GetLexeme() << "'" <<endl;
	}
	else if (tok.GetToken() == Token::IF) {
		out << "IF"<<endl;
	}
	else if (tok.GetToken() == Token::ELSE) {
		out << "ELSE"<<endl;
	}
	else if (tok.GetToken() == Token::ELSIF) {
		out << "ELSIF"<<endl;
	}
	else if (tok.GetToken() == Token::PUT) {
		out << "PUT"<<endl;
	}
	else if (tok.GetToken() == Token::PUTLN) {
		out << "PUTLN"<<endl;
	}
	else if (tok.GetToken() == Token::INT) {
		out << "INT"<<endl;
	}
	else if (tok.GetToken() == Token::CHAR) {
		out << "CHAR"<<endl;
	}
	else if (tok.GetToken() == Token::FLOAT) {
		out << "FLOAT"<<endl;
	}
	else if (tok.GetToken() == Token::STRING) {
		out << "STRING"<<endl;
	}
	else if (tok.GetToken() == Token::BOOL) {
		out << "BOOL"<<endl;
	}
	else if (tok.GetToken() == Token::PROCEDURE) {
		out << "PROCEDURE"<<endl;
	}
	else if (tok.GetToken() == Token::END) {
		out << "END"<<endl;
	}
	else if (tok.GetToken() == Token::IS) {
		out << "IS"<<endl;
	}
	else if (tok.GetToken() == Token::BEGIN) {
		out << "BEGIN"<<endl;
	}
	else if (tok.GetToken() == Token::GET) {
		out << "GET"<<endl;
	}
	else if (tok.GetToken() == Token::CONST) {
		out << "CONST"<<endl;
	}
	else if (tok.GetToken() == Token::MOD) {
		out << "MOD"<<endl;
	}
	else if (tok.GetToken() == Token::AND) {
		out << "AND"<<endl;
	}
	else if (tok.GetToken() == Token::OR) {
		out << "OR"<<endl;
	}
	else if (tok.GetToken() == Token::NOT) {
		out << "NOT"<<endl;
	}
	else if (tok.GetToken() == Token::THEN) {
		out << "THEN"<<endl;
	}
	else if (tok.GetToken() == Token::DONE) {
		out << "DONE"<<endl;
	}
	else if (tok.GetToken() == Token::ERR) {
		out << "ERR: In line "<<tok.GetLinenum()+1<< ", Error Message {" + tok.GetLexeme() + "}";
	}
	else{
		out << tok.GetLexeme()  <<endl;
	}
	return out;
}

/**
 * Function used to check whether the given lexeme is
 * an id or keyword
 */
LexItem id_or_kw(const string& lexeme, int linenum) {
	if (lexeme == "if") {
		return LexItem(Token::IF,"if", linenum);
	}
	else if (lexeme == "else") {
		return LexItem(Token::ELSE,"else", linenum);
	}
	else if (lexeme == "elsif") {
		return LexItem(Token::ELSIF,"ELSIF", linenum);
	}
	else if (lexeme == "put") {
		return LexItem(Token::PUT,"put", linenum);
	}
	else if (lexeme == "putline") {
		return LexItem(Token::PUTLN,"putline", linenum);
	}
	else if (lexeme == "integer") {
		return LexItem(Token::INT,"integer", linenum);
	}
	else if (lexeme == "character") {
		return LexItem(Token::CHAR,"char", linenum);
	}
	else if (lexeme == "float") {
		return LexItem(Token::FLOAT,"float", linenum);
	}
	else if (lexeme == "string") {
		return LexItem(Token::STRING,"string", linenum);
	}
	else if (lexeme == "boolean") {
		return LexItem(Token::BOOL,"boolean", linenum);
	}
	else if (lexeme == "procedure") {
		return LexItem(Token::PROCEDURE,"procedure", linenum);
	}
	else if (lexeme == "end") {
		return LexItem(Token::END,"end", linenum);
	}
	else if (lexeme == "is") {
		return LexItem(Token::IS,"is", linenum);
	}
	else if (lexeme == "begin") {
		return LexItem(Token::BEGIN,"begin", linenum);
	}
	else if (lexeme == "get") {
		return LexItem(Token::GET,"get", linenum);
	}
	else if (lexeme == "true") {
		return LexItem(Token::BCONST,"true", linenum);
	}
	else if (lexeme == "false") {
		return LexItem(Token::BCONST,"false", linenum);
	}
	else if (lexeme == "constant") {
		return LexItem(Token::CONST,"const", linenum);
	}
	else if (lexeme == "mod") {
		return LexItem(Token::MOD,"mod", linenum);
	}
	else if (lexeme == "and") {
		return LexItem(Token::AND,"and", linenum);
	}
	else if (lexeme == "or") {
		return LexItem(Token::OR,"or", linenum);
	}
	else if (lexeme == "not") {
		return LexItem(Token::NOT,"not", linenum);
	}
	else if (lexeme == "then") {
		return LexItem(Token::THEN,"then", linenum);
	}
	else if (lexeme == "done") {
		return LexItem(Token::DONE,"done", linenum);
	}
	else{
		return LexItem(Token::IDENT,lexeme, linenum);
	}
}
