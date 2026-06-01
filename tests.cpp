#include <iostream>
#include <string>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "ThreadFilesReader.h"

class TFRTest : public ::testing::Test {
public:
	void SetUp() override {
		tmpDir = std::filesystem::temp_directory_path() / "tfr_test";
		std::filesystem::create_directory(tmpDir);
	}
	void TearDown() override {
		std::filesystem::remove_all(tmpDir);
	}
	std::string createTestFile(const std::string& filename, const std::string& content) {
		std::filesystem::path path = tmpDir / filename;
		std::ofstream file(path.string());
		file << content;
		return path.string();
	}

	void checkValues(int blankLines, int commentLines, int filesCount, int codeLines) {
		EXPECT_EQ(obj.getBlankLines(), blankLines);
		EXPECT_EQ(obj.getCommentLines(), commentLines);
		EXPECT_EQ(obj.getFilesCount(), filesCount);
		EXPECT_EQ(obj.getCodeLines(), codeLines);
	}

protected:
    ThreadFilesReader obj;
	std::filesystem::path tmpDir;
};

TEST_F(TFRTest, TrimEmptyStr) {
	EXPECT_EQ(obj.trim(""), "");	
}
TEST_F(TFRTest, TrimSpacesStr) {
	EXPECT_EQ(obj.trim("   \t   "), "");
}
TEST_F(TFRTest, TrimStrWithSpacesAfter) {
	EXPECT_EQ(obj.trim("world    "), "world");
}
TEST_F(TFRTest, TrimStrWithSpacesBefore) {
	EXPECT_EQ(obj.trim("        world"), "world");
}
TEST_F(TFRTest, TrimStrWithSpacesBothSides) {
	EXPECT_EQ(obj.trim("        world            "), "world");
}
TEST_F(TFRTest, TrimNoSpacesStr) {
	EXPECT_EQ(obj.trim("world"), "world");
}
TEST_F(TFRTest, TrimStrWithEscapes) {
	EXPECT_EQ(obj.trim("\t\rworld\n\r\n"), "world");
}
TEST_F(TFRTest, TrimStrWithEscapesBetween) {
	EXPECT_EQ(obj.trim("  hello  world   "), "hello  world");
}

TEST_F(TFRTest, ProcessLineEmptyStr) {
	auto str = obj.trim("");
	EXPECT_FALSE(obj.processLine(str, false));
	checkValues(1, 0, 0, 0);
}
TEST_F(TFRTest, ProcessLineOnlySpaces) {
	auto str = obj.trim("    \t   ");
	EXPECT_TRUE(obj.processLine(str, true));
	checkValues(1, 0, 0, 0);
}
TEST_F(TFRTest, ProcessLine_BlockCommentStart) {
	auto str = obj.trim("/* start of block");
	bool result = obj.processLine(str, false);
	EXPECT_TRUE(result); 
	checkValues(0, 1, 0, 0);
}

TEST_F(TFRTest, ProcessLine_BlockCommentEnd) {
	auto str = obj.trim("end of block */");
	bool result = obj.processLine(str, true);
	EXPECT_FALSE(result);  
	checkValues(0, 1, 0, 0);
}

TEST_F(TFRTest, ProcessLine_BlockCommentSingleLine) {
	auto str = obj.trim("/* inline block */");
	bool result = obj.processLine(str, false);
	EXPECT_FALSE(result);
	checkValues(0, 1, 0, 0);
}

TEST_F(TFRTest, ProcessLine_CodeThenLineComment) {
	auto str = obj.trim("int x; // comment");
	bool result = obj.processLine(str, false);
	EXPECT_FALSE(result);
	checkValues(0, 1, 0, 1);
}

TEST_F(TFRTest, ProcessLine_LineComment_TwoCharsOnly) {
	auto str = obj.trim("//");
	bool result = obj.processLine(str, false);
	EXPECT_FALSE(result);
	checkValues(0, 1, 0, 0);
} 

TEST_F(TFRTest, ProcessLine_InsideBlockComment) {
	auto str = obj.trim("just some text inside block");
	bool result = obj.processLine(str, true);
	EXPECT_TRUE(result); 
	checkValues(0, 1, 0, 0);
}

TEST_F(TFRTest, ProcessLine_PureCode) {
	auto str = obj.trim("int x = 42;");
	bool result = obj.processLine(str, false);
	EXPECT_FALSE(result);
	checkValues(0, 0, 0, 1);
}

