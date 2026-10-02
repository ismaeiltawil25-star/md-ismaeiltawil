cmake_minimum_required(VERSION 3.16)
# The single source of truth for the version: the Windows version resource,
# the packaging script and the release workflow all read it from here.
project(md-backpack-reader VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network PrintSupport)

include(FetchContent)
FetchContent_Declare(
    md4c
    GIT_REPOSITORY https://github.com/mity/md4c.git
    GIT_TAG release-0.5.2
)
FetchContent_MakeAvailable(md4c)

# --- Core library (no Qt Widgets dependency) ---
add_library(md_backpack_core STATIC
    src/models/MarkdownDocument.h
    src/services/DocumentLoader.h
    src/services/DocumentLoader.cpp
    src/services/MarkdownPreprocessor.h
    src/services/MarkdownPreprocessor.cpp
    src/services/SupportedFileTypes.h
    src/services/SupportedFileTypes.cpp
    src/services/text/TextDecoder.h
    src/services/text/TextDecoder.cpp
    src/services/markdown/FenceTracker.h
    src/services/markdown/FenceTracker.cpp
    src/services/markdown/FrontMatterStripper.h
    src/services/markdown/FrontMatterStripper.cpp
    src/services/markdown/DefinitionListConverter.h
    src/services/markdown/DefinitionListConverter.cpp
    src/services/markdown/FootnoteProcessor.h
    src/services/markdown/FootnoteProcessor.cpp
    src/services/markdown/InlineCodeSpans.h
    src/services/markdown/InlineCodeSpans.cpp
    src/renderers/MarkdownHtmlRenderer.h
    src/renderers/MarkdownHtmlRenderer.cpp
    src/renderers/RenderPipelineStage.h
    src/renderers/MarkdownExtensions.h
    src/renderers/extensions/HtmlEntities.h
    src/renderers/extensions/HtmlEntities.cpp
    src/renderers/extensions/HtmlTextRuns.h
    src/renderers/extensions/HtmlTextRuns.cpp
    src/renderers/extensions/EmojiShortcodes.h
    src/renderers/extensions/EmojiShortcodes.cpp
    src/renderers/extensions/InlineExtensions.h
    src/renderers/extensions/InlineExtensions.cpp
    src/renderers/extensions/LatexMathConverter.h
    src/renderers/extensions/LatexMathConverter.cpp
    src/renderers/extensions/LinkAndMathRenderers.h
    src/renderers/extensions/LinkAndMathRenderers.cpp
    src/renderers/extensions/HeadingAnchors.h
    src/renderers/extensions/HeadingAnchors.cpp
    src/renderers/extensions/AlertRenderer.h
    src/renderers/extensions/AlertRenderer.cpp
    src/renderers/extensions/SyntaxLanguages.h
    src/renderers/extensions/SyntaxLanguages.cpp
    src/renderers/extensions/SyntaxHighlighter.h
    src/renderers/extensions/SyntaxHighlighter.cpp
    src/renderers/HtmlElementWrapper.h
    src/renderers/HtmlElementWrapper.cpp
    src/renderers/HtmlNormalizer.h
    src/renderers/HtmlNormalizer.cpp
    src/renderers/HtmlRtlProcessor.h
    src/renderers/HtmlRtlProcessor.cpp
    src/renderers/TaskListNormalizer.h
    src/renderers/TaskListNormalizer.cpp
    src/utils/ErrorLogger.h
    src/utils/ErrorLogger.cpp
    src/utils/PerformanceMonitor.h
    src/utils/PerformanceMonitor.cpp
)

target_include_directories(md_backpack_core
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
    PRIVATE ${md4c_SOURCE_DIR}/src
)

target_link_libraries(md_backpack_core
    PUBLIC Qt6::Core
    PRIVATE md4c md4c-html
)

# --- Reader Appearance library (no Qt Widgets dependency) ---
add_library(md_backpack_appearance STATIC
    src/app/appearance/ReaderAppearanceSettings.h
    src/app/appearance/ReaderAppearanceDefaults.h
    src/app/appearance/ReaderTheme.h
    src/app/appearance/ReaderTheme.cpp
    src/app/appearance/ReaderStylesheetBuilder.h
    src/app/appearance/ReaderStylesheetBuilder.cpp
    src/app/appearance/ReaderAppearanceRepository.h
    src/app/appearance/ReaderAppearanceRepository.cpp
    src/app/appearance/ReaderAppearanceController.h
    src/app/appearance/ReaderAppearanceController.cpp
)

