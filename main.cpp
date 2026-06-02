#include <gtest/gtest.h>
#include "ThreadFilesReader.h"

int main(int argc, char** argv) {
	testing::InitGoogleTest(&argc, argv);
	ThreadFilesReader reader;
	reader.Menu();

	return RUN_ALL_TESTS();
}