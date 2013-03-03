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

		TEST_CASE(matchType);
		TEST_CASE(matchIncludes);
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

	
    void matchType() {
		{
			givenACodeSampleToTokenize enumIsType("enum myEnum{ a,b };", true);
			ASSERT_EQUALS(true, Token::Match(enumIsType.tokens(), "%type%"));
		}
		{
			givenACodeSampleToTokenize classIsType("class myClass{}; myClass c;", true);
			ASSERT_EQUALS(true, Token::Match(classIsType.tokens(), "%type%"));
		}
		{
			givenACodeSampleToTokenize classIsType2("myClass c;", true);
			ASSERT_EQUALS(true, Token::Match(classIsType2.tokens(), "%type% %var%"));
		}
    }
    void matchIncludes() {
        {
			givenACodeSampleToTokenize include_quote("#include \"abc.h\";");
			ASSERT_EQUALS(true, Token::Match(include_quote.tokens(), "#include"));
			ASSERT_EQUALS(true, Token::Match(include_quote.tokens(), "#include %str%"));
		}
		{
			givenACodeSampleToTokenize include_angle("#include <abc.h>;");

			std::cout << include_angle.tokens()->stringifyList(true, true, true,true,true) << std::endl; 
			ASSERT_EQUALS(true, Token::Match(include_angle.tokens(), "#include <"));
			ASSERT_EQUALS(true, Token::Match(include_angle.tokens(), "#include [<>]"));
			ASSERT_EQUALS(true, Token::Match(include_angle.tokens(), "#include <    %any%"));
			
			ASSERT_EQUALS(true, Token::Match(include_angle.tokens(), "#include <%str%>"));
			//ASSERT_EQUALS(true, Token::Match(classIsType2.tokens(), "#include %var%"));
		}
    }
};

REGISTER_TEST(TestUnusedIncludes)
