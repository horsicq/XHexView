INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

HEADERS += \
    $$PWD/xhexview.h \
    $$PWD/xhexviewoptionswidget.h \
    $$PWD/xhexviewwidget.h \
    $$PWD/dialoghexview.h

SOURCES += \
    $$PWD/xhexview.cpp \
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

!contains(XCONFIG, dialoggotoaddress) {
    XCONFIG += dialoggotoaddress
    include($$PWD/../FormatDialogs/dialoggotoaddress.pri)
}

!contains(XCONFIG, dialoghexsignature) {
    XCONFIG += dialoghexsignature
    include($$PWD/../FormatDialogs/dialoghexsignature.pri)
}

!contains(XCONFIG, searchsignatureswidget) {
    XCONFIG += searchsignatureswidget
    include($$PWD/../FormatWidgets/SearchSignatures/searchsignatureswidget.pri)
}

!contains(XCONFIG, xoptions) {
    XCONFIG += xoptions
    include($$PWD/../XOptions/xoptions.pri)
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

DISTFILES += \
    $$PWD/LICENSE \
    $$PWD/README.md \
    $$PWD/xhexview.cmake
