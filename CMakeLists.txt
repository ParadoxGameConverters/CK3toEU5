cmake_minimum_required(VERSION 3.25)
project(CK3toEU5)

set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

SET(CONVERTER_SOURCE)

set(CMAKE_CXX_STANDARD 23)

add_executable(CK3toEU5
	"src/main.cpp"
)

set_target_properties(CK3toEU5
	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY}
)