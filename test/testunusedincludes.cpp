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

// test path - header is allowed now
#include "path.h"

#include "tokenize.h"
#include "testsuite.h"
#include "testutils.h"
#include "CheckUnusedIncludes.h"
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
        ASSERT_EQUALS(2, CheckUnusedIncludes.GetIncludeMap().size());
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
};

REGISTER_TEST(TestUnusedIncludes)
