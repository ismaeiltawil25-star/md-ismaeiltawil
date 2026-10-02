find_package(Qt6 REQUIRED COMPONENTS Test)

set(TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/helpers)

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
add_unit_test(TextDecoderTest unit/TextDecoderTest.cpp)
add_unit_test(FenceTrackerTest unit/FenceTrackerTest.cpp)
add_unit_test(MarkdownExtensionUnitsTest unit/MarkdownExtensionUnitsTest.cpp)
add_unit_test(SupportedFileTypesTest unit/SupportedFileTypesTest.cpp)

# Pipeline tests that load rendered HTML into a QTextDocument (needs Qt Gui).
# Run headless so they also work on CI agents without a desktop session.
function(add_render_test name source)
    add_unit_test(${name} ${source})
    target_link_libraries(${name} PRIVATE md_backpack_appearance Qt6::Gui)
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

add_render_test(RenderedDocumentContractTest unit/RenderedDocumentContractTest.cpp)
add_render_test(RenderRobustnessTest unit/RenderRobustnessTest.cpp)
add_render_test(MarkdownFeatureCoverageTest unit/MarkdownFeatureCoverageTest.cpp)

# Rules the compiler cannot check (e.g. no raw non-ASCII in string literals).
add_executable(SourceHygieneTest unit/SourceHygieneTest.cpp)
target_compile_definitions(SourceHygieneTest PRIVATE SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_link_libraries(SourceHygieneTest PRIVATE Qt6::Core Qt6::Test)
add_test(NAME SourceHygieneTest COMMAND SourceHygieneTest)

# Page view: settings/geometry/palette unit tests + widget rendering test.
add_executable(PageLayoutTest unit/PageLayoutTest.cpp)
target_link_libraries(PageLayoutTest PRIVATE md_backpack_layout Qt6::Test)
add_test(NAME PageLayoutTest COMMAND PageLayoutTest)

add_executable(PageViewRenderingTest
    unit/PageViewRenderingTest.cpp
    ${CMAKE_SOURCE_DIR}/src/app/document/DocumentTabPage.cpp
)
target_compile_definitions(PageViewRenderingTest PRIVATE TEST_DATA_DIR="${TEST_DATA_DIR}")
target_link_libraries(PageViewRenderingTest PRIVATE
    md_backpack_core md_backpack_appearance md_backpack_layout Qt6::Widgets Qt6::Test
)
add_test(NAME PageViewRenderingTest COMMAND PageViewRenderingTest)

# Printing: prints to PDF files and checks the result with the project's PDF engine.
add_executable(DocumentPrinterTest unit/DocumentPrinterTest.cpp)
target_compile_definitions(DocumentPrinterTest PRIVATE TEST_DATA_DIR="${TEST_DATA_DIR}")
target_link_libraries(DocumentPrinterTest PRIVATE md_backpack_print md_backpack_core Qt6::Test)
add_test(NAME DocumentPrinterTest COMMAND DocumentPrinterTest)
set_tests_properties(DocumentPrinterTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
set_tests_properties(PageViewRenderingTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
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
    md_backpack_layout Qt6::Widgets Qt6::Test
)
add_test(NAME DocumentTabPageTest COMMAND DocumentTabPageTest)

add_unit_test(PerformanceMonitorTest unit/PerformanceMonitorTest.cpp)
add_unit_test(ErrorLoggerTest unit/ErrorLoggerTest.cpp)
add_unit_test(RenderPipelineContractTest unit/RenderPipelineContractTest.cpp)

add_subdirectory(unit/pdf)
add_subdirectory(unit/widgets)
add_subdirectory(unit/services)

add_executable(MainWindowPdfIntegrationTest
    unit/MainWindowPdfIntegrationTest.cpp
    ${CMAKE_SOURCE_DIR}/src/app/MainWindow.cpp
    ${CMAKE_SOURCE_DIR}/src/app/document/DocumentTabPage.cpp
    ${CMAKE_SOURCE_DIR}/src/app/document/DocumentOpenPipeline.cpp
    ${CMAKE_SOURCE_DIR}/src/app/appearance/ReaderAppearanceDialog.cpp
)
target_include_directories(MainWindowPdfIntegrationTest PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(MainWindowPdfIntegrationTest PRIVATE
    md_backpack_core md_backpack_appearance md_backpack_layout md_backpack_print
    pdf_widgets pdf_preview_services pdf_rendering pdf_core
    Qt6::Widgets Qt6::Test Qt6::Network
)
add_test(NAME MainWindowPdfIntegrationTest COMMAND MainWindowPdfIntegrationTest)

add_executable(MainWindowPageViewTest
    unit/MainWindowPageViewTest.cpp
    ${CMAKE_SOURCE_DIR}/src/app/MainWindow.cpp
    ${CMAKE_SOURCE_DIR}/src/app/document/DocumentTabPage.cpp
    ${CMAKE_SOURCE_DIR}/src/app/document/DocumentOpenPipeline.cpp
    ${CMAKE_SOURCE_DIR}/src/app/appearance/ReaderAppearanceDialog.cpp
)
target_compile_definitions(MainWindowPageViewTest PRIVATE TEST_DATA_DIR="${TEST_DATA_DIR}")
target_include_directories(MainWindowPageViewTest PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(MainWindowPageViewTest PRIVATE
    md_backpack_core md_backpack_appearance md_backpack_layout md_backpack_print
    pdf_widgets pdf_preview_services pdf_rendering pdf_core
    Qt6::Widgets Qt6::Test Qt6::Network
)
add_test(NAME MainWindowPageViewTest COMMAND MainWindowPageViewTest)

add_executable(MainWindowLinkNavigationTest
    unit/MainWindowLinkNavigationTest.cpp
    ${CMAKE_SOURCE_DIR}/src/app/MainWindow.cpp
    ${CMAKE_SOURCE_DIR}/src/app/document/DocumentTabPage.cpp
    ${CMAKE_SOURCE_DIR}/src/app/document/DocumentOpenPipeline.cpp
    ${CMAKE_SOURCE_DIR}/src/app/appearance/ReaderAppearanceDialog.cpp
)
target_include_directories(MainWindowLinkNavigationTest PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(MainWindowLinkNavigationTest PRIVATE
    md_backpack_core md_backpack_appearance md_backpack_layout md_backpack_print
    pdf_widgets pdf_preview_services pdf_rendering pdf_core
    Qt6::Widgets Qt6::Test Qt6::Network
)
add_test(NAME MainWindowLinkNavigationTest COMMAND MainWindowLinkNavigationTest)

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
    md_backpack_core md_backpack_layout Qt6::Widgets Qt6::Test
)
add_test(NAME DocumentOpenPipelineTest COMMAND DocumentOpenPipelineTest)

add_executable(PdfDiagnostic diagnostic/PdfDiagnostic.cpp)
target_include_directories(PdfDiagnostic PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(PdfDiagnostic PRIVATE
    pdf_rendering pdf_core Qt6::Core Qt6::Widgets
)

add_executable(WidgetDisplayDiagnostic diagnostic/WidgetDisplayDiagnostic.cpp
    ${CMAKE_SOURCE_DIR}/src/app/MainWindow.cpp
    ${CMAKE_SOURCE_DIR}/src/app/document/DocumentTabPage.cpp
    ${CMAKE_SOURCE_DIR}/src/app/document/DocumentOpenPipeline.cpp
    ${CMAKE_SOURCE_DIR}/src/app/appearance/ReaderAppearanceDialog.cpp
)
target_include_directories(WidgetDisplayDiagnostic PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(WidgetDisplayDiagnostic PRIVATE
    md_backpack_core md_backpack_appearance md_backpack_layout md_backpack_print
    pdf_widgets pdf_preview_services pdf_rendering pdf_core
    Qt6::Widgets Qt6::Test Qt6::Network
)
