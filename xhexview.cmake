include_directories(${CMAKE_CURRENT_LIST_DIR})

if (NOT DEFINED DIALOGGOTOADDRESS_SOURCES)
    include(${CMAKE_CURRENT_LIST_DIR}/../FormatDialogs/dialoggotoaddress.cmake)
    set(XHEXVIEW_SOURCES ${XHEXVIEW_SOURCES} ${DIALOGGOTOADDRESS_SOURCES})
endif()

include(${CMAKE_CURRENT_LIST_DIR}/../Controls/xabstracttableview.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../Controls/xlineedithex.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../FormatDialogs/dialogdump.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../FormatDialogs/dialogsearch.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../FormatDialogs/dialogshowdata.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../FormatDialogs/dialoggotoaddress.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../FormatDialogs/dialoghexsignature.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../FormatDialogs/dialogdatainspector.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../FormatWidgets/SearchSignatures/searchsignatureswidget.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../FormatWidgets/SearchStrings/searchstringswidget.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../FormatWidgets/SearchValues/searchvalueswidget.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../XOptions/xoptionswidget.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../Formats/xbinary.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../XDataConvertorWidget/xdataconvertorwidget.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../XHexEdit/xhexedit.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../XSymbolsWidget/xsymbolswidget.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../die_widget/die_widget.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../XVisualizationWidget/xvisualizationwidget.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../FormatDialogs/dialogwidget.cmake)

set(XHEXVIEW_SOURCES
    ${XHEXVIEW_SOURCES}
    ${ABSTRACTWIDGETS_SOURCES}
    ${XABSTRACTTABLEVIEW_SOURCES}
    ${XLINEEDITHEX_SOURCES}
    ${DIALOGDUMP_SOURCES}
    ${DIALOGSEARCH_SOURCES}
    ${DIALOGSHOWDATA_SOURCES}
    ${DIALOGHEXSIGNATURE_SOURCES}
    ${DIALOGDATAINSPECTOR_SOURCES}
    ${SEARCHSIGNATURESWIDGET_SOURCES}
    ${SEARCHSTRINGSWIDGET_SOURCES}
    ${SEARCHVALUESWIDGET_SOURCES}
    ${XOPTIONSWIDGET_SOURCES}
    ${FORMATS_SOURCES}
    ${XHEXEDIT_SOURCES}
    ${XSYMBOLSWIDGET_SOURCES}
    ${DIE_WIDGET_SOURCES}
    ${XDATACONVERTORWIDGET_SOURCES}
    ${XVISUALIZATIONWIDGET_SOURCES}
    ${DIALOGWIDGET_SOURCES}
)

set(XHEXVIEW_SOURCES
    ${XHEXVIEW_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/dialoghexview.cpp
    ${CMAKE_CURRENT_LIST_DIR}/dialoghexview.h
    ${CMAKE_CURRENT_LIST_DIR}/dialoghexview.ui
    ${CMAKE_CURRENT_LIST_DIR}/xhexview.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xhexview.h
    ${CMAKE_CURRENT_LIST_DIR}/xhexviewoptionswidget.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xhexviewoptionswidget.h
    ${CMAKE_CURRENT_LIST_DIR}/xhexviewoptionswidget.ui
    ${CMAKE_CURRENT_LIST_DIR}/xhexviewwidget.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xhexviewwidget.h
    ${CMAKE_CURRENT_LIST_DIR}/xhexviewwidget.ui
)

