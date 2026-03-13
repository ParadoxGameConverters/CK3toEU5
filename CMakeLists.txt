cmake_minimum_required(VERSION 3.25)
project(CK3toEU5)

if (RUN_CLANG_TIDY)
	set(BUILD_TYPE "Clang-tidy")
else (RUN_CLANG_TIDY)
	set(BUILD_TYPE ${CMAKE_BUILD_TYPE})
endif (RUN_CLANG_TIDY)


set(FRONTER_SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/Fronter/Fronter/Source)
set(FRONTER_COMMONS_SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/Fronter/commonItems)
set(FRONTER_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/${BUILD_TYPE}-${PLATFORM})
set(CONVERTER_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/${BUILD_TYPE}-${PLATFORM}/CK3toEU5)
set(TEST_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/test/${BUILD_TYPE}-${PLATFORM})

set(CMAKE_CXX_STANDARD 23)
set (UNICODE_DEFAULT OFF)

if (PLATFORM STREQUAL "Windows")
	ADD_DEFINITIONS(-DUNICODE -D_UNICODE -DWINDOWS)
elseif (PLATFORM STREQUAL "Linux")
	if (COVERAGE STREQUAL true)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
		set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE ON)
	else (COVERAGE STREQUAL true)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall")
	endif (COVERAGE STREQUAL true)
endif (PLATFORM STREQUAL "Windows")


#### Gtest ####
set(GTEST_SOURCES ${GTEST_SOURCES} "external/commonItems/external/googletest/googletest/src/gtest_main.cc")
set(GTEST_SOURCES ${GTEST_SOURCES} "external/commonItems/external/googletest/googletest/src/gtest-all.cc")
set(GTEST_SOURCES ${GTEST_SOURCES} "external/commonItems/external/googletest/googlemock/src/gmock-all.cc")

add_library(
	GtestLib
	${GTEST_SOURCES}
  )


#### Fronter ####

if (PLATFORM STREQUAL "Windows")
	link_directories("./external/Fronter/external/wxWidgets/3.2.1/lib/vc14x_x64_dll")
	link_directories("./external/Fronter/external/curl/lib")
	find_library(LIBCURL_LIB libcurl HINT "./external/Fronter/external/curl/lib" NO_DEFAULT_PATH)
	find_library(LIBCURL_DLL_LIB libcurl.dll HINT "./external/Fronter/external/curl/lib" NO_DEFAULT_PATH)
endif (PLATFORM STREQUAL "Windows")

include_directories("./external/Fronter/external/SQLiteCpp/include")
include_directories("./external/Fronter/external/SQLiteCpp/sqlite3")
include_directories("./external/Fronter/external/commonItems/external/json/single_include")
if (PLATFORM STREQUAL "Windows")
	include_directories(${PROJECT_SOURCE_DIR}/external/Fronter/external/curl/include)
	include_directories(${PROJECT_SOURCE_DIR}/external/Fronter/external/wxWidgets/3.2.1/include)
	include_directories(${PROJECT_SOURCE_DIR}/external/Fronter/external/wxWidgets/3.2.1/include/msvc)
	ADD_DEFINITIONS(-D_WINDOWS)
	ADD_DEFINITIONS(-D__WXMSW__)
	ADD_DEFINITIONS(-DwxMSVC_VERSION_AUTO)
	ADD_DEFINITIONS(-DWXUSINGDLL)
	ADD_DEFINITIONS(-DwxMSVC_VERSION_ABI_COMPAT)
elseif (PLATFORM STREQUAL "Linux")
	find_package(CURL REQUIRED)
	find_package(wxWidgets REQUIRED COMPONENTS net core base)
	include_directories("/usr/include/wx-3.2-unofficial")
	include_directories("/usr/lib/x86_64-linux-gnu/wx/include/gtk3-unicode-3.2-unofficial")
	include(${wxWidgets_USE_FILE})
	add_compile_options(-D_FILE_OFFSET_BITS=64 -DWXUSINGDLL -D__WXGTK__ -pthread)
endif (PLATFORM STREQUAL "Windows")

