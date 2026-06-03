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
		EXPECT_EQ(obj.GetBlankLines(), blankLines);
		EXPECT_EQ(obj.GetCommentLines(), commentLines);
		EXPECT_EQ(obj.GetFilesCount(), filesCount);
		EXPECT_EQ(obj.GetCodeLines(), codeLines);
	}

protected:
	ISXFileReader::ThreadFilesReader obj;
	std::filesystem::path tmpDir;
};

TEST_F(TFRTest, TrimEmptyStr) {
	EXPECT_EQ(obj.Trim(""), "");	
}
TEST_F(TFRTest, TrimSpacesStr) {
	EXPECT_EQ(obj.Trim("   \t   "), "");
}
TEST_F(TFRTest, TrimStrWithSpacesAfter) {
	EXPECT_EQ(obj.Trim("world    "), "world");
}
TEST_F(TFRTest, TrimStrWithSpacesBefore) {
	EXPECT_EQ(obj.Trim("        world"), "world");
}
TEST_F(TFRTest, TrimStrWithSpacesBothSides) {
	EXPECT_EQ(obj.Trim("        world            "), "world");
}
TEST_F(TFRTest, TrimNoSpacesStr) {
	EXPECT_EQ(obj.Trim("world"), "world");
}
TEST_F(TFRTest, TrimStrWithEscapes) {
	EXPECT_EQ(obj.Trim("\t\rworld\n\r\n"), "world");
}
TEST_F(TFRTest, TrimStrWithEscapesBetween) {
	EXPECT_EQ(obj.Trim("  hello  world   "), "hello  world");
}

TEST_F(TFRTest, ProcessLineEmptyStr) {
	auto str = obj.Trim("");
	EXPECT_FALSE(obj.ProcessLine(str, false));
	checkValues(1, 0, 0, 0);
}
TEST_F(TFRTest, ProcessLineOnlySpaces) {
	auto str = obj.Trim("    \t   ");
	EXPECT_TRUE(obj.ProcessLine(str, true));
	checkValues(1, 0, 0, 0);
}
TEST_F(TFRTest, ProcessLine_BlockCommentStart) {
	auto str = obj.Trim("/* start of block");
	bool result = obj.ProcessLine(str, false);
	EXPECT_TRUE(result); 
	checkValues(0, 1, 0, 0);
}

TEST_F(TFRTest, ProcessLine_BlockCommentEnd) {
	auto str = obj.Trim("end of block */");
	bool result = obj.ProcessLine(str, true);
	EXPECT_FALSE(result);  
	checkValues(0, 1, 0, 0);
}

TEST_F(TFRTest, ProcessLine_BlockCommentSingleLine) {
	auto str = obj.Trim("/* inline block */");
	bool result = obj.ProcessLine(str, false);
	EXPECT_FALSE(result);
	checkValues(0, 1, 0, 0);
}

TEST_F(TFRTest, ProcessLine_CodeThenLineComment) {
	auto str = obj.Trim("int x; // comment");
	bool result = obj.ProcessLine(str, false);
	EXPECT_FALSE(result);
	checkValues(0, 1, 0, 1);
}

TEST_F(TFRTest, ProcessLine_LineComment_TwoCharsOnly) {
	auto str = obj.Trim("//");
	bool result = obj.ProcessLine(str, false);
	EXPECT_FALSE(result);
	checkValues(0, 1, 0, 0);
} 

TEST_F(TFRTest, ProcessLine_InsideBlockComment) {
	auto str = obj.Trim("just some text inside block");
	bool result = obj.ProcessLine(str, true);
	EXPECT_TRUE(result); 
	checkValues(0, 1, 0, 0);
}

TEST_F(TFRTest, ProcessLine_PureCode) {
	auto str = obj.Trim("int x = 42;");
	bool result = obj.ProcessLine(str, false);
	EXPECT_FALSE(result);
	checkValues(0, 0, 0, 1);
}

TEST_F(TFRTest, ProcessLine_SlashInString_NotComment) {
	auto str = obj.Trim("std::string s = \"http://example.com\";");
	bool result = obj.ProcessLine(str, false);
	EXPECT_FALSE(result);
	checkValues(0, 0, 0, 1);
}

TEST_F(TFRTest, ProcessLine_CodeAroundInlineBlock) {
	auto str = obj.Trim("int x; /* comment */ int y;");
	bool result = obj.ProcessLine(str, false);
	EXPECT_FALSE(result);
	checkValues(0, 1, 0, 1);
}

TEST_F(TFRTest, ProcessLine_BlockThenCode) {
	auto str = obj.Trim("/* comment */ int x;");
	bool result = obj.ProcessLine(str, false);
	EXPECT_FALSE(result);
	checkValues(0, 1, 0, 1);
}

TEST_F(TFRTest, ProcessFile_NonExistentFile) {
	EXPECT_NO_THROW(obj.ProcessFile("/nonexistent/path/file.cpp"));
}

TEST_F(TFRTest, ProcessFile_EmptyFile) {
	std::string path = createTestFile("empty.cpp", "");
	EXPECT_NO_THROW(obj.ProcessFile(path));
	checkValues(0, 0, 1, 0);
}

TEST_F(TFRTest, ProcessFile_OnlyBlankLines) {
	std::string path = createTestFile("blank.cpp", "\n\n\n");
	EXPECT_NO_THROW(obj.ProcessFile(path));
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
	EXPECT_NO_THROW(obj.ProcessFile(path));
	checkValues(1, 1, 1, 3);
}

TEST_F(TFRTest, ProcessFile_InlineBlockComment) {
	auto path = createTestFile("inline.cpp",
		"int x = /* value */ 42;\n"
	);
	obj.ProcessFile(path);
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
	EXPECT_NO_THROW(obj.ProcessFile(path));
	checkValues(0, 4, 1, 1);
}
TEST_F(TFRTest, ProcessFile_MultipleFiles) {
	auto p1 = createTestFile("a.cpp", "int x;\n");
	auto p2 = createTestFile("b.cpp", "int y;\n");
	auto p3 = createTestFile("c.cpp", "int z;\n");
	obj.ProcessFile(p1);
	obj.ProcessFile(p2);
	obj.ProcessFile(p3);
	checkValues(0, 0, 3, 3);
}

TEST_F(TFRTest, ProcessFile_UnclosedBlockComment) {
	auto path = createTestFile("unclosed.cpp",
		"int x;\n"  
		"/* start\n" 
		"no end\n"   
	);
	obj.ProcessFile(path);
	checkValues(0, 2, 1, 1);
}

TEST_F(TFRTest, IsValidType_cpp) { EXPECT_TRUE(obj.IsValidType("file.cpp")); }
TEST_F(TFRTest, IsValidType_h) { EXPECT_TRUE(obj.IsValidType("file.h")); }
TEST_F(TFRTest, IsValidType_hpp) { EXPECT_TRUE(obj.IsValidType("file.hpp")); }
TEST_F(TFRTest, IsValidType_c) { EXPECT_TRUE(obj.IsValidType("file.c")); }

TEST_F(TFRTest, IsValidType_txt) { EXPECT_FALSE(obj.IsValidType("file.txt")); }
TEST_F(TFRTest, IsValidType_py) { EXPECT_FALSE(obj.IsValidType("file.py")); }
TEST_F(TFRTest, IsValidType_NoExt) { EXPECT_FALSE(obj.IsValidType("Makefile")); }
TEST_F(TFRTest, IsValidType_CppUpper) { EXPECT_FALSE(obj.IsValidType("file.CPP")); }
TEST_F(TFRTest, IsValidType_Empty) { EXPECT_FALSE(obj.IsValidType("")); }
