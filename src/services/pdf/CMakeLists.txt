add_library(pdf_preview_services STATIC PdfFilePreviewService.cpp)
target_include_directories(pdf_preview_services PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pdf_preview_services PUBLIC pdf_rendering Qt6::Core)
set_target_properties(pdf_preview_services PROPERTIES AUTOMOC ON)