file(GLOB F_MAIN_SOURCES "${FRONTER_SOURCE_DIR}/*.cpp")
file(GLOB F_CONFIGURATION_SOURCES "${FRONTER_SOURCE_DIR}/Configuration/*.cpp")
file(GLOB F_CONFIGURATION_OPTIONS_SOURCES "${FRONTER_SOURCE_DIR}/Configuration/Options/*.cpp")
file(GLOB F_FRAMES_SOURCES "${FRONTER_SOURCE_DIR}/Frames/*.cpp")
file(GLOB F_FRAMES_TABS_SOURCES "${FRONTER_SOURCE_DIR}/Frames/Tabs/*.cpp")
file(GLOB F_UPDATECHECKER_SOURCES "${FRONTER_SOURCE_DIR}/UpdateChecker/*.cpp")
file(GLOB F_UTILS_SOURCES "${FRONTER_SOURCE_DIR}/Utils/*.cpp")
file(GLOB F_UTILS_LOCALIZATION_SOURCES "${FRONTER_SOURCE_DIR}/Utils/Localization/*.cpp")
file(GLOB F_CONVERTERLAUNCHER_SOURCES "${FRONTER_SOURCE_DIR}/WorkerThreads/ConverterLauncher/*.cpp")
if (PLATFORM STREQUAL "Windows")
	list(REMOVE_ITEM F_CONVERTERLAUNCHER_SOURCES "${FRONTER_SOURCE_DIR}/WorkerThreads/ConverterLauncher/LinuxConverterLauncher.cpp")
elseif (PLATFORM STREQUAL "Linux")
	list(REMOVE_ITEM F_CONVERTERLAUNCHER_SOURCES "${FRONTER_SOURCE_DIR}/WorkerThreads/ConverterLauncher/WinConverterLauncher.cpp")
endif (PLATFORM STREQUAL "Windows")
file(GLOB F_LOGWATCHER_SOURCES "${FRONTER_SOURCE_DIR}/WorkerThreads/LogWatcher/*.cpp")
file(GLOB F_MODCOPIER_SOURCES "${FRONTER_SOURCE_DIR}/WorkerThreads/ModCopier/*.cpp")
file(GLOB F_SQLITE_SOURCES "${FRONTER_SOURCE_DIR}/../../external/SQLiteCpp/src/*.cpp")
file(GLOB F_SQLITE3_SOURCES "${FRONTER_SOURCE_DIR}/../../external/SQLiteCpp/sqlite3/*.c")

if (PLATFORM STREQUAL "Windows")
	add_executable(ConverterFrontend WIN32
		${F_MAIN_SOURCES}
		${F_CONFIGURATION_SOURCES}
		${F_CONFIGURATION_OPTIONS_SOURCES}
		${F_FRAMES_SOURCES}
		${F_FRAMES_TABS_SOURCES}
		${F_UTILS_SOURCES}
		${F_UTILS_LOCALIZATION_SOURCES}
		${F_CONVERTERLAUNCHER_SOURCES}
		${F_LOGWATCHER_SOURCES}
		${F_MODCOPIER_SOURCES}
		${F_UPDATECHECKER_SOURCES}
		${F_SQLITE_SOURCES}
		${F_SQLITE3_SOURCES}
	   )
	 set_target_properties(ConverterFrontend
		PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY ${FRONTER_OUTPUT_DIRECTORY}
	 )
elseif (PLATFORM STREQUAL "Linux")
	add_executable(ConverterFrontend
		${F_MAIN_SOURCES}
		${F_CONFIGURATION_SOURCES}
		${F_CONFIGURATION_OPTIONS_SOURCES}
		${F_FRAMES_SOURCES}
		${F_FRAMES_TABS_SOURCES}
		${F_UTILS_SOURCES}
		${F_UTILS_LOCALIZATION_SOURCES}
		${F_CONVERTERLAUNCHER_SOURCES}
		${F_LOGWATCHER_SOURCES}
		${F_MODCOPIER_SOURCES}
		${F_UPDATECHECKER_SOURCES}
		${F_SQLITE_SOURCES}
		${F_SQLITE3_SOURCES}
	)
	set_target_properties(ConverterFrontend
		PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY ${FRONTER_OUTPUT_DIRECTORY}
	)
endif (PLATFORM STREQUAL "Windows")

if (PLATFORM STREQUAL "Windows")
	target_link_libraries(ConverterFrontend LINK_PUBLIC ${LIBCURL_LIB} ${LIBCURL_DLL_LIB} commonLib) 
elseif (PLATFORM STREQUAL "Linux")
	target_link_libraries(ConverterFrontend LINK_PUBLIC ${wxWidgets_LIBRARIES} ${CURL_LIBRARIES} commonLib)
endif (PLATFORM STREQUAL "Windows")

