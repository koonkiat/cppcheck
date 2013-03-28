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
		
		TEST_CASE(checkAllVar_symbolDB);

        TEST_CASE(checkDeclaredVar_manual);
        TEST_CASE(checkRequiredVar);

        TEST_CASE(checkAnonymousEnum);
        TEST_CASE(checkTypeDef);
        TEST_CASE(Typedef5);
        TEST_CASE(Typedef7);
    }
    void check(CheckUnusedIncludes &c, const char code[]) {
        // Clear the error buffer..
        errout.str("");

        Settings settings;
        settings.addEnabled("style");

        // Tokenize..
        Tokenizer tokenizer(&settings, this);
        std::istringstream istr(code);
        expandMacros(istr.str());
        tokenizer.tokenize(istr, "test.cpp");

        // Check for unused functions..
//        CheckUnusedIncludes CheckUnusedIncludes(&tokenizer, &settings, this);
        c.parseTokens(tokenizer);
        c.check(this);
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

            ASSERT_EQUALS(true, Token::Match(enumIsType.tokens(), "enum %var%"));

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
        CheckUnusedIncludes c;
        check(c, "#define SOMETHING \n#include <abc.h>;\n#include <xyz>;");

        ASSERT_EQUALS(0, c.GetIncludeMap().size());
        const CheckUnusedIncludes::IncludeMap& includeMap = c.GetIncludeMap();
        ASSERT_EQUALS(false, includeMap.find("abc.h") != includeMap.end());
        ASSERT_EQUALS(false, includeMap.find("xyz") != includeMap.end());
    }
    void parseIncludesQuotes() {
        CheckUnusedIncludes c;
        check(c, "#define SOMETHING \n#include \"abc.h\";\n#include \"xyz.hpp\";");

        ASSERT_EQUALS(2, c.GetIncludeMap().size());
        const CheckUnusedIncludes::IncludeMap& includeMap = c.GetIncludeMap();
        ASSERT_EQUALS(true, includeMap.find("abc.h") != includeMap.end());
        ASSERT_EQUALS(true, includeMap.find("xyz.hpp") != includeMap.end());
    }
    void parseIncludesWithPath() {
        CheckUnusedIncludes c;
        check(c, "#define SOMETHING \n#include \"..\\abc.h\";\n#include \"../seven/xyz.hpp\";");

        ASSERT_EQUALS(2, c.GetIncludeMap().size());
        const CheckUnusedIncludes::IncludeMap& includeMap = c.GetIncludeMap();
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
        CheckUnusedIncludes c;
        check(c, 
            "#ifdef ABC \n#include \"abc.h\";\nbool b = true; \n#else \n#include \"abc.h\";\n \
			bool b = false; \n \
			#endif \\ ABC \n \
			if(b) { \n \
			return; \n \
			}");

		const CheckUnusedIncludes::IncludeUsage &incl = c.GetIncludeMap().begin()->second;
		ASSERT_EQUALS(1, incl.dependencySet.size());
	}
	void checkAllVar_symbolDB() {
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

    void checkDeclaredVar_manual() {
        CheckUnusedIncludes c;
        check(c,"#include \"abc.h\";\n"
                "class xyz;\n"
                "class myClass{\n"
                    "public:\n"
                    "enum myEnum{\n"
                        "eNone = 0,\n"
                        "eOne };\n"
                    "abc Type_abc1;\n"
                    "struct myStruct{\n"
                        "xyz* Type_xyz1;\n"
                    "};\n"
                    "static std::string str1;\n"
                    "static const std::string str2;\n"
                    "const std::string* pStr1;\n"
                    "const char charArr[];\n"
                    "void f(const char* pChar) {}"
                "};"
            );

        const CheckUnusedIncludes::DeclaredSymbolsSet& declaredSymbols = c.GetDeclaredSymbolsSet();
        unsigned int expectedSymbolCount = 3;
        ASSERT_EQUALS(expectedSymbolCount, declaredSymbols.size());
        
        if (declaredSymbols.size() >= expectedSymbolCount)
        {
            CheckUnusedIncludes::DeclaredSymbolsSet::const_iterator it = declaredSymbols.begin();
            ASSERT_EQUALS("myClass", it->c_str());
            ++it;
            ASSERT_EQUALS("myEnum", it->c_str());
            ++it;
            ASSERT_EQUALS("myStruct", it->c_str());
        }
    }


    void checkRequiredVar() {
        CheckUnusedIncludes c;
        check(c, 
            "#include \"abc.h\";\n"
            "class xyz;\n"
            "class myClass{\n"
            "public:\n"
            "enum myEnum{\n"
            "eNone = 0,\n"
            "eOne };\n"
            "abc Type_abc1;\n"
            "struct myStruct{\n"
            "xyz* Type_xyz1;\n"
            "};\n"
            "static std::string str1;\n"
            "static const std::string str2;\n"
            "const std::string* pStr1;\n"
            "const char charArr[];\n"
            "void f(const char* pChar) {}"
            "};"
            );

        const CheckUnusedIncludes::RequiredSymbolsSet& requiredSymbols = c.GetRequiredSymbolsSet();
        unsigned int expectedSymbolCount = 2;
        ASSERT_EQUALS(expectedSymbolCount, requiredSymbols.size());

        if (requiredSymbols.size() >= expectedSymbolCount)
        {
            CheckUnusedIncludes::RequiredSymbolsSet::const_iterator it = requiredSymbols.begin();
            ASSERT_EQUALS("abc", it->c_str());
            ++it;
            ASSERT_EQUALS("xyz", it->c_str());
        }
    }


    void checkAnonymousEnum() {
        CheckUnusedIncludes c;
        check(c, 
            "enum myEnum{\n"
            "eNone = 0,\n"
            "eOne };\n"
            "enum {\n"
            "eAnonNone = 0,\n"
            "eAnonOne };\n"
            );

        const CheckUnusedIncludes::DeclaredSymbolsSet& declaredSymbols = c.GetDeclaredSymbolsSet();
        unsigned int expectedSymbolCount = 1;
        ASSERT_EQUALS(expectedSymbolCount, declaredSymbols.size());

        if (declaredSymbols.size() >= expectedSymbolCount)
        {
            CheckUnusedIncludes::DeclaredSymbolsSet::const_iterator it = declaredSymbols.begin();
            ASSERT_EQUALS("myEnum", it->c_str());
        }
    }
    void checkTypeDef() {
        CheckUnusedIncludes c;
        check(c, 
            "typedef std::set<std::string> IncludeDependencySet;\n"
            );

        const CheckUnusedIncludes::DeclaredSymbolsSet& declaredSymbols = c.GetDeclaredSymbolsSet();
        unsigned int expectedSymbolCount = 1;
        ASSERT_EQUALS(expectedSymbolCount, declaredSymbols.size());

        if (declaredSymbols.size() >= expectedSymbolCount)
        {
            CheckUnusedIncludes::DeclaredSymbolsSet::const_iterator it = declaredSymbols.begin();
            ASSERT_EQUALS("IncludeDependencySet", it->c_str());
        }
    }
	
    void Typedef5() {
        CheckUnusedIncludes c;
        check(c, 
            "typedef struct yy_buffer_state *YY_BUFFER_STATE;\n"
            "void f()\n"
            "{\n"
            "    YY_BUFFER_STATE state;\n"
            "}\n"
            );

        const CheckUnusedIncludes::DeclaredSymbolsSet& declaredSymbols = c.GetDeclaredSymbolsSet();
        unsigned int expectedSymbolCount = 1;
        ASSERT_EQUALS(expectedSymbolCount, declaredSymbols.size());

        if (declaredSymbols.size() >= expectedSymbolCount)
        {
            CheckUnusedIncludes::DeclaredSymbolsSet::const_iterator it = declaredSymbols.begin();
            ASSERT_EQUALS("YY_BUFFER_STATE", it->c_str());
        }
    }
    void Typedef7() {
        CheckUnusedIncludes c;
        check(c, "typedef int abc ; "
                 "Fred :: abc f ;");

        const CheckUnusedIncludes::DeclaredSymbolsSet& declaredSymbols = c.GetDeclaredSymbolsSet();
        unsigned int expectedSymbolCount = 0;
        ASSERT_EQUALS(expectedSymbolCount, declaredSymbols.size());
    }
    /*void Typedef9() {
        const char code[] = "typedef struct s S, * PS;\n"
                            "typedef struct t { int a; } T, *TP;\n"
                            "typedef struct { int a; } U;\n"
                            "typedef struct { int a; } * V;\n"
                            "S s;\n"
                            "PS ps;\n"
                            "T t;\n"
                            "TP tp;\n"
                            "U u;\n"
                            "V v;";

        const char expected[] =
            "struct t { int a ; } ; "
            "struct U { int a ; } ; "
            "struct Unnamed0 { int a ; } ; "
            "struct s s ; "
            "struct s * ps ; "
            "struct t t ; "
            "struct t * tp ; "
            "struct U u ; "
            "struct Unnamed0 * v ;";

        ASSERT_EQUALS(expected, tok(code, false));
    }
    void Typedef19() {
        {
            // ticket #1275
            const char code[] = "typedef struct {} A, *B, **C;\n"
                                "A a;\n"
                                "B b;\n"
                                "C c;";

            const char expected[] =
                "struct A { } ; "
                "struct A a ; "
                "struct A * b ; "
                "struct A * * c ;";

            ASSERT_EQUALS(expected, tok(code, false));
        }
	}
	
    void Typedef10() {
        const char code[] = "typedef union s S, * PS;\n"
                            "typedef union t { int a; float b ; } T, *TP;\n"
                            "typedef union { int a; float b; } U;\n"
                            "typedef union { int a; float b; } * V;\n"
                            "S s;\n"
                            "PS ps;\n"
                            "T t;\n"
                            "TP tp;\n"
                            "U u;\n"
                            "V v;";

        const char expected[] =
            "union t { int a ; float b ; } ; "
            "union U { int a ; float b ; } ; "
            "union Unnamed1 { int a ; float b ; } ; "
            "union s s ; "
            "union s * ps ; "
            "union t t ; "
            "union t * tp ; "
            "union U u ; "
            "union Unnamed1 * v ;";

        ASSERT_EQUALS(expected, tok(code, false));
    }

    void Typedef11() {
        const char code[] = "typedef enum { a = 0 , b = 1 , c = 2 } abc;\n"
                            "typedef enum xyz { x = 0 , y = 1 , z = 2 } XYZ;\n"
                            "abc e1;\n"
                            "XYZ e2;";

        const char expected[] = "int e1 ; "
                                "int e2 ;";

        ASSERT_EQUALS(expected, tok(code, false));
    }

    void Typedef12() {
        const char code[] = "typedef vector<int> V1;\n"
                            "typedef std::vector<int> V2;\n"
                            "typedef std::vector<std::vector<int> > V3;\n"
                            "typedef std::list<int>::iterator IntListIterator;\n"
                            "V1 v1;\n"
                            "V2 v2;\n"
                            "V3 v3;\n"
                            "IntListIterator iter;";

        const char expected[] =
            "vector < int > v1 ; "
            "std :: vector < int > v2 ; "
            "std :: vector < std :: vector < int > > v3 ; "
            "std :: list < int > :: iterator iter ;";

        ASSERT_EQUALS(expected, tok(code, false));
    }
    void Typedef15() {
        {
            const char code[] = "typedef char frame[10];\n"
                                "frame f;";

            const char expected[] = "char f [ 10 ] ;";

            ASSERT_EQUALS(expected, tok(code, false));
        }

        {
            const char code[] = "typedef unsigned char frame[10];\n"
                                "frame f;";

            const char expected[] = "unsigned char f [ 10 ] ;";

            ASSERT_EQUALS(expected, tok(code, false));
        }
    }
    void Typedef21() {
        const char code[] = "typedef void (* PF)();\n"
                            "typedef void * (* PFV)(void *);\n"
                            "PF pf;\n"
                            "PFV pfv;";

        const char expected[] =
            ""
            ""
            "void ( * pf ) ( ) ; "
            "void * ( * pfv ) ( void * ) ;";

        ASSERT_EQUALS(expected, Typedef(code));
    }
	void Typedef22() {
        {
            const char code[] = "class Fred {\n"
                                "    typedef void * (*testfp)(void *);\n"
                                "    testfp get() { return test; }\n"
                                "    static void * test(void * p) { return p; }\n"
                                "};\n";

            const char expected[] =
                "class Fred { "
                ""
                "void * ( * get ( ) ) ( void * ) { return test ; } "
                "static void * test ( void * p ) { return p ; } "
                "} ;";

            ASSERT_EQUALS(expected, tok(code, false));
        }
	}


	*/
};

REGISTER_TEST(TestUnusedIncludes)
