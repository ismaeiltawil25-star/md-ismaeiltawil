add_library(pdf_widgets STATIC
    PdfSinglePagePreviewWidget.h PdfSinglePagePreviewWidget.cpp
    PdfSinglePagePreviewController.h PdfSinglePagePreviewController.cpp
    PdfZoomToolbar.h PdfZoomToolbar.cpp
)
target_include_directories(pdf_widgets PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pdf_widgets PUBLIC Qt6::Widgets pdf_rendering)
set_target_properties(pdf_widgets PROPERTIES AUTOMOC ON)
