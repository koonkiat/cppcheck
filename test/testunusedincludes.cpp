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

#include <sstream>

// test path - header is allowed now
#include "path.h"

#include "tokenize.h"
#include "testsuite.h"
#include "testutils.h"
#include "CheckUnusedIncludes.h"
#include "preprocessor.h"
#include "symboldatabase.h"

#include <sstream>

extern std::ostringstream errout;

class TestUnusedIncludes : public TestFixture {
public:
    TestUnusedIncludes() : TestFixture("TestUnusedIncludes")
    { }

private:

    void run() {

		// test path
		TEST_CASE(accept_file);


		TEST_CASE(understanding_MatchType);
        TEST_CASE(understanding_MatchIncludes);

        TEST_CASE(parseIncludesAngle);
        TEST_CASE(parseIncludesQuotes);
        TEST_CASE(parseIncludesWithPath);

        TEST_CASE(dependency_multipleFiles);

		TEST_CASE(noIncludeDuplicationForMultipleConditionalCompiles);
		
		TEST_CASE(checkVar);


    }
    void check(const char code[]) {
        // Clear the error buffer..
        errout.str("");

        Settings settings;
        settings.addEnabled("style");

        // Tokenize..
        Tokenizer tokenizer(&settings, this);
        std::istringstream istr(code);
        tokenizer.tokenize(istr, "test.cpp");

        // Check for unused functions..
        CheckUnusedIncludes CheckUnusedIncludes(&tokenizer, &settings, this);
        CheckUnusedIncludes.parseTokens(tokenizer);
        CheckUnusedIncludes.check(this);
    }
	