if (PLATFORM STREQUAL "Windows" AND NOT RUN_CLANG_TIDY)
	configure_file("external/Fronter/Fronter/Resources/converter.ico" "${FRONTER_OUTPUT_DIRECTORY}/converter.ico" COPYONLY)
	configure_file("external/Fronter/Fronter/Resources/GeneralFAQ-READ.ME.FIRST.txt" "${FRONTER_OUTPUT_DIRECTORY}/GeneralFAQ-READ.ME.FIRST.txt" COPYONLY)
	configure_file("external/Fronter/Fronter/Resources/libcrypto-1_1-x64.dll" "${FRONTER_OUTPUT_DIRECTORY}/libcrypto-1_1-x64.dll" COPYONLY)
	configure_file("external/Fronter/Fronter/Resources/libssl-1_1-x64.dll" "${FRONTER_OUTPUT_DIRECTORY}/libssl-1_1-x64.dll" COPYONLY)
	configure_file("external/Fronter/external/curl/bin/libcurl-x64.dll" "${FRONTER_OUTPUT_DIRECTORY}/libcurl-x64.dll" COPYONLY)
	if (CMAKE_BUILD_TYPE STREQUAL "Release")
		configure_file("external/Fronter/external/wxWidgets/3.2.1/lib/vc14x_x64_dll/wxbase32u_vc14x_x64.dll" "${FRONTER_OUTPUT_DIRECTORY}" COPYONLY)
		configure_file("external/Fronter/external/wxWidgets/3.2.1/lib/vc14x_x64_dll/wxmsw32u_core_vc14x_x64.dll" "${FRONTER_OUTPUT_DIRECTORY}" COPYONLY)
	elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
		configure_file("external/Fronter/external/wxWidgets/3.2.1/lib/vc14x_x64_dll/wxbase32ud_vc14x_x64.dll" "${FRONTER_OUTPUT_DIRECTORY}" COPYONLY)
		configure_file("external/Fronter/external/wxWidgets/3.2.1/lib/vc14x_x64_dll/wxmsw32ud_core_vc14x_x64.dll" "${FRONTER_OUTPUT_DIRECTORY}" COPYONLY)
	endif (CMAKE_BUILD_TYPE STREQUAL "Release")
	file(GLOB FRONTER_CONFIG "data/fronter/*")
	file(COPY ${FRONTER_CONFIG} DESTINATION "${FRONTER_OUTPUT_DIRECTORY}/Configuration/")
	file(GLOB LOCALIZATIONS "external/Fronter/Fronter/Resources/*.yml")
	file(COPY ${LOCALIZATIONS} DESTINATION "${FRONTER_OUTPUT_DIRECTORY}/Configuration/")
elseif (PLATFORM STREQUAL "Linux" AND NOT RUN_CLANG_TIDY)
	add_custom_command(TARGET ConverterFrontend
		COMMAND mkdir -p "${FRONTER_OUTPUT_DIRECTORY}/Configuration/" &&
		cp -r "data/fronter/*" "${FRONTER_OUTPUT_DIRECTORY}/Configuration/" &&
		cp -r "external/Fronter/Fronter/Resources/*yml" "${FRONTER_OUTPUT_DIRECTORY}/Configuration/" &&
		cp external/Fronter/Fronter/Resources/converter.ico ${FRONTER_OUTPUT_DIRECTORY} &&
		cp external/Fronter/Fronter/Resources/GeneralFAQ-READ.ME.FIRST.txt ${FRONTER_OUTPUT_DIRECTORY}
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	)
endif (PLATFORM STREQUAL "Windows" AND NOT RUN_CLANG_TIDY)


#### Converter ####

include_directories(.)
include_directories("./external/fmt/include")
include_directories("./external/commonItems")

set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

