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
