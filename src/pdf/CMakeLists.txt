add_library(pdf_core STATIC
    PdfHeaderParser.cpp
    PdfTokenizer.cpp
    PdfObjectParser.cpp
    PdfObjectStore.cpp
    PdfXRefParser.cpp
    PdfTrailerParser.cpp
    PdfIndirectObjectLoader.cpp
    PdfCatalogResolver.cpp
    PdfPageTreeResolver.cpp
    PdfDocumentLoader.cpp
    PdfStreamBodyReader.cpp
    PdfStreamObjectReader.cpp
    PdfRawStreamStore.cpp
    PdfStreamDecoder.cpp
    PdfPageContentStreamResolver.cpp
    PdfContentStreamTokenizer.cpp
    PdfContentStreamInterpreter.cpp
    PdfGraphicsStateTracker.cpp
    PdfPathRecorder.cpp
)
target_include_directories(pdf_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pdf_core PUBLIC Qt6::Core)

find_package(Qt6 QUIET COMPONENTS ZlibPrivate)
if (NOT TARGET Qt6::ZlibPrivate)
    message(FATAL_ERROR "Qt6::ZlibPrivate is required. Install Qt6 with Zlib support.")
endif()
target_link_libraries(pdf_core PRIVATE Qt6::ZlibPrivate)

set_target_properties(pdf_core PROPERTIES AUTOMOC ON)
