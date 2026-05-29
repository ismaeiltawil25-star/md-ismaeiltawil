find_package(Qt6 REQUIRED COMPONENTS Test)

set(TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data")

function(add_unit_test name source)
    add_executable(${name} ${source})

    target_compile_definitions(${name} PRIVATE
        TEST_DATA_DIR="${TEST_DATA_DIR}"
    )

    target_link_libraries(${name} PRIVATE
        md_backpack_core Qt6::Test
    )

    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_unit_test(DocumentLoaderTest unit/DocumentLoaderTest.cpp)
add_unit_test(MarkdownHtmlRendererTest unit/MarkdownHtmlRendererTest.cpp)
add_unit_test(MarkdownPreprocessorTest unit/MarkdownPreprocessorTest.cpp)
add_unit_test(HtmlElementWrapperTest unit/HtmlElementWrapperTest.cpp)
add_unit_test(HtmlNormalizerTest unit/HtmlNormalizerTest.cpp)
add_unit_test(TaskListNormalizerTest unit/TaskListNormalizerTest.cpp)
add_unit_test(ComprehensiveRenderTest unit/ComprehensiveRenderTest.cpp)
add_unit_test(MarkdownRenderContractTest unit/MarkdownRenderContractTest.cpp)

add_executable(ReaderStylesheetBuilderTest unit/ReaderStylesheetBuilderTest.cpp)
target_link_libraries(ReaderStylesheetBuilderTest PRIVATE
    md_backpack_appearance Qt6::Test
)
add_test(NAME ReaderStylesheetBuilderTest COMMAND ReaderStylesheetBuilderTest)

add_executable(ReaderAppearanceRepositoryTest unit/ReaderAppearanceRepositoryTest.cpp)
target_link_libraries(ReaderAppearanceRepositoryTest PRIVATE
    md_backpack_appearance Qt6::Test
)
add_test(NAME ReaderAppearanceRepositoryTest COMMAND ReaderAppearanceRepositoryTest)

add_executable(ReaderAppearanceControllerTest unit/ReaderAppearanceControllerTest.cpp)
target_link_libraries(ReaderAppearanceControllerTest PRIVATE
    md_backpack_appearance Qt6::Test
)
add_test(NAME ReaderAppearanceControllerTest COMMAND ReaderAppearanceControllerTest)

add_executable(DocumentTabStateTest unit/DocumentTabStateTest.cpp)
target_include_directories(DocumentTabStateTest PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(DocumentTabStateTest PRIVATE
    Qt6::Core Qt6::Test
)
add_test(NAME DocumentTabStateTest COMMAND DocumentTabStateTest)

add_executable(DocumentTabPageTest
    unit/DocumentTabPageTest.cpp
    ${CMAKE_SOURCE_DIR}/src/app/document/DocumentTabPage.cpp
)
target_include_directories(DocumentTabPageTest PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(DocumentTabPageTest PRIVATE
    Qt6::Widgets Qt6::Test
)
add_test(NAME DocumentTabPageTest COMMAND DocumentTabPageTest)

add_unit_test(PerformanceMonitorTest unit/PerformanceMonitorTest.cpp)
add_unit_test(ErrorLoggerTest unit/ErrorLoggerTest.cpp)
add_unit_test(RenderPipelineContractTest unit/RenderPipelineContractTest.cpp)

add_subdirectory(unit/pdf)
add_subdirectory(unit/widgets)
add_subdirectory(unit/services)

add_executable(DocumentOpenPipelineTest
    unit/DocumentOpenPipelineTest.cpp
    ${CMAKE_SOURCE_DIR}/src/app/document/DocumentTabPage.cpp
    ${CMAKE_SOURCE_DIR}/src/app/document/DocumentOpenPipeline.cpp
)
target_compile_definitions(DocumentOpenPipelineTest PRIVATE
    TEST_DATA_DIR="${TEST_DATA_DIR}"
)
target_include_directories(DocumentOpenPipelineTest PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(DocumentOpenPipelineTest PRIVATE
    md_backpack_core Qt6::Widgets Qt6::Test
)
add_test(NAME DocumentOpenPipelineTest COMMAND DocumentOpenPipelineTest)
