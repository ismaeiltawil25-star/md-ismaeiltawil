cmake_minimum_required(VERSION 3.16)
project(md-backpack-reader VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

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
    src/services/text/TextDecoder.h
    src/services/text/TextDecoder.cpp
    src/services/markdown/FenceTracker.h
    src/services/markdown/FenceTracker.cpp
    src/services/markdown/FrontMatterStripper.h
    src/services/markdown/FrontMatterStripper.cpp
    src/services/markdown/DefinitionListConverter.h
    src/services/markdown/DefinitionListConverter.cpp
    src/renderers/MarkdownHtmlRenderer.h
    src/renderers/MarkdownHtmlRenderer.cpp
    src/renderers/RenderPipelineStage.h
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
    src/app/appearance/ReaderAppearanceDialog.h
    src/app/appearance/ReaderAppearanceDialog.cpp
    ${CMAKE_SOURCE_DIR}/deployment/windows/resources/app-icons.qrc
)

if(WIN32)
    set_target_properties(md-backpack-reader PROPERTIES WIN32_EXECUTABLE TRUE)
    target_sources(md-backpack-reader PRIVATE
        ${CMAKE_SOURCE_DIR}/deployment/windows/resources/md-backpack-reader.rc
    )
    if(TARGET Qt6::EntryPoint)
        target_link_libraries(md-backpack-reader PRIVATE Qt6::EntryPoint)
    endif()
endif()

target_link_libraries(md-backpack-reader PRIVATE
    md_backpack_core md_backpack_appearance
    pdf_widgets pdf_preview_services
    Qt6::Widgets Qt6::Network
)

add_subdirectory(src/pdf)
add_subdirectory(src/widgets)
add_subdirectory(src/services/pdf)

# Strict warnings on all production code (see cmake/CompilerWarnings.cmake).
include(cmake/CompilerWarnings.cmake)
foreach(target IN ITEMS
        md_backpack_core md_backpack_appearance md-backpack-reader
        pdf_core pdf_rendering pdf_widgets pdf_preview_services)
    mdbr_enable_warnings(${target})
endforeach()

enable_testing()
add_subdirectory(tests)
