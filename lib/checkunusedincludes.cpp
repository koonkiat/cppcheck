/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2013 Daniel Marjamäki and Cppcheck team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


//---------------------------------------------------------------------------
#include "checkunusedincludes.h"
#include "tokenize.h"
#include "token.h"
#include <cctype>
#include <sstream>
#include <algorithm>
#include "symboldatabase.h"
#include "Path.h"
//---------------------------------------------------------------------------


CheckUnusedIncludes::~CheckUnusedIncludes()
{
	std::string outstr;
	outstr += "IncludeMap\n";
	for (IncludeMap::const_iterator it = _includes.begin(); it != _includes.end(); ++it) {
		outstr += "\n" + it->first + ":";
		const IncludeDependencySet& depSet = it->second.dependencySet;
		for (IncludeDependencySet::const_iterator setIter = depSet.begin(); setIter != depSet.end(); ++setIter) {
			outstr += "\n" + *setIter;
		}
		outstr += "\n";
		outstr += "RequiredSymbolsSet\n";
		const RequiredSymbolsSet& reqSet = it->second.requiredSymbols;
		for (RequiredSymbolsSet::const_iterator setIter = reqSet.begin(); setIter != reqSet.end(); ++setIter) {
			outstr += *setIter + "\n";
		}
		outstr += "DeclaredSymbolsSet\n";
		const DeclaredSymbolsSet& decSet = it->second.declaredSymbols;
		for (DeclaredSymbolsSet::const_iterator setIter = decSet.begin(); setIter != decSet.end(); ++setIter) {
			outstr += *setIter + "\n";
		}
	}
	outstr += "\n";

	std::cout << outstr << std::endl;
}

//---------------------------------------------------------------------------
// FUNCTION USAGE - Check for unused functions etc
//---------------------------------------------------------------------------