set(PCH_SOURCES ${PCH_SOURCES} "${PROJECT_SOURCE_DIR}/external/commonItems/BulkParser.h")
set(PCH_SOURCES ${PCH_SOURCES} "${PROJECT_SOURCE_DIR}/external/commonItems/Color.h")
set(PCH_SOURCES ${PCH_SOURCES} "${PROJECT_SOURCE_DIR}/external/commonItems/CommonFunctions.h")
set(PCH_SOURCES ${PCH_SOURCES} "${PROJECT_SOURCE_DIR}/external/commonItems/CommonRegexes.h")
set(PCH_SOURCES ${PCH_SOURCES} "${PROJECT_SOURCE_DIR}/external/commonItems/ConvenientParser.h")
set(PCH_SOURCES ${PCH_SOURCES} "${PROJECT_SOURCE_DIR}/external/commonItems/ConverterVersion.h")
set(PCH_SOURCES ${PCH_SOURCES} "${PROJECT_SOURCE_DIR}/external/commonItems/Date.h")
set(PCH_SOURCES ${PCH_SOURCES} "${PROJECT_SOURCE_DIR}/external/commonItems/GameVersion.h")
set(PCH_SOURCES ${PCH_SOURCES} "${PROJECT_SOURCE_DIR}/external/commonItems/Localization/LocalizationBlock.h")
set(PCH_SOURCES ${PCH_SOURCES} "${PROJECT_SOURCE_DIR}/external/commonItems/Localization/LocalizationDatabase.h")
set(PCH_SOURCES ${PCH_SOURCES} "${PROJECT_SOURCE_DIR}/external/commonItems/Localization/LocalizationLambdas.h")
set(PCH_SOURCES ${PCH_SOURCES} "${PROJECT_SOURCE_DIR}/external/commonItems/Log.h")
set(PCH_SOURCES ${PCH_SOURCES} "${PROJECT_SOURCE_DIR}/external/commonItems/ModLoader/Mod.h")
set(PCH_SOURCES ${PCH_SOURCES} "${PROJECT_SOURCE_DIR}/external/commonItems/ModLoader/ModFilesystem.h")
set(PCH_SOURCES ${PCH_SOURCES} "${PROJECT_SOURCE_DIR}/external/commonItems/ModLoader/ModLoader.h")
set(PCH_SOURCES ${PCH_SOURCES} "${PROJECT_SOURCE_DIR}/external/commonItems/ModLoader/ModParser.h")
set(PCH_SOURCES ${PCH_SOURCES} "${PROJECT_SOURCE_DIR}/external/commonItems/OSCompatibilityLayer.h")
set(PCH_SOURCES ${PCH_SOURCES} "${PROJECT_SOURCE_DIR}/external/commonItems/Parser.h")
set(PCH_SOURCES ${PCH_SOURCES} "${PROJECT_SOURCE_DIR}/external/commonItems/ParserHelpers.h")
set(PCH_SOURCES ${PCH_SOURCES} "${PROJECT_SOURCE_DIR}/external/commonItems/StringUtils.h")
set(PCH_SOURCES ${PCH_SOURCES} "${PROJECT_SOURCE_DIR}/external/commonItems/targa.h")

set(MAIN_SOURCE "src/main.cpp")

# converter sources will be added one by one like so:
set(CONVERTER_SOURCES ${CONVERTER_SOURCES} "src/ck3_to_eu5_converter.cpp")

add_subdirectory("./external/fmt")
link_directories("./external/rakaly")

add_subdirectory(external/commonItems EXCLUDE_FROM_ALL)

if (RUN_CLANG_TIDY)
	set(CMAKE_CXX_CLANG_TIDY clang-tidy --header-filter=.*src.* --config-file=${CMAKE_SOURCE_DIR}/.clang-tidy --extra-arg=/EHsc)
endif(RUN_CLANG_TIDY)
add_library(CK3toEU5lib
	${CONVERTER_SOURCES}
 )
target_precompile_headers(CK3toEU5lib PUBLIC ${PCH_SOURCES})
if (PLATFORM STREQUAL "Windows")
	target_compile_options(CK3toEU5lib PRIVATE /permissive /W4 /w14242 /w14254 /w14263 /w14265 /w14287 /we4289 /w14296 
        /w14311 /w14545 /w14546 /w14547 /w14549 /w14555 /w14619 /w14640 /w14826 /w14905 /w14906 /w14928 
        /external:anglebrackets /external:W0 /WX)
	target_link_libraries(CK3toEU5lib LINK_PUBLIC commonLib fmt::fmt rakaly.dll.lib)
elseif (PLATFORM STREQUAL "Linux")
	target_link_libraries(CK3toEU5lib LINK_PUBLIC commonLib fmt::fmt rakaly)
endif (PLATFORM  STREQUAL "Windows")

add_executable(
	CK3toEU5
	${MAIN_SOURCE}
 )
