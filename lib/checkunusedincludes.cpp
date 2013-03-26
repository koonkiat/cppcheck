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
//---------------------------------------------------------------------------




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

            IncludeUsage &incl = _includes[ includeName ];
            incl.filename = includeName;

			incl.dependencySet.insert(tokenizer.getSourceFilePath());
        }
    }
}
void CheckUnusedIncludes::parseTokensForDeclaredTypes(const Tokenizer &tokenizer)
{
	tokenizer;
}
void CheckUnusedIncludes::parseTokensForRequiredTypes(const Tokenizer &tokenizer)
{
    tokenizer;
    /*// Function declarations..
    for (const Token *tok = tokenizer.tokens(); tok; tok = tok->next()) {
        if (tok->fileIndex() != 0)
            continue;

        // token contains a ':' => skip to next ; or {
        if (tok->str().find(":") != std::string::npos) {
            while (tok && tok->str().find_first_of(";{"))
                tok = tok->next();
            if (tok)
                continue;
            break;
        }

        // If this is a template function, skip it
        if (tok->previous() && tok->previous()->str() == ">")
            continue;

        const Token *funcname = 0;

        if (Token::Match(tok, "%type% %var% ("))
            funcname = tok->next();
        else if (Token::Match(tok, "%type% *|& %var% ("))
            funcname = tok->tokAt(2);
        else if (Token::Match(tok, "%type% :: %var% (") && !Token::Match(tok, tok->strAt(2).c_str()))
            funcname = tok->tokAt(2);

        // Don't assume throw as a function name: void foo() throw () {}
        if (Token::Match(tok->previous(), ")|const") || funcname == 0)
            continue;

        tok = funcname->linkAt(1);

        // Check that ") {" is found..
        if (! Token::Match(tok, ") const| {") &&
            ! Token::Match(tok, ") const| throw ( ) {"))
            funcname = 0;

        if (funcname) {
            IncludeUsage &func = _includes[ funcname->str()];

            if (!func.lineNumber)
                func.lineNumber = funcname->linenr();

            // No filename set yet..
            if (func.filename.empty()) {
                func.filename = tokenizer.getSourceFilePath();
            }
            // Multiple files => filename = "+"
            else if (func.filename != tokenizer.getSourceFilePath()) {
                //func.filename = "+";
                func.usedOtherFile |= func.usedSameFile;
            }
        }
    }

    // Function usage..
    const Token *scopeEnd = NULL;
    for (const Token *tok = tokenizer.tokens(); tok; tok = tok->next()) {
        if (scopeEnd == NULL) {
            if (!Token::Match(tok, ")|= const| {"))
                continue;
            scopeEnd = tok;
            while (scopeEnd->str() != "{")
                scopeEnd = scopeEnd->next();
            scopeEnd = scopeEnd->link();
        } else if (tok == scopeEnd) {
            scopeEnd = NULL;
            continue;
        }


        const Token *funcname = 0;

        if (Token::Match(tok->next(), "%var% (")) {
            funcname = tok->next();
        }

        else if (Token::Match(tok, "[;{}.,()[=+-/&|!?:] %var% [(),;:}]"))
            funcname = tok->next();

        else if (Token::Match(tok, "[=(,] &| %var% :: %var%")) {
            funcname = tok->next();
            if (funcname->str() == "&")
                funcname = funcname->next();
            while (Token::Match(funcname,"%var% :: %var%"))
                funcname = funcname->tokAt(2);
            if (!Token::Match(funcname, "%var% [,);]"))
                continue;
        }

        else
            continue;

        // funcname ( => Assert that the end parentheses isn't followed by {
        if (Token::Match(funcname, "%var% (")) {
            if (Token::Match(funcname->linkAt(1), ") const|throw|{"))
                funcname = NULL;
        }

        if (funcname) {
            IncludeUsage &func = _includes[ funcname->str()];

            if (func.filename.empty() || func.filename == "+")
                func.usedOtherFile = true;
            else
                func.usedSameFile = true;
        }
    }*/
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