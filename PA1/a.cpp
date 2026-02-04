#include "lex.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <bits/stdc++.h>

using namespace std;

//DRIVER CODE
int main(int argc, char **argv) {

	//declare all the local variables required
	std::ifstream inputFile;
	vector<string> args;
	bool printAll = false;
	bool printString = false;
	bool printNum = false;
	bool printId = false;
	bool printKW = false;
	int fileNameCount = 0;
	string fileName;

	int lineNumber =0;
	int totalTokens = 0;
	int totalNumerals = 0;
	int totalStrings = 0;
	int totalIds = 0;
	int totalKeywords = 0;

	vector<string> keywords;
	vector<float> numerals;
	vector<string> strings;
	vector<string> ids;
	bool isError = false;

	//check arguments
	if (argc == 1) {
		std::cout<<"No specified input file."<<endl;
		return 0;
	}
	inputFile.open(argv[1]);
	if (!inputFile.is_open()) {
		std::cout <<"CANNOT OPEN THE FILE "<<argv[1] <<endl;
		return 0;
	}
	if (inputFile.peek() == EOF) {
        std::cout << "Empty file." << std::endl;
        return -1;
    } else {
        inputFile.clear();
        inputFile.seekg(0, ios::beg);
    }

	for (int i = 1; i < argc; i++) {
		string arugment = argv[i];
		if (arugment[0] == '-') {
			if (arugment == "-all") {
				printAll = true;
			} else if (arugment == "-num") {
				printNum = true;
			} else if (arugment == "-str") {
				printString = true;
			} else if (arugment == "-id") {
				printId = true;
			} else if (arugment == "-kw") {
				printKW = true;
			} else {
				std::cout << "Unrecognized flag {" << arugment << "}" << endl;
				return 0;
			}
		} else if (argv[i][0] != '-') {
			fileNameCount++;
            fileName = argv[i];
            if (fileNameCount > 1) {
                cout << "Only one file name is allowed." << endl;
                return -1;
            }
		}
	}
	basic_istream<char> &in= inputFile;

	//iterate through the stream
	while(!in.eof()) {
		LexItem lexItem = getNextToken(in,lineNumber);
		//if we get error , break the loop and exit
		if (lexItem.GetToken() == Token::ERR) {
			std::cout<< lexItem;
			isError = true;
			break;
		}
		//if we are done, then exit
		if (lexItem.GetToken() == Token::DONE) {
			break;
		}

		totalTokens++;
		if (lexItem.GetToken() != Token::DONE && printAll)
			std::cout<< lexItem;

		if (lexItem.GetToken() == Token::ICONST) {
			auto itemFound = std::find(numerals.begin(), numerals.end(), stof(lexItem.GetLexeme()));
			if (itemFound == numerals.end()) {
				numerals.push_back(stof(lexItem.GetLexeme()));
				totalNumerals++;
			}
		}

		if (lexItem.GetToken() == Token::FCONST) {
			auto itemFound = std::find(numerals.begin(), numerals.end(), stof(lexItem.GetLexeme()));
			if (itemFound == numerals.end()) {
				numerals.push_back(stof(lexItem.GetLexeme()));
				totalNumerals++;
			}
		}
		if (lexItem.GetToken() == Token::SCONST ) {
			strings.push_back(lexItem.GetLexeme());
			totalStrings++;
		}
		if (lexItem.GetToken() == Token::CCONST) {
			string char_const = "\""+ lexItem.GetLexeme() + "\"";
			strings.push_back(char_const);
			totalStrings++;
		}
		if (lexItem.GetToken() == Token::IDENT) {
			auto itemFound = std::find(ids.begin(), ids.end(), lexItem.GetLexeme());
			if (itemFound == ids.end()) {
				ids.push_back(lexItem.GetLexeme());
				totalIds++;
			}
		}
		if (lexItem.GetToken() == Token::IF
				|| lexItem.GetToken() == Token::ELSE
				|| lexItem.GetToken() == Token::ELSIF
				|| lexItem.GetToken() == Token::PUT
				|| lexItem.GetToken() == Token::PUTLN
				|| lexItem.GetToken() == Token::GET
				|| lexItem.GetToken() == Token::INT
				|| lexItem.GetToken() == Token::FLOAT
				|| lexItem.GetToken() == Token::CHAR
				|| lexItem.GetToken() == Token::STRING
				|| lexItem.GetToken() == Token::BOOL
				|| lexItem.GetToken() == Token::PROCEDURE
				|| lexItem.GetToken() == Token::TRUE
				|| lexItem.GetToken() == Token::FALSE
				|| lexItem.GetToken() == Token::END
				|| lexItem.GetToken() == Token::IS
				|| lexItem.GetToken() == Token::THEN
				|| lexItem.GetToken() == Token::CONST
				|| lexItem.GetToken() == Token::AND
				|| lexItem.GetToken() == Token::MOD
				|| lexItem.GetToken() == Token::OR
				|| lexItem.GetToken() == Token::NOT
				|| lexItem.GetToken() == Token::BEGIN

		) {
			auto itemFound = std::find(keywords.begin(), keywords.end(), lexItem.GetLexeme());
			if (itemFound == keywords.end()) {
				keywords.push_back(lexItem.GetLexeme());
				totalKeywords++;
			}
		}

	}

	if (isError) {
		std::cout<<endl;
		return 0;
	}

	std::cout<<endl;
	std::cout <<"Lines: "<<lineNumber<<endl;
	std::cout <<"Total Tokens: "<<totalTokens<<endl;
	std::cout <<"Numerals: "<<totalNumerals<<endl;
	std::cout<<"Characters and Strings : " <<totalStrings<<endl;
	std::cout<<"Identifiers: " <<totalIds<<endl;
	std::cout<<"Keywords: "<<totalKeywords <<endl;
	stable_sort(strings.begin(), strings.end());
	stable_sort(ids.begin(), ids.end());
	stable_sort(numerals.begin(), numerals.end());

	if (numerals.size() > 0 && printNum) {
		std::cout<<"NUMERIC CONSTANTS:"<<endl;
		int i = 0;
		for (auto it1 : numerals){
			std::cout<<it1;
			if (i<numerals.size()-1)
				std::cout <<", ";
			i++;
		}
		std::cout<<endl;
	}

	if (strings.size() > 0 && printString) {
		std::cout<<"CHARACTERS AND STRINGS:"<<endl;
		int i =0;
		for (auto it2 : strings){
			std::cout<<it2;
			if (i<strings.size()-1)
				std::cout <<", ";
			i++;
		}
		std::cout<<endl;
	}

	if (ids.size() > 0 && printId) {
		std::cout<<"IDENTIFIERS:"<<endl;
		int i=0;
		for (auto it3 : ids){
			std::cout<<it3;
			if (i<ids.size()-1)
				std::cout <<", ";
			i++;
		}
		std::cout<<endl;
	}
	if (keywords.size() > 0 && printKW) {
		std::cout<<"KEYWORDS:"<<endl;
		int i = 0;
		for (auto it4 : keywords){
			std::cout<<it4;
			if (i<keywords.size()-1)
				std::cout <<", ";
			i++;
		}
		std::cout<<endl;
	}
}