set_target_properties( CK3toEU5
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CONVERTER_OUTPUT_DIRECTORY}
)
target_precompile_headers(CK3toEU5 REUSE_FROM CK3toEU5lib)
target_link_libraries(CK3toEU5 LINK_PUBLIC CK3toEU5lib)
if (PLATFORM STREQUAL "Windows")
	target_compile_options(CK3toEU5 PRIVATE /permissive /W4 /w14242 /w14254 /w14263 /w14265 /w14287 /we4289 /w14296 
        /w14311 /w14545 /w14546 /w14547 /w14549 /w14555 /w14619 /w14640 /w14826 /w14905 /w14906 /w14928 
        /external:anglebrackets /external:W0 /WX)
	target_link_libraries(CK3toEU5 LINK_PUBLIC commonLib rakaly.dll.lib)
elseif (PLATFORM STREQUAL "Linux")
	target_link_libraries(CK3toEU5 LINK_PUBLIC commonLib rakaly)
endif (PLATFORM  STREQUAL "Windows")

if (NOT RUN_CLANG_TIDY)
	configure_file("data/version.txt" "${FRONTER_OUTPUT_DIRECTORY}/version.txt" COPYONLY)
	configure_file("data/readme.txt" "${FRONTER_OUTPUT_DIRECTORY}/readme.txt" COPYONLY)
	file(COPY "data/blank_mod" DESTINATION "${CONVERTER_OUTPUT_DIRECTORY}")
	file(COPY "data/configurables" DESTINATION "${CONVERTER_OUTPUT_DIRECTORY}")
	if (PLATFORM STREQUAL "Windows")
		configure_file("external/rakaly/rakaly.dll" "${FRONTER_OUTPUT_DIRECTORY}/CK3toEU5/rakaly.dll" COPYONLY)
	endif (PLATFORM STREQUAL "Windows")
	add_custom_command(TARGET CK3toEU5 POST_BUILD WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} COMMAND git rev-parse HEAD > ${CONVERTER_OUTPUT_DIRECTORY}/../commit_id.txt)
endif (NOT RUN_CLANG_TIDY)


#### Tests ####

include_directories("./external/commonItems/external/googletest/googletest")
include_directories("./external/commonItems/external/googletest/googletest/include")
include_directories("./external/commonItems/external/googletest/googlemock")
include_directories("./external/commonItems/external/googletest/googlemock/include")

# test sources will be added one by one like so:
set(TEST_SOURCES ${TEST_SOURCES} "src/placeholder_tests.cpp")


add_executable(
	CK3toEU5Tests
	${TEST_SOURCES}
 )
set_target_properties(CK3toEU5Tests
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIRECTORY}
)
target_precompile_headers(CK3toEU5Tests REUSE_FROM CK3toEU5lib)
if (PLATFORM STREQUAL "Windows")
    target_compile_options(CK3toEU5Tests PRIVATE /permissive /W4 /w14242 /w14254 /w14263 /w14265 /w14287 /we4289 
        /w14296 /w14311 /w14545 /w14546 /w14547 /w14549 /w14555 /w14619 /w14640 /w14826 /w14905 /w14906 /w14928 
        /external:anglebrackets /external:W0 /WX)
elseif (PLATFORM STREQUAL "Linux")
endif (PLATFORM STREQUAL "Windows")

file(COPY "data/blank_mod" DESTINATION "${TEST_OUTPUT_DIRECTORY}")
file(COPY "data/test_files" DESTINATION "${TEST_OUTPUT_DIRECTORY}")
if (PLATFORM STREQUAL "Windows")
	configure_file("external/rakaly/rakaly.dll" "${TEST_OUTPUT_DIRECTORY}/rakaly.dll" COPYONLY)
endif (PLATFORM STREQUAL "Windows")

target_link_libraries(CK3toEU5Tests LINK_PUBLIC CK3toEU5lib GtestLib)

enable_testing()

add_test(NAME CK3toEU5Tests COMMAND CK3toEU5Tests WORKING_DIRECTORY ${TEST_OUTPUT_DIRECTORY})

####  Coverage ####