TEST_F(TFRTest, ProcessLine_SlashInString_NotComment) {
	auto str = obj.trim("std::string s = \"http://example.com\";");
	bool result = obj.processLine(str, false);
	EXPECT_FALSE(result);
	checkValues(0, 0, 0, 1);
}

TEST_F(TFRTest, ProcessLine_CodeAroundInlineBlock) {
	auto str = obj.trim("int x; /* comment */ int y;");
	bool result = obj.processLine(str, false);
	EXPECT_FALSE(result);
	checkValues(0, 1, 0, 1);
}

TEST_F(TFRTest, ProcessLine_BlockThenCode) {
	auto str = obj.trim("/* comment */ int x;");
	bool result = obj.processLine(str, false);
	EXPECT_FALSE(result);
	checkValues(0, 1, 0, 1);
}

TEST_F(TFRTest, ProcessFile_NonExistentFile) {
	EXPECT_NO_THROW(obj.processFile("/nonexistent/path/file.cpp"));
}

TEST_F(TFRTest, ProcessFile_EmptyFile) {
	std::string path = createTestFile("empty.cpp", "");
	EXPECT_NO_THROW(obj.processFile(path));
	checkValues(0, 0, 1, 0);
}

TEST_F(TFRTest, ProcessFile_OnlyBlankLines) {
	std::string path = createTestFile("blank.cpp", "\n\n\n");
	EXPECT_NO_THROW(obj.processFile(path));
	checkValues(3, 0, 1, 0);
}

TEST_F(TFRTest, ProcessFile_CodeAndComments) {
	std::string path = createTestFile("code.cpp",
		"int main() {\n"          
		"    // comment\n"       
		"\n"                      
		"    return 0;\n"        
		"}\n"
	);
	EXPECT_NO_THROW(obj.processFile(path));
	checkValues(1, 1, 1, 3);
}

TEST_F(TFRTest, ProcessFile_InlineBlockComment) {
	auto path = createTestFile("inline.cpp",
		"int x = /* value */ 42;\n"
	);
	obj.processFile(path);
	checkValues(0, 1, 1, 1);
}

TEST_F(TFRTest, ProcessFile_BlockComment) {
	std::string path = createTestFile("block.cpp",
		"/*\n"
		" * multiline\n"
		" * comment\n"
		"*/\n"
		"int x = 1;\n"
	);
	EXPECT_NO_THROW(obj.processFile(path));
	checkValues(0, 4, 1, 1);
}
TEST_F(TFRTest, ProcessFile_MultipleFiles) {
	auto p1 = createTestFile("a.cpp", "int x;\n");
	auto p2 = createTestFile("b.cpp", "int y;\n");
	auto p3 = createTestFile("c.cpp", "int z;\n");
	obj.processFile(p1);
	obj.processFile(p2);
	obj.processFile(p3);
	checkValues(0, 0, 3, 3);
}

TEST_F(TFRTest, ProcessFile_UnclosedBlockComment) {
	auto path = createTestFile("unclosed.cpp",
		"int x;\n"  
		"/* start\n" 
		"no end\n"   
	);
	obj.processFile(path);
	checkValues(0, 2, 1, 1);
}

TEST_F(TFRTest, IsValidType_cpp) { EXPECT_TRUE(obj.isValidType("file.cpp")); }
TEST_F(TFRTest, IsValidType_h) { EXPECT_TRUE(obj.isValidType("file.h")); }
TEST_F(TFRTest, IsValidType_hpp) { EXPECT_TRUE(obj.isValidType("file.hpp")); }
TEST_F(TFRTest, IsValidType_c) { EXPECT_TRUE(obj.isValidType("file.c")); }

TEST_F(TFRTest, IsValidType_txt) { EXPECT_FALSE(obj.isValidType("file.txt")); }
TEST_F(TFRTest, IsValidType_py) { EXPECT_FALSE(obj.isValidType("file.py")); }
TEST_F(TFRTest, IsValidType_NoExt) { EXPECT_FALSE(obj.isValidType("Makefile")); }
TEST_F(TFRTest, IsValidType_CppUpper) { EXPECT_FALSE(obj.isValidType("file.CPP")); }
TEST_F(TFRTest, IsValidType_Empty) { EXPECT_FALSE(obj.isValidType("")); }