void CheckUnusedIncludes::parseTokens(const Tokenizer &tokenizer)
{
	parseTokensForDeclaredTypes(tokenizer);
	parseTokensForRequiredTypes(tokenizer);
	parseTokensForIncludes(tokenizer);
}
void CheckUnusedIncludes::parseTokensForIncludes(const Tokenizer &tokenizer)
{
    for (const Token *tok = tokenizer.tokens(); tok; tok = tok->next()) {
        if (tok->fileIndex() != 0)
            continue;
        if(Token::Match(tok, "#include"))
        {
            std::string includeName("");
            if (Token::Match(tok, "#include %str%")) {
                std::string str = tok->strAt(1);
                size_t startOfIncludeName = 1;
                size_t endOfIncludeName = str.find_last_of('\"');
                //find end of path
                if(str.find_last_of('/') != std::string::npos)
                {
                    startOfIncludeName = str.find_last_of('/') + 1;
                }
                if(startOfIncludeName == 1 
                && str.find_last_of('\\') != std::string::npos)
                {
                    startOfIncludeName = str.find_last_of('\\') + 1;
                }
                includeName = str.substr(startOfIncludeName, endOfIncludeName - startOfIncludeName);
            }
            else {
                continue;
            }

			std::transform(includeName.begin(), includeName.end(), includeName.begin(), ::tolower);
			IncludeUsage &incl = _includes[ includeName ];
            incl.filename = includeName;

			incl.dependencySet.insert(tokenizer.getSourceFilePath());
        }
    }
}
void CheckUnusedIncludes::parseTokensForDeclaredTypes(const Tokenizer &tokenizer)
{
    // use symboldatabase scope list
    const SymbolDatabase* symbolDatabase = tokenizer.getSymbolDatabase();

	std::string fileName("");
	GetFileNameFromPath(tokenizer.getSourceFilePath(), fileName);
	std::transform(fileName.begin(), fileName.end(), fileName.begin(), ::tolower);

    for (std::list<Scope>::const_iterator scope = symbolDatabase->scopeList.begin(); scope != symbolDatabase->scopeList.end(); ++scope) {
        if (scope->isForwardDeclaration()) {
            continue;
        }
        if (scope->isClassOrStruct()) {
			IncludeUsage &incl = _includes[ fileName ];
            incl.declaredSymbols.insert(scope->className);
        }
    }
    // custom parsing for enums
    for (const Token *tok = tokenizer.tokens(); tok; tok = tok->next()) {
        if (tok->fileIndex() != 0)
            continue;
        if(Token::Match(tok, "enum %var%")) {
			IncludeUsage &incl = _includes[ fileName ];
            incl.declaredSymbols.insert(tok->strAt(1));
        }
    }
    // todo parsing for typedef
    parseTokenForTypedef(tokenizer);

}
void CheckUnusedIncludes::parseTokensForRequiredTypes(const Tokenizer &tokenizer)
{
    // use symboldatabase var list
    const SymbolDatabase* symbolDatabase = tokenizer.getSymbolDatabase();
	
	std::string fileName("");
	GetFileNameFromPath(tokenizer.getSourceFilePath(), fileName);
	std::transform(fileName.begin(), fileName.end(), fileName.begin(), ::tolower);

    size_t listCount = symbolDatabase->getVariableListSize();
    for (unsigned int i = 0; i < listCount; ++i)
    {
        const Variable* var = symbolDatabase->getVariableFromVarId(i);
        if (!var || !var->typeEndToken())
        {
            continue;
        }
        
        if (var->typeEndToken() && var->typeStartToken() 
            && !var->typeEndToken()->isStandardType()
            && !var->typeStartToken()->isStandardType())
        {
            bool isFromStdLib = var->typeStartToken()->str() == "std";
            if (!isFromStdLib)
            {
                bool isPointerOrRef = var->typeEndToken()->str() == "*" || var->typeEndToken()->str() == "&";
				IncludeUsage &incl = _includes[ fileName ];
                if (isPointerOrRef)
                {
                    incl.requiredSymbols.insert(var->typeEndToken()->previous()->str());
                }
                else
                {
                    incl.requiredSymbols.insert(var->typeEndToken()->str());
                }
            }
        }
    }

}
void CheckUnusedIncludes::GetFileNameFromPath(std::string src_path, std::string& out_filename)
{
	size_t lastForwardSlash = src_path.find_last_of('/');
	size_t lastBackSlash = src_path.find_last_of('\\');

	if(lastForwardSlash == std::string::npos && lastBackSlash == std::string::npos) {
		out_filename = src_path;
		return;
	}

	size_t startPos = 0;

	if(lastForwardSlash != std::string::npos && lastBackSlash != std::string::npos) {
		startPos = (lastForwardSlash > lastBackSlash) ? lastForwardSlash : lastBackSlash;
	}
	else {
		startPos = (lastForwardSlash != std::string::npos) ? lastForwardSlash : lastBackSlash;
	}
	out_filename = src_path.substr(startPos+1);
}



void CheckUnusedIncludes::check(ErrorLogger * const errorLogger)
{
    for (IncludeMap::const_iterator it = _includes.begin(); it != _includes.end(); ++it) {
        const IncludeUsage &func = it->second;
        if (func.usedOtherFile || func.filename.empty())
            continue;
        if (it->first == "main" ||
            it->first == "WinMain" ||
            it->first == "_tmain" ||
            it->first == "if" ||
            (it->first.compare(0, 8, "operator") == 0 && it->first.size() > 8 && !std::isalnum(it->first[8])))
            continue;
        if (! func.usedSameFile) {
            std::string filename;
            if (func.filename == "+")
                filename = "";
            else
                filename = func.filename;
            unusedIncludeError(errorLogger, filename, func.lineNumber, it->first);
        } else if (! func.usedOtherFile) {
            /** @todo add error message "function is only used in <file> it can be static" */
            /*
            std::ostringstream errmsg;
            errmsg << "The function '" << it->first << "' is only used in the file it was declared in so it should have local linkage.";
            _errorLogger->reportErr( errmsg.str() );
            */
        }
    }
}

