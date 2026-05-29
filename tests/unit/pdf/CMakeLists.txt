add_executable(PdfByteReaderTest PdfByteReaderTest.cpp)
target_link_libraries(PdfByteReaderTest PRIVATE pdf_core Qt6::Test)
add_test(NAME PdfByteReaderTest COMMAND PdfByteReaderTest)

add_executable(PdfHeaderParserTest PdfHeaderParserTest.cpp)
target_link_libraries(PdfHeaderParserTest PRIVATE pdf_core Qt6::Test)
add_test(NAME PdfHeaderParserTest COMMAND PdfHeaderParserTest)

add_executable(PdfTokenizerTest PdfTokenizerTest.cpp)
target_link_libraries(PdfTokenizerTest PRIVATE pdf_core Qt6::Test)
add_test(NAME PdfTokenizerTest COMMAND PdfTokenizerTest)

add_executable(PdfObjectParserTest PdfObjectParserTest.cpp)
target_link_libraries(PdfObjectParserTest PRIVATE pdf_core Qt6::Test)
add_test(NAME PdfObjectParserTest COMMAND PdfObjectParserTest)

add_executable(PdfObjectStoreTest PdfObjectStoreTest.cpp)
target_link_libraries(PdfObjectStoreTest PRIVATE pdf_core Qt6::Test)
add_test(NAME PdfObjectStoreTest COMMAND PdfObjectStoreTest)

add_executable(PdfXRefParserTest PdfXRefParserTest.cpp)
target_link_libraries(PdfXRefParserTest PRIVATE pdf_core Qt6::Test)
add_test(NAME PdfXRefParserTest COMMAND PdfXRefParserTest)

add_executable(PdfTrailerParserTest PdfTrailerParserTest.cpp)
target_link_libraries(PdfTrailerParserTest PRIVATE pdf_core Qt6::Test)
add_test(NAME PdfTrailerParserTest COMMAND PdfTrailerParserTest)