if (COVERAGE STREQUAL true)
	add_custom_target(gcov
		COMMAND mkdir -p gcoverage
	)
	add_custom_command(TARGET gcov
		COMMAND echo "=================== GCOV ===================="
		COMMAND echo "-- Source directories:"
		COMMAND echo "		${CMAKE_SOURCE_DIR}/src/"
		COMMAND echo "-- Coverage files have been output to ${CMAKE_BINARY_DIR}/gcoverage"
	)

	add_custom_target(lcov
		COMMAND mkdir -p lcoverage
	)
	add_custom_command(TARGET lcov
		COMMAND echo "=================== LCOV ===================="
		COMMAND echo "-- Passing lcov tool under code coverage"
		COMMAND lcov --c --d CMakeFiles/CK3toEU5lib.dir/src/ --d CMakeFiles/CK3toEU5Tests.dir/src/ --o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND lcov --remove lcoverage/coverage.info "/usr/include/c++/11/*" -o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND lcov --remove lcoverage/coverage.info "/usr/include/c++/11/bits/*" -o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND lcov --remove lcoverage/coverage.info "/usr/include/c++/11/ext/*" -o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND lcov --remove lcoverage/coverage.info "/usr/include/x86_64-linux-gnu/c++/11/bits/*" -o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND lcov --remove lcoverage/coverage.info "/usr/include/c++/12/*" -o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND lcov --remove lcoverage/coverage.info "/usr/include/c++/12/bits/*" -o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND lcov --remove lcoverage/coverage.info "/usr/include/c++/12/ext/*" -o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND lcov --remove lcoverage/coverage.info "/usr/include/x86_64-linux-gnu/c++/12/bits/*" -o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND lcov --remove lcoverage/coverage.info "/usr/include/c++/13/*" -o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND lcov --remove lcoverage/coverage.info "/usr/include/c++/13/bits/*" -o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND lcov --remove lcoverage/coverage.info "/usr/include/c++/13/ext/*" -o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND lcov --remove lcoverage/coverage.info "/usr/include/x86_64-linux-gnu/c++/13/bits/*" -o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND lcov --remove lcoverage/coverage.info "/usr/include/c++/14/*" -o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND lcov --remove lcoverage/coverage.info "/usr/include/c++/14/bits/*" -o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND lcov --remove lcoverage/coverage.info "/usr/include/c++/14/ext/*" -o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND lcov --remove lcoverage/coverage.info "/usr/include/x86_64-linux-gnu/c++/14/bits/*" -o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND lcov --remove lcoverage/coverage.info "/usr/lib/gcc/x86_64-linux-gnu/12/include/*" -o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND lcov --remove lcoverage/coverage.info "/usr/lib/gcc/x86_64-linux-gnu/13/include/*" -o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND lcov --remove lcoverage/coverage.info "/usr/lib/gcc/x86_64-linux-gnu/14/include/*" -o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND lcov --remove lcoverage/coverage.info '*/external/bitmap/*' -o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND lcov --remove lcoverage/coverage.info '*/external/commonItems/*' -o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND lcov --remove lcoverage/coverage.info '*/external/fmt/include/fmt/*' -o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND lcov --remove lcoverage/coverage.info '*/external/rakaly/*' -o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND lcov --remove lcoverage/coverage.info '*/external/stb/*' -o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND lcov --remove lcoverage/coverage.info '*/gmock/*' -o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND lcov --remove lcoverage/coverage.info '*/gtest/*' -o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND lcov --remove lcoverage/coverage.info '*/gtest/internal/*' -o lcoverage/coverage.info --ignore-errors inconsistent,inconsistent,unused,unused
		COMMAND echo "-- Generating HTML output files"
		COMMAND genhtml lcoverage/coverage.info --output-directory lcoverage --ignore-errors inconsistent,inconsistent,unused,unused
	)
	add_dependencies(gcov CK3toEU5Tests)

	# Make sure to clean up the coverage folder
	set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES gcoverage)

	# Create the gcov-clean target. This cleans the build as well as generated 
	# .gcda and .gcno files.
	add_custom_target(init
		COMMAND ${CMAKE_MAKE_PROGRAM} clean
		COMMAND rm -f ${TEST_OUTPUT_DIRECTORY}/*.gcno
		COMMAND rm -f ${TEST_OUTPUT_DIRECTORY}/*.gcda
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	)
endif (COVERAGE STREQUAL true)


#### Clang-tidy ####

#find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
#set(CLANG_TIDY_COMMAND "${CLANG_TIDY_EXE}")
# checks to maybe add:
#  cert-*
#  cppcoreguidelines-*,

add_library(run_clang_tidy
	${CONVERTER_SOURCES}
	${TEST_SOURCES}
 )
target_link_libraries(run_clang_tidy pthread)
set_target_properties(run_clang_tidy PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
add_dependencies(run_clang_tidy CK3toEU5 CK3toEU5Tests)