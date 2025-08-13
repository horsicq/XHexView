INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

HEADERS += \
    $$PWD/xhexview.h \
    $$PWD/xhexviewex.h \
    $$PWD/xhexviewoptionswidget.h \
    $$PWD/xhexviewwidget.h \
    $$PWD/dialoghexview.h

SOURCES += \
    $$PWD/xhexview.cpp \
    $$PWD/xhexviewex.cpp \
    $$PWD/xhexviewoptionswidget.cpp \
    $$PWD/xhexviewwidget.cpp \
    $$PWD/dialoghexview.cpp

FORMS += \
    $$PWD/xhexviewoptionswidget.ui \
    $$PWD/xhexviewwidget.ui \
    $$PWD/dialoghexview.ui

!contains(XCONFIG, xabstracttableview) {
    XCONFIG += xabstracttableview
    include($$PWD/../Controls/xabstracttableview.pri)
}

!contains(XCONFIG, xlineedithex) {
    XCONFIG += xlineedithex
    include($$PWD/../Controls/xlineedithex.pri)
}

!contains(XCONFIG, dialogdump) {
    XCONFIG += dialogdump
    include($$PWD/../FormatDialogs/dialogdump.pri)
}

!contains(XCONFIG, dialogsearch) {
    XCONFIG += dialogsearch
    include($$PWD/../FormatDialogs/dialogsearch.pri)
}

!contains(XCONFIG, dialogshowdata) {
    XCONFIG += dialogshowdata
    include($$PWD/../FormatDialogs/dialogshowdata.pri)
}

!contains(XCONFIG, dialoggotoaddress) {
    XCONFIG += dialoggotoaddress
    include($$PWD/../FormatDialogs/dialoggotoaddress.pri)
}

!contains(XCONFIG, dialoghexsignature) {
    XCONFIG += dialoghexsignature
    include($$PWD/../FormatDialogs/dialoghexsignature.pri)
}

!contains(XCONFIG, dialogdatainspector) {
    XCONFIG += dialogdatainspector
    include($$PWD/../FormatDialogs/dialogdatainspector.pri)
}

!contains(XCONFIG, searchsignatureswidget) {
    XCONFIG += searchsignatureswidget
    include($$PWD/../FormatWidgets/SearchSignatures/searchsignatureswidget.pri)
}

!contains(XCONFIG, searchstringswidget) {
    XCONFIG += searchstringswidget
    include($$PWD/../FormatWidgets/SearchStrings/searchstringswidget.pri)
}

!contains(XCONFIG, searchvalueswidget) {
    XCONFIG += searchvalueswidget
    include($$PWD/../FormatWidgets/SearchValues/searchvalueswidget.pri)
}

!contains(XCONFIG, xoptionswidget) {
    XCONFIG += xoptionswidget
    include($$PWD/../XOptions/xoptionswidget.pri)
}

!contains(XCONFIG, xbinary) {
    XCONFIG += xbinary
    include($$PWD/../Formats/xbinary.pri)
}

!contains(XCONFIG, xhexedit) {
    XCONFIG += xhexedit
    include($$PWD/../XHexEdit/xhexedit.pri)
}

!contains(XCONFIG, xinfodb) {
    XCONFIG += xinfodb
    include($$PWD/../XInfoDB/xinfodb.pri)
}

!contains(XCONFIG, die_widget) {
    XCONFIG += die_widget
    include($$PWD/../die_widget/die_widget.pri)
}

!contains(XCONFIG, xsymbolswidget) {
    XCONFIG += xsymbolswidget
    include($$PWD/../XSymbolsWidget/xsymbolswidget.pri)
}

!contains(XCONFIG, xdataconvertorwidget) {
    XCONFIG += xdataconvertorwidget
    include($$PWD/../XDataConvertorWidget/xdataconvertorwidget.pri)
}

!contains(XCONFIG, xvisualizationwidget) {
    XCONFIG += xvisualizationwidget
    include($$PWD/../XVisualizationWidget/xvisualizationwidget.pri)
}

!contains(XCONFIG, xstructwidget) {
    XCONFIG += xstructwidget
    include($$PWD/../FormatWidgets/XAbstractWidgets/xstructwidget.pri)
}

!contains(XCONFIG, dialogwidget) {
    XCONFIG += dialogwidget
    include($$PWD/../FormatDialogs/dialogwidget.pri)
}

!contains(XCONFIG, xregionswidget) {
    XCONFIG += xregionswidget
    include($$PWD/../XRegionsWidget/xregionswidget.pri)
}

DISTFILES += \
    $$PWD/LICENSE \
    $$PWD/README.md \
    $$PWD/xhexview.cmake
