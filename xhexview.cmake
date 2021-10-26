include_directories(${CMAKE_CURRENT_LIST_DIR})

include(${CMAKE_CURRENT_LIST_DIR}/../FormatDialogs/dialoggotoaddress.cmake)

set(XHEXVIEW_SOURCES
    ${DIALOGGOTOADDRESS_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/dialoghexview.cpp
    ${CMAKE_CURRENT_LIST_DIR}/dialoghexview.ui
    ${CMAKE_CURRENT_LIST_DIR}/xhexview.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xhexviewwidget.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xhexviewwidget.ui
)