target_include_directories(md_backpack_appearance
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(md_backpack_appearance
    PUBLIC Qt6::Core
)

# --- Page layout library (page view: settings, geometry, palette, dialog) ---
add_library(md_backpack_layout STATIC
    src/app/layout/PageLayoutSettings.h
    src/app/layout/PageLayoutSettings.cpp
    src/app/layout/PageGeometry.h
    src/app/layout/PageGeometry.cpp
    src/app/layout/PageViewPalette.h
    src/app/layout/PageViewPalette.cpp
    src/app/layout/PageLayoutRepository.h
    src/app/layout/PageLayoutRepository.cpp
)

target_include_directories(md_backpack_layout
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(md_backpack_layout
    PUBLIC Qt6::Widgets
)

# --- Settings UI (Preferences window, theme controls) ---
add_library(md_backpack_settings STATIC
    src/app/settings/SettingsPage.h
    src/app/settings/SettingsDialog.h
    src/app/settings/SettingsDialog.cpp
    src/app/settings/AppearanceSettingsPage.h
    src/app/settings/AppearanceSettingsPage.cpp
    src/app/settings/PageLayoutSettingsPage.h
    src/app/settings/PageLayoutSettingsPage.cpp
    src/app/settings/ThemeIcons.h
    src/app/settings/InteractionStyle.h
    src/app/settings/InteractionStyle.cpp
    src/app/settings/ThemeIcons.cpp
)

target_include_directories(md_backpack_settings
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(md_backpack_settings
    PUBLIC md_backpack_appearance md_backpack_layout Qt6::Widgets
)

# --- Printing library (print/preview of any open document) ---
add_library(md_backpack_print STATIC
    src/app/print/PrintableContent.h
    src/app/print/DocumentPrinter.h
    src/app/print/DocumentPrinter.cpp
    src/app/print/PrintController.h
    src/app/print/PrintController.cpp
    src/app/print/PrintIcons.h
    src/app/print/PrintIcons.cpp
)

target_include_directories(md_backpack_print
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(md_backpack_print
    PUBLIC md_backpack_layout md_backpack_appearance pdf_rendering Qt6::Widgets Qt6::PrintSupport
)

# --- Application executable ---
add_executable(md-backpack-reader
    src/main.cpp
    src/app/MainWindow.h
    src/app/MainWindow.cpp
    src/app/document/DocumentTabPage.h
    src/app/document/DocumentTabPage.cpp
    src/app/document/DocumentOpenResult.h
    src/app/document/DocumentOpenPipeline.h
    src/app/document/DocumentOpenPipeline.cpp
    src/app/SingleInstanceService.h
    src/app/SingleInstanceService.cpp
    ${CMAKE_SOURCE_DIR}/deployment/windows/resources/app-icons.qrc
)

if(WIN32)
    # Product name/version embedded in the .exe (Explorer > Properties, code signing).
    configure_file(${CMAKE_SOURCE_DIR}/deployment/windows/resources/version.rc.in
                   ${CMAKE_BINARY_DIR}/version.rc @ONLY)
    set_target_properties(md-backpack-reader PROPERTIES WIN32_EXECUTABLE TRUE)
    target_sources(md-backpack-reader PRIVATE
        ${CMAKE_SOURCE_DIR}/deployment/windows/resources/md-backpack-reader.rc
        ${CMAKE_BINARY_DIR}/version.rc
    )
    if(TARGET Qt6::EntryPoint)
        target_link_libraries(md-backpack-reader PRIVATE Qt6::EntryPoint)
    endif()
endif()

target_link_libraries(md-backpack-reader PRIVATE
    md_backpack_core md_backpack_appearance md_backpack_layout md_backpack_print md_backpack_settings
    pdf_widgets pdf_preview_services
    Qt6::Widgets Qt6::Network
)

add_subdirectory(src/pdf)
add_subdirectory(src/widgets)
add_subdirectory(src/services/pdf)

# Strict warnings on all production code (see cmake/CompilerWarnings.cmake).
include(cmake/CompilerWarnings.cmake)
foreach(target IN ITEMS
        md_backpack_core md_backpack_appearance md_backpack_layout md_backpack_print md_backpack_settings md-backpack-reader
        pdf_core pdf_rendering pdf_widgets pdf_preview_services)
    mdbr_enable_warnings(${target})
endforeach()

enable_testing()
add_subdirectory(tests)
