add_executable(PdfByteReaderTest PdfByteReaderTest.cpp)
target_link_libraries(PdfByteReaderTest PRIVATE pdf_core Qt6::Test)
add_test(NAME PdfByteReaderTest COMMAND PdfByteReaderTest)

add_executable(PdfHeaderParserTest PdfHeaderParserTest.cpp)
target_link_libraries(PdfHeaderParserTest PRIVATE pdf_core Qt6::Test)
add_test(NAME PdfHeaderParserTest COMMAND PdfHeaderParserTest)

add_executable(PdfTokenizerTest PdfTokenizerTest.cpp)
target_link_libraries(PdfTokenizerTest PRIVATE pdf_core Qt6::Test)
add_test(NAME PdfTokenizerTest COMMAND PdfTokenizerTest)