void CheckUnusedIncludes::unusedIncludeError(ErrorLogger * const errorLogger,
        const std::string &filename, unsigned int lineNumber,
        const std::string &includename)
{
    std::list<ErrorLogger::ErrorMessage::FileLocation> locationList;
    if (!filename.empty()) {
        ErrorLogger::ErrorMessage::FileLocation fileLoc;
        fileLoc.setfile(filename);
        fileLoc.line = lineNumber;
        locationList.push_back(fileLoc);
    }

    const ErrorLogger::ErrorMessage errmsg(locationList, Severity::style, "The include '" + includename + "' is never used.", "unusedInclude", false);
    if (errorLogger)
        errorLogger->reportErr(errmsg);
    else
        reportError(errmsg);
}
void CheckUnusedIncludes::GetIncludeDependencies(std::string & out_String)
{
    for (IncludeMap::const_iterator it = _includes.begin(); it != _includes.end(); ++it) {
        const IncludeUsage &incl = it->second;
        std::ostringstream ss;
        ss << incl.filename << " (" << incl.dependencySet.size() << ") :\n";
        out_String.append(ss.str());
        //out_String.append(incl.filename + "("+std::to_string(incl.dependencySet.size())+")"+":\n");
		for (IncludeDependencySet::const_iterator str_it = incl.dependencySet.begin(); str_it != incl.dependencySet.end(); ++str_it) {
			out_String.append("\t" + *str_it + "\n");
		}
	}
}

void CheckUnusedIncludes::parseTokenForTypedef( const Tokenizer &tokenizer )
{
	std::string fileName("");
	GetFileNameFromPath(tokenizer.getSourceFilePath(), fileName);
	std::transform(fileName.begin(), fileName.end(), fileName.begin(), ::tolower);

    for (const Token *tok = tokenizer.tokens(); tok; tok = tok->next()) {
        if (tok->fileIndex() != 0)
            continue;
        if(Token::Match(tok, "typedef %type%")) {
            Token* tokOffset = tok->next();
            Token* typeEnd = NULL;
            Token *typeStart = NULL;

            if (tok->next()->str() == "::" || Token::Match(tok->next(), "%type%")) {
                typeStart = tok->next();

                while (Token::Match(tokOffset, "const|signed|unsigned|struct|enum %type%") ||
                    (tokOffset->next() && tokOffset->next()->isStandardType()))
                    tokOffset = tokOffset->next();

                typeEnd = tokOffset;
                tokOffset = tokOffset->next();

                bool atEnd = false;
                while (!atEnd) {
                    if (tokOffset && tokOffset->str() == "::") {
                        typeEnd = tokOffset;
                        tokOffset = tokOffset->next();
                    }

                    if (Token::Match(tokOffset, "%type%") &&
                        tokOffset->next() && !Token::Match(tokOffset->next(), "[|;|,|(")) {
                            typeEnd = tokOffset;
                            tokOffset = tokOffset->next();
                    } else if (Token::simpleMatch(tokOffset, "const (")) {
                        typeEnd = tokOffset;
                        tokOffset = tokOffset->next();
                        atEnd = true;
                    }else if (Token::Match(tokOffset, "*|&") && tokOffset->next()) {
                        typeEnd = tokOffset;
                        tokOffset = tokOffset->next();
                    } else {
                        atEnd = true;
                    }
                }
            } else
                continue; // invalid input

            // check for invalid input
            if (!tokOffset) {
                return;
            }
            // check for template
            if (tokOffset->str() == "<") {
                tokOffset->findClosingBracket(typeEnd);

                while (typeEnd && Token::Match(typeEnd->next(), ":: %type%"))
                    typeEnd = typeEnd->tokAt(2);

                if (!typeEnd) {
                    // internal error
                    return;
                }

                while (Token::Match(typeEnd->next(), "const|volatile"))
                    typeEnd = typeEnd->next();

                tok = typeEnd;
                tokOffset = tok->next();
            }
//             if (Token::Match(tokOffset, "%var%")) {
//             }
			IncludeUsage &incl = _includes[ fileName ];
            incl.declaredSymbols.insert(tokOffset->str());
        }
    }
}
