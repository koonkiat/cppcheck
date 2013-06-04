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
#ifndef CheckUnusedIncludesH
#define CheckUnusedIncludesH
//---------------------------------------------------------------------------

#include "config.h"
#include "check.h"
#include "tokenize.h"
#include "errorlogger.h"

/// @addtogroup Checks
/// @{

class CPPCHECKLIB CheckUnusedIncludes: public Check {
public:
    typedef std::set<std::string> IncludeDependencySet;
    typedef std::set<std::string> DeclaredSymbolsSet;
    typedef std::set<std::string> RequiredSymbolsSet;

    /** @brief This constructor is used when registering the CheckUnusedIncludes */
    CheckUnusedIncludes() : Check(myName())
    { }

	~CheckUnusedIncludes();
	
    /** @brief This constructor is used when running checks. */
    CheckUnusedIncludes(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
        : Check(myName(), tokenizer, settings, errorLogger)
    { }

    // Parse current tokens and determine..
    // * Check what functions are used
    // * What functions are declared
    void parseTokens(const Tokenizer &tokenizer);
	void parseTokensForIncludes(const Tokenizer &tokenizer);
    void parseTokensForDeclaredTypes(const Tokenizer &tokenizer);

    void parseTokenForTypedef( const Tokenizer &tokenizer );

    void parseTokensForRequiredTypes(const Tokenizer &tokenizer);

    void check(ErrorLogger * const errorLogger);

    class CPPCHECKLIB IncludeUsage {
    public:
        IncludeUsage() : lineNumber(0), usedSameFile(false), usedOtherFile(false)
        { }

        std::string filename;
        unsigned int lineNumber;
        bool   usedSameFile;
        bool   usedOtherFile;
        IncludeDependencySet	dependencySet;
		DeclaredSymbolsSet		declaredSymbols;
		RequiredSymbolsSet		requiredSymbols;
    };
    typedef std::map<std::string, IncludeUsage> IncludeMap;

    const IncludeMap& GetIncludeMap() { return m_includeMap; }
	void GetIncludeDependencies(std::string & out_String);

private:

    void getErrorMessages(ErrorLogger *errorLogger, const Settings *settings) const {
        CheckUnusedIncludes c(0, settings, errorLogger);
        c.unusedIncludeError(errorLogger, "", 0, "includeName");
    }

    /**
     * Dummy implementation, just to provide error for --errorlist
     */
    void unusedIncludeError(ErrorLogger * const errorLogger,
                             const std::string &filename, unsigned int lineNumber,
                             const std::string &includename);

    /**
     * Dummy implementation, just to provide error for --errorlist
     */
    void runSimplifiedChecks(const Tokenizer *, const Settings *, ErrorLogger *) {

    }

    static std::string myName() {
        return "Unused includes";
    }

    std::string classInfo() const {
        return "Check for includes that are never called\n";
    }

	void GetFileNameFromPath(std::string src_path, std::string& out_filename);

    IncludeMap m_includeMap;
};
/// @}
//---------------------------------------------------------------------------
#endif
