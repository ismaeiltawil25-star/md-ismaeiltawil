add_library(pdf_rendering STATIC PdfBasicPathRenderer.cpp)
target_include_directories(pdf_rendering PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pdf_rendering PUBLIC pdf_core Qt6::Gui)
set_target_properties(pdf_rendering PROPERTIES AUTOMOC ON)
