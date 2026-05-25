add_library(pdf_core INTERFACE)
target_include_directories(pdf_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pdf_core INTERFACE Qt6::Core)