    static std::string expandMacros(const std::string& code, ErrorLogger *errorLogger = 0) {
        return Preprocessor::expandMacros(code, "file.cpp", "", errorLogger);
    }
//---------------------------------------------------------------------------//	
	// test path
	void accept_file() const {
        ASSERT(Path::acceptFile("index.cpp"));
        ASSERT(Path::acceptFile("index.invalid.cpp"));
        ASSERT(Path::acceptFile("index.invalid.Cpp"));
        ASSERT(Path::acceptFile("index.invalid.C"));
        ASSERT(Path::acceptFile("index.invalid.C++"));
        ASSERT(Path::acceptFile("index.")==false);
        ASSERT(Path::acceptFile("index")==false);
        ASSERT(Path::acceptFile("")==false);
        ASSERT(Path::acceptFile("C")==false);

        // we accept any headers now
        ASSERT_EQUALS(true, Path::acceptFile("index.h"));
        ASSERT_EQUALS(true, Path::acceptFile("index.hpp"));
    }
    void understanding_MatchType() {
		{
			givenACodeSampleToTokenize enumIsType("enum myEnum{ a,b };", true);
            ASSERT_EQUALS(true, Token::Match(enumIsType.tokens(), "%type%"));
            std::string str = enumIsType.tokens()->strAt(1);
            ASSERT_EQUALS("myEnum", str);
		}
		{
			givenACodeSampleToTokenize classIsType2("myClass c;", true);
			ASSERT_EQUALS(true, Token::Match(classIsType2.tokens(), "%type% %var%"));
		}
    }
    void understanding_MatchIncludes() {
		{
			givenACodeSampleToTokenize include_angle("#define SOMETHING \n#include <abc.h>;");
			// std::cout << include_angle.tokens()->stringifyList(true, true, true,true,true) << std::endl; 
			const Token *tok = include_angle.tokens();
			ASSERT_EQUALS(false, Token::Match(tok, "#include"));

			tok = tok->next()->next();
			ASSERT_EQUALS(true, Token::Match(tok, "#include"));
			std::string includeName = tok->strAt(2);
			ASSERT_EQUALS("abc", includeName.c_str());
		}
    }
    void parseIncludesAngle() {
        Settings settings;
        settings.addEnabled("style");

        // Tokenize..
        Tokenizer tokenizer(&settings, this);
        std::istringstream istr("#define SOMETHING \n#include <abc.h>;\n#include <xyz>;");
        tokenizer.tokenize(istr, "test.cpp");

        CheckUnusedIncludes CheckUnusedIncludes(&tokenizer, &settings, this);
        CheckUnusedIncludes.parseTokens(tokenizer);
        ASSERT_EQUALS(0, CheckUnusedIncludes.GetIncludeMap().size());
        const CheckUnusedIncludes::IncludeMap& includeMap = CheckUnusedIncludes.GetIncludeMap();
        ASSERT_EQUALS(false, includeMap.find("abc.h") != includeMap.end());
        ASSERT_EQUALS(false, includeMap.find("xyz") != includeMap.end());
    }
    void parseIncludesQuotes() {
        Settings settings;
        settings.addEnabled("style");

        // Tokenize..
        Tokenizer tokenizer(&settings, this);
        std::istringstream istr("#define SOMETHING \n#include \"abc.h\";\n#include \"xyz.hpp\";");
        tokenizer.tokenize(istr, "test.cpp");

        CheckUnusedIncludes CheckUnusedIncludes(&tokenizer, &settings, this);
        CheckUnusedIncludes.parseTokens(tokenizer);
        ASSERT_EQUALS(2, CheckUnusedIncludes.GetIncludeMap().size());
        const CheckUnusedIncludes::IncludeMap& includeMap = CheckUnusedIncludes.GetIncludeMap();
        ASSERT_EQUALS(true, includeMap.find("abc.h") != includeMap.end());
        ASSERT_EQUALS(true, includeMap.find("xyz.hpp") != includeMap.end());
    }
    void parseIncludesWithPath() {
        Settings settings;
        settings.addEnabled("style");

        // Tokenize..
        Tokenizer tokenizer(&settings, this);
        std::istringstream istr("#define SOMETHING \n#include \"..\\abc.h\";\n#include \"../seven/xyz.hpp\";");
        tokenizer.tokenize(istr, "test.cpp");

        CheckUnusedIncludes CheckUnusedIncludes(&tokenizer, &settings, this);
        CheckUnusedIncludes.parseTokens(tokenizer);
        ASSERT_EQUALS(2, CheckUnusedIncludes.GetIncludeMap().size());
        const CheckUnusedIncludes::IncludeMap& includeMap = CheckUnusedIncludes.GetIncludeMap();
        ASSERT_EQUALS(true, includeMap.find("abc.h") != includeMap.end());
        ASSERT_EQUALS(true, includeMap.find("xyz.hpp") != includeMap.end());
    }
    void multipleFiles1() {
		CheckUnusedIncludes c;

        // Clear the error buffer..
        errout.str("");

		FileCodePair file1h("file1.h", "#include <stdio>");
		FileCodePair file1cpp("file1.cpp", "#include \"file1.h\";");
		
		const FileCodePair* arr[] = {&file1h, &file1cpp};
		
        for (int i = 0; i < 2; ++i) {
            // Clear the error buffer..
            errout.str("");
            Settings settings;
            Tokenizer tokenizer(&settings, this);

            std::istringstream istr(arr[i]->code);
            tokenizer.tokenize(istr, arr[i]->filename.str().c_str());

            c.parseTokens(tokenizer);
        }

        // Check for unused functions..
        c.check(this);
		
        
        const CheckUnusedIncludes::IncludeMap& includeMap = c.GetIncludeMap();
        
        ASSERT_EQUALS(2, includeMap.size());

        CheckUnusedIncludes::IncludeMap::const_iterator it1 = includeMap.find("stdio");
        CheckUnusedIncludes::IncludeMap::const_iterator it2 = includeMap.find("file1.h");

        ASSERT_EQUALS(true, it1 != includeMap.end());
        ASSERT_EQUALS(true, it2 != includeMap.end());

        ASSERT_EQUALS(file1h.filename.str(), it1->second.filename);
        ASSERT_EQUALS(file1cpp.filename.str(), it2->second.filename);
    }
    void dependency_multipleFiles() {
		CheckUnusedIncludes c;

        // Clear the error buffer..
        errout.str("");

		FileCodePair file1h("file1.h", "#include <stdio>;");
		FileCodePair file1cpp("file1.cpp", "#include \"file1.h\";");
		FileCodePair file2h("file2.h", "#include \"file1.h\";");
		FileCodePair file2cpp("file2.cpp", "#include \"file2.h\"; \n \
										    #include \"file1.h\";");
		
		const FileCodePair* arr[] = {&file1h, &file1cpp, &file2h, &file2cpp};
		
        for (int i = 0; i < 4; ++i) {
            // Clear the error buffer..
            errout.str("");
            Settings settings;
            Tokenizer tokenizer(&settings, this);

            std::istringstream istr(arr[i]->code);
            tokenizer.tokenize(istr, arr[i]->filename.str().c_str());

            c.parseTokens(tokenizer);
        }

        // Check for unused functions..
        c.check(this);
		
        //const CheckUnusedIncludes::IncludeMap& includeMap = c.GetIncludeMap();
		std::string dependencies;
		c.GetIncludeDependencies(dependencies);
		ASSERT_EQUALS("file1.h (3) :\n\tfile1.cpp\n\tfile2.cpp\n\tfile2.h\nfile2.h (1) :\n\tfile2.cpp\n", dependencies);
    }
	void noIncludeDuplicationForMultipleConditionalCompiles()
	{	
        Settings settings;
        settings.addEnabled("style");

        // Tokenize..
        Tokenizer tokenizer(&settings, this);
		std::istringstream istr("#ifdef ABC \n#include \"abc.h\";\nbool b = true; \n#else \n#include \"abc.h\";\n \
								bool b = false; \n \
								#endif \\ ABC \n \
								if(b) { \n \
								return; \n \
								}");
		expandMacros(istr.str());
        tokenizer.tokenize(istr, "test.cpp");

        CheckUnusedIncludes c(&tokenizer, &settings, this);
        c.parseTokens(tokenizer);
        c.check(this);
        
		const CheckUnusedIncludes::IncludeUsage &incl = c.GetIncludeMap().begin()->second;
		ASSERT_EQUALS(1, incl.dependencySet.size());
	}
	void checkVar() {
		Settings settings;
        settings.addEnabled("style");

        // Tokenize..
        Tokenizer tokenizer(&settings, this);
        std::istringstream istr("#include \"abc.h\";\n#include \"xyz.hpp\";\n"
                                "class myClass{\n"
                                "public:\n"
                                "enum myEnum{\n"
                                "eNone = 0,\n"
                                "eOne };\n"
                                "static std::string i;\n"
						        "static const std::string j;\n"
							    "const std::string* k;\n"
							    "const char m[];\n"
							    "void f(const char* l) {}"
                                "};"
                                );
		expandMacros(istr.str());
        tokenizer.tokenize(istr, "test.cpp");

		const SymbolDatabase* symbolDatabase = tokenizer.getSymbolDatabase();
		ASSERT_EQUALS(1+5, symbolDatabase->getVariableListSize());
		ASSERT_EQUALS("std", symbolDatabase->getVariableFromVarId(1)->typeStartToken()->str());
        ASSERT_EQUALS("std", symbolDatabase->getVariableFromVarId(2)->typeStartToken()->str());
        ASSERT_EQUALS("std", symbolDatabase->getVariableFromVarId(3)->typeStartToken()->str());
        ASSERT_EQUALS("char", symbolDatabase->getVariableFromVarId(4)->typeStartToken()->str());
        ASSERT_EQUALS("char", symbolDatabase->getVariableFromVarId(5)->typeStartToken()->str());

        ASSERT_EQUALS("string", symbolDatabase->getVariableFromVarId(1)->typeEndToken()->str());
        ASSERT_EQUALS("string", symbolDatabase->getVariableFromVarId(2)->typeEndToken()->str());
        ASSERT_EQUALS("*", symbolDatabase->getVariableFromVarId(3)->typeEndToken()->str());
        ASSERT_EQUALS("char", symbolDatabase->getVariableFromVarId(4)->typeEndToken()->str());
        ASSERT_EQUALS("*", symbolDatabase->getVariableFromVarId(5)->typeEndToken()->str());

        
        const Scope *scope = NULL;
        for (std::list<Scope>::const_iterator it = symbolDatabase->scopeList.begin(); it != symbolDatabase->scopeList.end(); ++it) {
            if (it->isClassOrStruct()) {
                scope = &(*it);
                break;
            }
        }
        ASSERT(scope != 0);
        if (!scope)
            return;
        ASSERT_EQUALS("myClass", scope->className);
	}
};

REGISTER_TEST(TestUnusedIncludes)
