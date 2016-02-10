#!/bin/bash

# Sample usage: tools/autotest.sh component/subcomponent/
# Will do the following:
#  - Extract all BUILD files under component/subcomponent/...
#  - Extract all test targets from them, i.e. build targets ending in _test.
#  - Build and run the tests
#  - Collect the report in xml format. Also sotre stdout and stderr of test.
#  - Generate code coverage report.
# A seaprate subdir is made under $TEST_RESULTS_DIR for each test target.
# Please make sure to always run this script from the project root path.

TEST_RESULTS_DIR="./autotest-results-cpp"
GCOVR=cpp-base/tools/gcovr

# Check if the subdir argument is given:
if [ $# != 1 ] && [ $# != 2 ] ; then
	echo "Usage: $0 <subdir to run all its tests recursively>" \
                       "[one test target name to skip all t ests but this (optional)]"
	exit 1
fi
SUBDIR=$1
SPECIFIC_TEST_TARGET=$2

# Some rough heuristic to see if we are not run from the project root path:
if [ ! -z "`echo $SUBDIR | grep '\.'`" ] ; then
	echo "Bad subdir $SUBDIR -- contains '.'"
	echo "Make sure to run this script from the project root path."
	exit 1
fi

# Make sure to end SUBDIR in a forward slash:
if [ -z "`echo $SUBDIR` | grep /?" ] ; then
	SUBDIR=${SUBDIR}"/"
fi

# Find all BUILD files
BUILD_FILES=`find $SUBDIR -name BUILD`
if [ -z "$BUILD_FILES" ] ; then
	echo "No BUILD file found under $SUBDIR"
	exit 1
fi

# Read the BUILD files and extract all test targets:
TARGETS=""
for BUILD_FILE in $BUILD_FILES ; do
	# Extract test targets: extract 'my_test' from 'name = "my_test",':
	TARGET_NAMES=`grep "_test[\"\|']" $BUILD_FILE | cut -f 2 -d "=" | sed 's/ //g' | \
	                                                sed 's/"//g' | sed 's/,//g'`

	# Extract "dir1/dir2/" from "dir1/dir2/BUILD"
	BUILD_PATH=`echo $BUILD_FILE | sed 's/BUILD$//g'`

	for T in $TARGET_NAMES ; do
		if [ ! -z "$SPECIFIC_TEST_TARGET" ] && [ "$T" != "$SPECIFIC_TEST_TARGET" ] ; then
			continue
		fi
		# Append to the list of targets: e.g., component/subcomponent/my_test
		TARGETS="${TARGETS} ${BUILD_PATH}${T}"
	done
done

echo "Test targets are:"
for T in $TARGETS ; do
	echo "    $T"
done

# Create a temporary directory to hold the test results
echo ; echo "Deleting and recreating $TEST_RESULTS_DIR ..."
rm -rf $TEST_RESULTS_DIR 2>/dev/null
mkdir -p $TEST_RESULTS_DIR

bazel clean

for TARGET in $TARGETS ; do
	echo ; echo ">>>> Building $TARGET:"
	# g++ flags that enable the generation of code coverage info are disabled by default.
	# To have them enabled for test codes, I've created a custom bazelrc that's used here.
	bazel --blazerc=.bazelrc.test build $TARGET

	# Test result path: $TEST_RESULTS_DIR/component:subcomponent:my_test/
	TEST_RESULT_PATH=${TEST_RESULTS_DIR}/`echo $TARGET | sed 's/\//:/g'`
	mkdir $TEST_RESULT_PATH
	echo ">>>> Running bazel-bin/$TARGET, results go to ${TEST_RESULT_PATH}/output.xml"
	bazel-bin/$TARGET --gtest_output="xml:${TEST_RESULT_PATH}/output.xml" \
			1>${TEST_RESULT_PATH}/stdout 2>${TEST_RESULT_PATH}/stderr

	# Coverage data is written to two data files per each source, *.gcno and *.gcda, and bazel
	# writes them to some mysterious path. This path may be specified to g++ (via bazel), but
	# interestingly g++ only takes the path to the .gcda file (not the .gcno file!). Also,
	# gcov/gcovr (the tool to analyze these files) can't correctly handle seeing these files
	# and the source files being in different places. So: let these files be generated in
	# bazel's default path, then collect them and put next to source files, do the analysis
	# and remove them.
	# e.g. bazel-out/local_linux-fastbuild/bin/component/subcomponent/
	#          _objs/some_build_target/component/subcomponent/some_file.pic.gcda
	# Extract _objs path from the above path.
	NUM_SLASHES=`echo $TARGET | grep -o "/" | wc -l`
	BUILD_PATH=`echo $TARGET | cut -f 1-$((NUM_SLASHES)) -d "/"`/
	echo ">>>> Collecting code coverage report"
	echo "Build path for test target $TARGET is $BUILD_PATH"

	OBJS_PATH=bazel-out/local_linux-fastbuild/bin/${BUILD_PATH}_objs/
	GCNO_FILES=`find $OBJS_PATH -name "*.gcno"`
	GCDA_FILES=`find $OBJS_PATH -name "*.gcda"`
	for FILE in $GCNO_FILES $GCDA_FILES ; do
		# In the example above, the subpath after some_build_target/ is where the current
		# .gcno/.gcda file (i.e., $FILE) should be copied to (next to its source file).
		DST_FILE=`echo $FILE | cut -f $((NUM_SLASHES+6))- -d "/"`
		echo "Copying $FILE to $DST_FILE"
		cp $FILE $DST_FILE
	done

	echo "Generating code coverage report in $TEST_RESULT_PATH/gc.*.html and gc.xml"
	$GCOVR -r $BUILD_PATH --html --html-details -o $TEST_RESULT_PATH/gc.html
	$GCOVR -r $BUILD_PATH --xml-pretty -o $TEST_RESULT_PATH/gc.xml

	# Remove all *.gcda files so in the next text we only see those relevant tothat test.
	# Do not remove *.gcno; they are compile-time-generated files that may not get reproduced
	# by running each test.
	rm -f $GCDA_FILES
	# Also remove whatever we copied next to source files. After the next test, the relevant
	# ones will be copied again.
	rm -f `find $SUBDIR -name "*.gcno"` `find $SUBDIR -name "*.gcda"`
done

