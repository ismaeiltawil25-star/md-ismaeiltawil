add_library(pdf_core STATIC
    PdfHeaderParser.cpp
    PdfTokenizer.cpp
    PdfObjectParser.cpp
    PdfObjectStore.cpp
    PdfXRefParser.cpp
    PdfTrailerParser.cpp
)
target_include_directories(pdf_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pdf_core PUBLIC Qt6::Core)
set_target_properties(pdf_core PROPERTIES AUTOMOC ON)
