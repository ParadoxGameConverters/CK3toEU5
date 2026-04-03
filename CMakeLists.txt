cmake_minimum_required(VERSION 3.20)
project(CK3toEU5 VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    set(RAKALY_LIBRARY "${CMAKE_SOURCE_DIR}/external/rakaly/rakaly.dll.lib")
    set(RAKALY_RUNTIME "${CMAKE_SOURCE_DIR}/external/rakaly/rakaly.dll")
elseif(APPLE)
    set(RAKALY_LIBRARY "${CMAKE_SOURCE_DIR}/external/rakaly/librakaly.dylib")
    set(RAKALY_RUNTIME "${CMAKE_SOURCE_DIR}/external/rakaly/librakaly.dylib")
else()
    set(RAKALY_LIBRARY "${CMAKE_SOURCE_DIR}/external/rakaly/librakaly.so")
    set(RAKALY_RUNTIME "${CMAKE_SOURCE_DIR}/external/rakaly/librakaly.so")
endif()

add_library(ck3_to_eu5_lib
    src/ck3/installed_world_enricher.cpp
    src/ck3/installed_titles.cpp
    src/ck3/input_reader.cpp
    src/ck3/raw_save_normalizer.cpp
    src/ck3/realm_normalizer.cpp
    src/ck3/world.cpp
    src/ck3/world_importer.cpp
    src/common/csv_reader.cpp
    src/common/filesystem_utils.cpp
    src/common/logger.cpp
    src/common/pds_node.cpp
    src/common/pds_parser.cpp
    src/common/process.cpp
    src/common/string_utils.cpp
    src/config/configuration.cpp
    src/config/configuration_loader.cpp
    src/convert/country_context.cpp
    src/convert/diplomacy_converter.cpp
    src/convert/economy_converter.cpp
    src/convert/military_converter.cpp
    src/convert/pop_synthesizer.cpp
    src/convert/tag_generator.cpp
    src/convert/war_converter.cpp
    src/convert/world_converter.cpp
    src/diagnostics/diagnostics_report.cpp
    src/eu5/framework.cpp
    src/eu5/installed_definitions_loader.cpp
    src/eu5/world_sanitizer.cpp
    src/eu5/world_validator.cpp
    src/eu5/framework_builder.cpp
    src/eu5/installed_data_extractor.cpp
    src/maps/map_correspondence.cpp
    src/mappers/bootstrap_province_mapping_generator.cpp
    src/mappers/mapper_bundle.cpp
    src/mappers/province_matcher.cpp
    src/output/eu5_outputter.cpp
    external/lodepng/lodepng.cpp
)

target_include_directories(ck3_to_eu5_lib PUBLIC src external/rakaly external/lodepng)

if (MSVC)
    target_compile_options(ck3_to_eu5_lib PRIVATE /W4)
else()
    target_compile_options(ck3_to_eu5_lib PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(ck3_to_eu5_lib PRIVATE "${RAKALY_LIBRARY}")

add_executable(ck3_to_eu5 src/main.cpp)
target_link_libraries(ck3_to_eu5 PRIVATE ck3_to_eu5_lib)

add_executable(ck3_to_eu5_normalize src/normalizer_main.cpp)
target_link_libraries(ck3_to_eu5_normalize PRIVATE ck3_to_eu5_lib)

add_executable(ck3_to_eu5_extract_eu5_framework src/extract_eu5_main.cpp)
target_link_libraries(ck3_to_eu5_extract_eu5_framework PRIVATE ck3_to_eu5_lib)

add_executable(ck3_to_eu5_generate_province_mappings src/generate_province_mappings_main.cpp)
target_link_libraries(ck3_to_eu5_generate_province_mappings PRIVATE ck3_to_eu5_lib)

add_executable(ck3_to_eu5_measure_mapping_coverage src/measure_mapping_coverage_main.cpp)
target_link_libraries(ck3_to_eu5_measure_mapping_coverage PRIVATE ck3_to_eu5_lib)

add_executable(ck3_to_eu5_generate_map_correspondence src/generate_map_correspondence_main.cpp)
target_link_libraries(ck3_to_eu5_generate_map_correspondence PRIVATE ck3_to_eu5_lib)

enable_testing()
add_executable(ck3_to_eu5_smoke_test tests/smoke_test.cpp)
target_link_libraries(ck3_to_eu5_smoke_test PRIVATE ck3_to_eu5_lib)
add_test(NAME smoke_test COMMAND ck3_to_eu5_smoke_test)

add_executable(ck3_to_eu5_real_save_regression_test tests/real_save_regression_test.cpp)
target_link_libraries(ck3_to_eu5_real_save_regression_test PRIVATE ck3_to_eu5_lib)
add_test(NAME real_save_regression_test COMMAND ck3_to_eu5_real_save_regression_test)

if(WIN32)
    add_custom_command(TARGET ck3_to_eu5 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${RAKALY_RUNTIME}" $<TARGET_FILE_DIR:ck3_to_eu5>)
    add_custom_command(TARGET ck3_to_eu5_normalize POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${RAKALY_RUNTIME}" $<TARGET_FILE_DIR:ck3_to_eu5_normalize>)
    add_custom_command(TARGET ck3_to_eu5_extract_eu5_framework POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${RAKALY_RUNTIME}" $<TARGET_FILE_DIR:ck3_to_eu5_extract_eu5_framework>)
    add_custom_command(TARGET ck3_to_eu5_generate_province_mappings POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${RAKALY_RUNTIME}" $<TARGET_FILE_DIR:ck3_to_eu5_generate_province_mappings>)
    add_custom_command(TARGET ck3_to_eu5_measure_mapping_coverage POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${RAKALY_RUNTIME}" $<TARGET_FILE_DIR:ck3_to_eu5_measure_mapping_coverage>)
    add_custom_command(TARGET ck3_to_eu5_generate_map_correspondence POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${RAKALY_RUNTIME}" $<TARGET_FILE_DIR:ck3_to_eu5_generate_map_correspondence>)
    add_custom_command(TARGET ck3_to_eu5_smoke_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${RAKALY_RUNTIME}" $<TARGET_FILE_DIR:ck3_to_eu5_smoke_test>)
    add_custom_command(TARGET ck3_to_eu5_real_save_regression_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${RAKALY_RUNTIME}" $<TARGET_FILE_DIR:ck3_to_eu5_real_save_regression_test>)
endif()
