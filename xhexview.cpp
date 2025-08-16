/* Copyright (c) 2020-2025 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xhexview.h"

XHexView::XHexView(QWidget *pParent) : XDeviceTableEditView(pParent)
{
    addShortcut(X_ID_HEX_DATA_INSPECTOR, this, SLOT(_dataInspector()));
    addShortcut(X_ID_HEX_DATA_CONVERTOR, this, SLOT(_dataConvertor()));
    addShortcut(X_ID_HEX_MULTISEARCH, this, SLOT(_multisearch()));
    addShortcut(X_ID_HEX_GOTO_OFFSET, this, SLOT(_goToOffsetSlot()));
    addShortcut(X_ID_HEX_GOTO_ADDRESS, this, SLOT(_goToAddressSlot()));
    addShortcut(X_ID_HEX_DUMPTOFILE, this, SLOT(_dumpToFileSlot()));
    addShortcut(X_ID_HEX_SELECT_ALL, this, SLOT(_selectAllSlot()));
    addShortcut(X_ID_HEX_COPY_DATA, this, SLOT(_copyDataSlot()));
    addShortcut(X_ID_HEX_COPY_OFFSET, this, SLOT(_copyOffsetSlot()));
    addShortcut(X_ID_HEX_COPY_ADDRESS, this, SLOT(_copyAddressSlot()));
    addShortcut(X_ID_HEX_FIND_STRING, this, SLOT(_findStringSlot()));
    addShortcut(X_ID_HEX_FIND_SIGNATURE, this, SLOT(_findSignatureSlot()));
    addShortcut(X_ID_HEX_FIND_VALUE, this, SLOT(_findValueSlot()));
    addShortcut(X_ID_HEX_FIND_NEXT, this, SLOT(_findNextSlot()));
    addShortcut(X_ID_HEX_SIGNATURE, this, SLOT(_hexSignatureSlot()));
    addShortcut(X_ID_HEX_FOLLOWIN_DISASM, this, SLOT(_disasmSlot()));
    addShortcut(X_ID_HEX_FOLLOWIN_MEMORYMAP, this, SLOT(_memoryMapSlot()));
    addShortcut(X_ID_HEX_FOLLOWIN_HEX, this, SLOT(_mainHexSlot()));
    addShortcut(X_ID_HEX_EDIT_HEX, this, SLOT(_editHex()));
    addShortcut(X_ID_HEX_EDIT_REMOVE, this, SLOT(_editRemove()));
    addShortcut(X_ID_HEX_EDIT_RESIZE, this, SLOT(_editResize()));

    g_nBytesProLine = 16;  // Default
    g_nElementByteSize = 1;
    g_nSymbolByteSize = 1;
    _setMode(ELEMENT_MODE_HEX);
    g_nDataBlockSize = 0;
    g_nViewStartDelta = 0;
    //    g_smode=SMODE_ANSI;  // TODO Set/Get
    g_nThisBase = 0;
    g_hexOptions = {};
    g_nAddressWidth = 8;         // TODO Set/Get
    g_bIsLocationColon = false;  // TODO Check
                                 //    g_nPieceSize=1; // TODO

    addColumn(tr("Address"), 0, true);
    addColumn(tr("Hex"), 0, true);
    addColumn(tr("Symbols"), 0, true);

    setTextFont(XOptions::getMonoFont());  // mb TODO move to XDeviceTableView !!!
                                           //    setBlinkingCursorEnable(true);
    // setBlinkingCursorEnable(false);
    g_sCodePage = "";
#if (QT_VERSION_MAJOR < 6) || defined(QT_CORE5COMPAT_LIB)
    g_pCodePageMenu = g_xCodePageOptions.createCodePagesMenu(this, true);
    connect(&g_xCodePageOptions, SIGNAL(setCodePage(QString)), this, SLOT(_setCodePage(QString)));
#endif
    setLocationMode(LOCMODE_OFFSET);
    setMapEnable(true);
    setMapWidth(20);

    // g_pixmapCache.setCacheLimit(1024);
    setVerticalLinesVisible(false);
}

void XHexView::adjustView()
{
    setTextFontFromOptions(XOptions::ID_HEX_FONT);

    g_bIsLocationColon = getGlobalOptions()->getValue(XOptions::ID_HEX_LOCATIONCOLON).toBool();

    viewport()->update();
}

void XHexView::_adjustView()
{
    adjustView();

    if (getDevice()) {
        reload(true);
    }
}

void XHexView::setData(QIODevice *pDevice, const OPTIONS &options, bool bReload)
{
    g_hexOptions = options;

    bool bReadOnly = false;

    if (pDevice) {
        bReadOnly = !(pDevice->isWritable());
    }

    setReadonly(bReadOnly);

    setDevice(pDevice, options.nStartOffset, options.nTotalSize);

    adjustView();
    adjustMap();

    // setMemoryMap(options.memoryMapRegion);

    //    resetCursorData();

    setLocationMode(options.addressMode);

    adjustHeader();
    adjustColumns();
    adjustScrollCount();

    if ((options.nStartSelectionOffset > 0) && (options.nStartSelectionOffset != -1)) {
        _goToViewPos(options.nStartSelectionOffset);
    }

    _initSetSelection(options.nStartSelectionOffset, options.nSizeOfSelection);
    //    setCursorViewPos(options.nStartSelectionOffset, COLUMN_HEX);

    if (bReload) {
        reload(true);
    }
}

void XHexView::goToOffset(qint64 nOffset)
{
    qint64 nViewPos = deviceOffsetToViewPos(nOffset);
    _goToViewPos(nViewPos);
}

// XADDR XHexView::getStartLocation()
// {
//     return g_hexOptions.nStartLocation;
// }

// XADDR XHexView::getSelectionInitLocation()
// {
//     return getSelectionInitOffset() + g_hexOptions.nStartLocation;
// }

void XHexView::setBytesProLine(qint32 nBytesProLine)
{
    g_nBytesProLine = nBytesProLine;
    adjustScrollCount();
    adjustView();
}

QList<XShortcuts::MENUITEM> XHexView::getMenuItems()
{
    QList<XShortcuts::MENUITEM> listResults;

    STATE menuState = getState();

    if (menuState.nSelectionViewSize) {
        getShortcuts()->_addMenuItem_Checked(&listResults, X_ID_HEX_DATA_INSPECTOR, this, SLOT(_dataInspector()), XShortcuts::GROUPID_NONE,
                                             getViewWidgetState(VIEWWIDGET_DATAINSPECTOR));
        getShortcuts()->_addMenuItem_Checked(&listResults, X_ID_HEX_DATA_CONVERTOR, this, SLOT(_dataConvertor()), XShortcuts::GROUPID_NONE,
                                             getViewWidgetState(VIEWWIDGET_DATACONVERTOR));
        getShortcuts()->_addMenuSeparator(&listResults, XShortcuts::GROUPID_NONE);
    }

    getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_GOTO_OFFSET, this, SLOT(_goToOffsetSlot()), XShortcuts::GROUPID_GOTO);
    getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_GOTO_ADDRESS, this, SLOT(_goToAddressSlot()), XShortcuts::GROUPID_GOTO);

    if (menuState.nSelectionViewSize) {
        getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_GOTO_SELECTION_START, this, SLOT(_goToSelectionStart()),
                                     (XShortcuts::GROUPID_SELECTION << 8) | XShortcuts::GROUPID_GOTO);
        getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_GOTO_SELECTION_END, this, SLOT(_goToSelectionEnd()),
                                     (XShortcuts::GROUPID_SELECTION << 8) | XShortcuts::GROUPID_GOTO);
    }

    getShortcuts()->_addMenuItem_Checked(&listResults, X_ID_HEX_MULTISEARCH, this, SLOT(_multisearch()), XShortcuts::GROUPID_NONE,
                                         getViewWidgetState(VIEWWIDGET_MULTISEARCH));

    if (menuState.nSelectionViewSize) {
        getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_DUMPTOFILE, this, SLOT(_dumpToFileSlot()), XShortcuts::GROUPID_NONE);
        getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_SIGNATURE, this, SLOT(_hexSignatureSlot()), XShortcuts::GROUPID_NONE);
    }

    getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_FIND_STRING, this, SLOT(_findStringSlot()), XShortcuts::GROUPID_FIND);
    getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_FIND_SIGNATURE, this, SLOT(_findSignatureSlot()), XShortcuts::GROUPID_FIND);
    getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_FIND_VALUE, this, SLOT(_findValueSlot()), XShortcuts::GROUPID_FIND);
    getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_FIND_NEXT, this, SLOT(_findNextSlot()), XShortcuts::GROUPID_FIND);

    getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_SELECT_ALL, this, SLOT(_selectAllSlot()), XShortcuts::GROUPID_SELECT);

    getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_COPY_OFFSET, this, SLOT(_copyOffsetSlot()), XShortcuts::GROUPID_COPY);
    getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_COPY_ADDRESS, this, SLOT(_copyAddressSlot()), XShortcuts::GROUPID_COPY);
    getShortcuts()->_addMenuSeparator(&listResults, XShortcuts::GROUPID_COPY);
    getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_COPY_DATA, this, SLOT(_copyDataSlot()), XShortcuts::GROUPID_COPY);

    getShortcuts()->_addMenuItem_Checked(&listResults, X_ID_HEX_STRINGS, this, SLOT(_strings()), XShortcuts::GROUPID_NONE, getViewWidgetState(VIEWWIDGET_STRINGS));
    getShortcuts()->_addMenuItem_Checked(&listResults, X_ID_HEX_VISUALIZATION, this, SLOT(_visualization()), XShortcuts::GROUPID_NONE,
                                         getViewWidgetState(VIEWWIDGET_VISUALIZATION));

    getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_BOOKMARKS_NEW, this, SLOT(_bookmarkNew()), XShortcuts::GROUPID_BOOKMARKS);
    getShortcuts()->_addMenuItem_Checked(&listResults, X_ID_HEX_BOOKMARKS_LIST, this, SLOT(_bookmarkList()), XShortcuts::GROUPID_BOOKMARKS,
                                         getViewWidgetState(VIEWWIDGET_BOOKMARKS));

    if (g_hexOptions.bMenu_Disasm) {
        getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_FOLLOWIN_DISASM, this, SLOT(_disasmSlot()), XShortcuts::GROUPID_FOLLOWIN);
    }

    if (g_hexOptions.bMenu_MemoryMap) {
        getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_FOLLOWIN_MEMORYMAP, this, SLOT(_memoryMapSlot()), XShortcuts::GROUPID_FOLLOWIN);
    }

    if (g_hexOptions.bMenu_MainHex) {
        getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_FOLLOWIN_HEX, this, SLOT(_mainHexSlot()), XShortcuts::GROUPID_FOLLOWIN);
    }

    if (!isReadonly()) {
        if (menuState.nSelectionViewSize) {
            getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_EDIT_HEX, this, SLOT(_editHex()), XShortcuts::GROUPID_EDIT);
        }
        getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_EDIT_PATCH, this, SLOT(_editPatch()), XShortcuts::GROUPID_EDIT);

        if (XBinary::isResizeEnable(getDevice())) {
            getShortcuts()->_addMenuSeparator(&listResults, XShortcuts::GROUPID_EDIT);
            getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_EDIT_REMOVE, this, SLOT(_editRemove()), XShortcuts::GROUPID_EDIT);
            getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_EDIT_RESIZE, this, SLOT(_editResize()), XShortcuts::GROUPID_EDIT);
        }
    }

    return listResults;
}

XHexView::SHOWRECORD XHexView::_getShowRecordByViewPos(qint64 nOffset)
{
    SHOWRECORD result = {};

    qint32 nNumberOfRecords = g_listShowRecords.count();

    for (qint32 i = 0; i < nNumberOfRecords; i++) {
        if ((g_listShowRecords.at(i).nViewPos != -1) && (g_listShowRecords.at(i).nViewPos <= nOffset) &&
            (nOffset < (g_listShowRecords.at(i).nViewPos + g_listShowRecords.at(i).nSize))) {
            result = g_listShowRecords.at(i);
            break;
        }
    }

    return result;
}

XAbstractTableView::OS XHexView::cursorPositionToOS(const XAbstractTableView::CURSOR_POSITION &cursorPosition)
{
    OS osResult = {};

    osResult.nViewPos = -1;

    if ((cursorPosition.bIsValid) && (cursorPosition.ptype == PT_CELL)) {
        qint64 nBlockViewPos = getViewPosStart() + (cursorPosition.nRow * g_nBytesProLine);

        if (cursorPosition.nColumn == COLUMN_LOCATION) {
            osResult.nViewPos = nBlockViewPos;
            //            osResult.nSize=g_nPieceSize;
            osResult.nSize = 1;
        } else if (cursorPosition.nColumn == COLUMN_ELEMENTS) {
            osResult.nViewPos = nBlockViewPos + ((cursorPosition.nAreaLeft - getSideDelta() - getCharWidth()) / (getCharWidth() * g_nPrintsProElement + getSideDelta())) *
                                                    g_nElementByteSize;
            //            osResult.nSize=g_nPieceSize;
            osResult.nSize = 1;
        } else if (cursorPosition.nColumn == COLUMN_SYMBOLS) {
            osResult.nViewPos = nBlockViewPos + ((cursorPosition.nAreaLeft - getSideDelta() - getCharWidth()) / getCharWidth()) * g_nSymbolByteSize;
            //            osResult.nSize=g_nPieceSize;
            osResult.nSize = 1;
        }

        //        osResult.nOffset=S_ALIGN_DOWN(osResult.nOffset,g_nPieceSize);

        if (isViewPosValid(osResult.nViewPos)) {
            SHOWRECORD showRecord = _getShowRecordByViewPos(osResult.nViewPos);

            if (showRecord.nSize) {
                osResult.nViewPos = showRecord.nViewPos;
                osResult.nSize = showRecord.nSize;
            }
        } else {
            osResult.nViewPos = getViewSize();  // TODO Check
            osResult.nSize = 0;
        }

        //        qDebug("nBlockOffset %x",nBlockOffset);
        //        qDebug("cursorPosition.nCellLeft %x",cursorPosition.nCellLeft);
        //        qDebug("getCharWidth() %x",getCharWidth());
        //        qDebug("nOffset %x",osResult.nOffset);
    } else if ((cursorPosition.bIsValid) && (cursorPosition.ptype == PT_MAP)) {
        osResult.nViewPos = XBinary::align_down((getViewSize() * cursorPosition.nPercentage) / getMapCount(), g_nBytesProLine);
        osResult.nSize = 0;
    }
    return osResult;
}

void XHexView::updateData()
{
    QIODevice *_pDevice = getDevice();

    if (_pDevice) {
        // Update cursor position
        qint64 nDataBlockStartViewPos = getViewPosStart();  // TODO Check

        //        qint64 nCursorOffset = nBlockStartLine + getCursorDelta();

        //        if (nCursorOffset >= getViewSize()) {
        //            nCursorOffset = getViewSize() - 1;
        //        }

        //        setCursorViewPos(nCursorOffset);

        XBinary::MODE mode = XBinary::getWidthModeFromByteSize(g_nAddressWidth);

        g_listLocationRecords.clear();
        g_listShowRecords.clear();

        qint32 nDataBlockSize = g_nBytesProLine * getLinesProPage();

        nDataBlockSize = qMin(nDataBlockSize, (qint32)(getViewSize() - nDataBlockStartViewPos));

        g_listHighlightsRegion.clear();
        if (getXInfoDB()) {
            QVector<XInfoDB::BOOKMARKRECORD> listBookMarks = getXInfoDB()->getBookmarkRecords(nDataBlockStartViewPos, XBinary::LT_OFFSET, nDataBlockSize);
            g_listHighlightsRegion.append(_convertBookmarksToHighlightRegion(&listBookMarks));
        }

        qint64 nDeviceOffset = viewPosToDeviceOffset(nDataBlockStartViewPos);

        g_baDataBuffer = read_array(nDeviceOffset, nDataBlockSize);
        // QList<QChar> listElements = getStringBuffer(&g_baDataBuffer);

        // qint32 nNumberOfElements = listElements.count();

        g_nDataBlockSize = g_baDataBuffer.size();

        if (g_nDataBlockSize) {
            QString sDataHexBuffer;
            QString sANSI;

            if ((g_mode == ELEMENT_MODE_HEX) || (g_mode == ELEMENT_MODE_BYTE)) {
                sDataHexBuffer = QByteArray(g_baDataBuffer.toHex());
            }

            if (g_sCodePage == "") {
                sANSI = XBinary::dataToString(g_baDataBuffer, XBinary::DSMODE_NOPRINT_TO_DOT);
            }

            // Locations
            for (qint32 i = 0; i < g_nDataBlockSize; i += g_nBytesProLine) {
                XADDR nCurrentLocation = 0;

                LOCATIONRECORD record = {};
                record.nViewPos = i + nDataBlockStartViewPos;

                if (getlocationMode() == LOCMODE_THIS) {
                    nCurrentLocation = record.nViewPos;

                    qint64 nDelta = (qint64)nCurrentLocation - (qint64)g_nThisBase;

                    record.sLocation = XBinary::thisToString(nDelta, getLocationBase());
                } else {
                    if (getlocationMode() == LOCMODE_ADDRESS) {
                        nCurrentLocation = viewPosToAddress(record.nViewPos);
                    } else if (getlocationMode() == LOCMODE_OFFSET) {
                        nCurrentLocation = viewPosToDeviceOffset(record.nViewPos);
                    }

                    if (getLocationBase() == 16) {
                        if (g_bIsLocationColon) {
                            record.sLocation = XBinary::valueToHexColon(mode, nCurrentLocation);
                        } else {
                            record.sLocation = XBinary::valueToHex(mode, nCurrentLocation);
                        }
                    } else {
                        record.sLocation = QString("%1").arg(nCurrentLocation, g_nAddressWidth, getLocationBase(), QChar('0'));
                    }
                }

                g_listLocationRecords.append(record);
            }

            // Elements
            qint32 nMaxBytes = 1;

            if (g_mode == ELEMENT_MODE_HEX) {
                nMaxBytes = 8;  // TODO compare with XBinary::getHexSize
            } else {
                nMaxBytes = g_nElementByteSize;
            }

            char *pData = g_baDataBuffer.data();
            qint32 nCurrentRowViewPos = 0;
            qint32 nRow = 0;
            bool bFirst = true;

            for (qint32 i = 0; i < g_nDataBlockSize;) {
                SHOWRECORD record = {};

                record.nSize = g_nElementByteSize;
                record.nViewPos = nDataBlockStartViewPos + i;
                record.nRowViewPos = nCurrentRowViewPos;
                record.nRow = nRow;

                if (bFirst) {
                    record.bFirstRowSymbol = true;
                    bFirst = false;
                }

                if (g_sCodePage == "") {
                    record.sSymbol = sANSI.mid(i, g_nElementByteSize);
                } else {
#if (QT_VERSION_MAJOR < 6) || defined(QT_CORE5COMPAT_LIB)
                    if (g_pCodec) {
                        qint32 nJmax = qMin(g_nDataBlockSize - i, nMaxBytes);

                        for (int j = record.nSize; j <= nJmax; j++) {
                            QTextCodec::ConverterState converterState = {};
                            record.nSize = j;
                            record.sSymbol = g_pCodec->toUnicode(pData + i, record.nSize, &converterState);
                            record.bIsSymbolError = (converterState.remainingChars > 0);

                            if (converterState.remainingChars == 0) {
                                break;
                            }
                        }
                    }
#endif
                }

                if (g_mode == ELEMENT_MODE_HEX) {
                    record.sElement = sDataHexBuffer.mid(i * 2, 2 * record.nSize);
                } else if (g_mode == ELEMENT_MODE_BYTE) {
                    record.sElement = sDataHexBuffer.mid(i * 2, 2);  // g_nSymbolsProElement
                } else if (g_mode == ELEMENT_MODE_UINT8) {
                    record.sElement = QString::number(XBinary::_read_uint8(pData + i));
                } else if (g_mode == ELEMENT_MODE_INT8) {
                    record.sElement = QString::number(XBinary::_read_int8(pData + i));
                } else if (g_mode == ELEMENT_MODE_WORD) {
                    record.sElement = XBinary::valueToHex(XBinary::_read_uint16(pData + i));
                } else if (g_mode == ELEMENT_MODE_UINT16) {
                    record.sElement = QString::number(XBinary::_read_uint16(pData + i));
                } else if (g_mode == ELEMENT_MODE_INT16) {
                    record.sElement = QString::number(XBinary::_read_int16(pData + i));
                } else if (g_mode == ELEMENT_MODE_DWORD) {
                    record.sElement = XBinary::valueToHex(XBinary::_read_uint32(pData + i));
                } else if (g_mode == ELEMENT_MODE_UINT32) {
                    record.sElement = QString::number(XBinary::_read_uint32(pData + i));
                } else if (g_mode == ELEMENT_MODE_INT32) {
                    record.sElement = QString::number(XBinary::_read_int32(pData + i));
                } else if (g_mode == ELEMENT_MODE_QWORD) {
                    record.sElement = XBinary::valueToHex(XBinary::_read_uint64(pData + i));
                } else if (g_mode == ELEMENT_MODE_UINT64) {
                    record.sElement = QString::number(XBinary::_read_uint64(pData + i));
                } else if (g_mode == ELEMENT_MODE_INT64) {
                    record.sElement = QString::number(XBinary::_read_int64(pData + i));
                }

                // if (i < nNumberOfElements) {
                //     record.sSymbol = listElements.at(i);
                // }

                if (record.nSize == 1) {
                    record.bIsBold = (g_baDataBuffer.at(i) != 0);  // TODO optimize !!! TODO Different rules
                }

                QList<HIGHLIGHTREGION> listHighLightRegions = getHighlightRegion(&g_listHighlightsRegion, nDataBlockStartViewPos + i, XBinary::LT_OFFSET);

                if (listHighLightRegions.count()) {
                    record.bIsHighlighted = true;
                    record.colBackground = listHighLightRegions.at(0).colBackground;
                    record.colBackgroundSelected = listHighLightRegions.at(0).colBackgroundSelected;
                } else {
                    record.colBackgroundSelected = getColor(TCLOLOR_SELECTED);
                }

                //                record.bIsSelected = isViewPosSelected(nDataBlockStartOffset + i);

                i += record.nSize;
                nCurrentRowViewPos += record.nSize;

                if (nCurrentRowViewPos >= g_nBytesProLine) {
                    nCurrentRowViewPos -= g_nBytesProLine;
                    nRow++;
                    record.bLastRowSymbol = true;
                    bFirst = true;
                } else if (nCurrentRowViewPos >= getViewSize()) {
                    record.bLastRowSymbol = true;
                }

                g_listShowRecords.append(record);
            }
        } else {
            g_baDataBuffer.clear();
        }

        setCurrentBlock(nDataBlockStartViewPos, g_nDataBlockSize);

        g_pixmapCache.clear();
    }
}

void XHexView::paintMap(QPainter *pPainter, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight)
{
    pPainter->save();

    QString sKey = "Map_0_0";
    sKey += QString("_%1").arg(nWidth);
    sKey += QString("_%1").arg(nHeight);

    QPixmap _pixmap(0, 0);

    if (g_pixmapCache.find(sKey, &_pixmap)) {
        //        if (false) {
        pPainter->drawPixmap(nLeft, nTop, nWidth, nHeight, _pixmap);
    } else {
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
        qreal ratio = QPaintDevice::devicePixelRatioF();
#else
        qreal ratio = QPaintDevice::devicePixelRatio();
#endif
        // qint32 nPartCount = qMin(nHeight, (qint32)nHeight);
        // qint32 nPartSize = getDevice()->size() / nPartCount;

        // XBinary::_MEMORY_MAP *pMemoryMap = getMemoryMap();

        // TODO memoryMap tooltips

        QPixmap pixmap(nWidth * ratio, nHeight * ratio);
        pixmap.setDevicePixelRatio(ratio);
        pixmap.fill(Qt::transparent);

        QPainter painterPixmap(&pixmap);
        // painterPixmap.fillRect(0, 0, nWidth, nHeight, QBrush(Qt::darkYellow));

        g_pixmapCache.insert(sKey, pixmap);

        pPainter->drawPixmap(nLeft, nTop, nWidth, nHeight, pixmap);
    }

    pPainter->restore();
}

void XHexView::paintCell(QPainter *pPainter, qint32 nRow, qint32 nColumn, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight)
{
    // #ifdef QT_DEBUG
    //     QElapsedTimer timer;
    //     timer.start();
    // #endif
    //     g_pPainterText->drawRect(nLeft,nTop,nWidth,nHeight);

    if (nColumn == COLUMN_LOCATION) {
        //        if (nRow < g_listLocationRecords.count()) {
        //            QRect rectSymbol;

        //            rectSymbol.setLeft(nLeft + getCharWidth());
        //            rectSymbol.setTop(nTop + getLineDelta());
        //            rectSymbol.setWidth(nWidth);
        //            rectSymbol.setHeight(nHeight - getLineDelta());

        //            //            pPainter->save();
        //            //            pPainter->setPen(viewport()->palette().color(QPalette::Dark));
        //            pPainter->drawText(rectSymbol, g_listLocationRecords.at(nRow).sLocation);  // TODO Text Optional
        //                                                                              //            pPainter->restore();
        //        }
    } else if ((nColumn == COLUMN_ELEMENTS) || (nColumn == COLUMN_SYMBOLS)) {
        qint32 nNumberOfShowRecords = g_listShowRecords.count();

        QFont fontBold = pPainter->font();
        fontBold.setBold(true);

        for (qint32 i = 0; i < nNumberOfShowRecords; i++) {
            if (g_listShowRecords.at(i).nRow == nRow) {
                SHOWRECORD record = g_listShowRecords.at(i);

                bool bIsSelected = isViewPosSelected(record.nViewPos);
                bool bIsSelectedPrev = false;
                bool bIsSelectedNext = false;

                if (i - 1 >= 0) {
                    bIsSelectedPrev = isViewPosSelected(g_listShowRecords.at(i - 1).nViewPos);
                }

                if (i + 1 < nNumberOfShowRecords) {
                    bIsSelectedNext = isViewPosSelected(g_listShowRecords.at(i + 1).nViewPos);
                }

                QRectF rectSymbol;

                if (bIsSelected) {
                    if (nColumn == COLUMN_ELEMENTS) {
                        qint32 _nRowOffset = record.nRowViewPos / g_nElementByteSize;

                        rectSymbol.setLeft(nLeft + getCharWidth() + (_nRowOffset * (g_nPrintsProElement * getCharWidth() + getSideDelta())));
                        rectSymbol.setTop(nTop + getLineDelta());
                        rectSymbol.setHeight(nHeight - getLineDelta());

                        int nWidth = (record.nSize / g_nElementByteSize) * (g_nPrintsProElement * getCharWidth() + getSideDelta());

                        if ((record.bLastRowSymbol) || (bIsSelected && (!bIsSelectedNext))) {
                            nWidth -= getSideDelta();
                        }

                        rectSymbol.setWidth(nWidth);
                    } else if (nColumn == COLUMN_SYMBOLS) {
                        rectSymbol.setLeft(nLeft + (record.nRowViewPos + 1) * getCharWidth());
                        rectSymbol.setTop(nTop + getLineDelta());
                        rectSymbol.setWidth(getCharWidth() * (record.nSize / g_nSymbolByteSize));
                        rectSymbol.setHeight(nHeight - getLineDelta());
                    }

                    if (rectSymbol.left() < (nLeft + nWidth)) {  // Paint Only visible
                        if (record.bIsBold) {
                            pPainter->save();
                            pPainter->setFont(fontBold);
                        }

                        // QString sSymbol;

                        // if (nColumn == COLUMN_ELEMENTS) {
                        //     sSymbol = record.sElement;
                        // } else if (nColumn == COLUMN_SYMBOLS) {
                        //     sSymbol = record.sSymbol;
                        // }

                        if (record.bIsSymbolError) {
                            pPainter->fillRect(rectSymbol, QBrush(Qt::red));
                        } else if (bIsSelected) {
                            pPainter->fillRect(rectSymbol, record.colBackgroundSelected);
                        }

                        if (bIsSelected) {
                            // Draw lines
                            bool bTop = false;
                            bool bLeft = false;
                            bool bBottom = false;
                            bool bRight = false;

                            if ((record.bFirstRowSymbol) || (!bIsSelectedPrev)) {
                                bLeft = true;
                            }

                            if ((record.bLastRowSymbol) || (!bIsSelectedNext)) {
                                bRight = true;
                            }

                            if (!isViewPosSelected(record.nViewPos - g_nBytesProLine)) {
                                bTop = true;
                            }

                            if (!isViewPosSelected(record.nViewPos + g_nBytesProLine)) {
                                bBottom = true;
                            }

                            if (bTop) {
                                pPainter->drawLine(rectSymbol.left(), rectSymbol.top(), rectSymbol.right(), rectSymbol.top());
                            }

                            if (bLeft) {
                                pPainter->drawLine(rectSymbol.left(), rectSymbol.top(), rectSymbol.left(), rectSymbol.bottom());
                            }

                            if (bBottom) {
                                pPainter->drawLine(rectSymbol.left(), rectSymbol.bottom(), rectSymbol.right(), rectSymbol.bottom());
                            }

                            if (bRight) {
                                pPainter->drawLine(rectSymbol.right(), rectSymbol.top(), rectSymbol.right(), rectSymbol.bottom());
                            }

                            if (record.bIsBold) {
                                pPainter->restore();
                            }
                        } else {
                            break;  // Do not paint invisible
                        }
                    }
                }
            }
        }
    }
    // #ifdef QT_DEBUG
    //     qDebug("XHexView::paintCell %lld",timer.elapsed());
    // #endif
}

void XHexView::paintColumn(QPainter *pPainter, qint32 nColumn, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight)
{
#ifdef QT_DEBUG
//    qDebug("XHexView::paintColumn");
//    QElapsedTimer timer;
//    timer.start();
#endif

    QString sKey;

    if (nColumn == COLUMN_LOCATION) {
        sKey = QString("location");
    } else if (nColumn == COLUMN_ELEMENTS) {
        sKey = QString("elements");
    } else if (nColumn == COLUMN_SYMBOLS) {
        sKey = QString("symbols");
    }

    if (sKey != "") {
        sKey += QString("_%1").arg(getViewPosStart());
        sKey += QString("_%1").arg(getViewSize());
        sKey += QString("_%1").arg(nWidth);
        sKey += QString("_%1").arg(nHeight);

        QPixmap _pixmap(0, 0);

        if (g_pixmapCache.find(sKey, &_pixmap)) {
            // qDebug("g_pixmapCache");
            pPainter->drawPixmap(nLeft, nTop, nWidth, nHeight, _pixmap);
        } else {
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
            qreal ratio = QPaintDevice::devicePixelRatioF();
#else
            qreal ratio = QPaintDevice::devicePixelRatio();
#endif
            QPixmap pixmap(nWidth * ratio, nHeight * ratio);
            pixmap.setDevicePixelRatio(ratio);
            pixmap.fill(Qt::transparent);

            QPainter painterPixmap(&pixmap);
            painterPixmap.setFont(pPainter->font());
            painterPixmap.setBackgroundMode(Qt::TransparentMode);

            if (nColumn == COLUMN_LOCATION) {
                qint32 nNumberOfRows = g_listLocationRecords.count();

                for (qint32 i = 0; i < nNumberOfRows; i++) {
                    QRectF rectSymbol;
                    rectSymbol.setLeft(getCharWidth());
                    rectSymbol.setTop(getLineHeight() * i + getLineDelta());
                    rectSymbol.setWidth(nWidth);
                    rectSymbol.setHeight(getLineHeight() - getLineDelta());

                    painterPixmap.drawText(rectSymbol, g_listLocationRecords.at(i).sLocation);  // TODO Text Optional pPainter->restore();
                }
            } else if ((nColumn == COLUMN_ELEMENTS) || (nColumn == COLUMN_SYMBOLS)) {
                QFont fontBold = painterPixmap.font();
                fontBold.setBold(true);

                qint32 nNumberOfShowRecords = g_listShowRecords.count();

                for (qint32 i = 0; i < nNumberOfShowRecords; i++) {
                    SHOWRECORD record = g_listShowRecords.at(i);

                    bool bIsHighlighted = g_listShowRecords.at(i).bIsHighlighted;
                    bool bIsHighlightedNext = false;

                    if (i + 1 < nNumberOfShowRecords) {
                        bIsHighlightedNext = g_listShowRecords.at(i + 1).bIsHighlighted;
                    }

                    QRectF rectSymbol;

                    if (nColumn == COLUMN_ELEMENTS) {
                        qint32 _nRowOffset = record.nRowViewPos / g_nElementByteSize;

                        rectSymbol.setLeft(getCharWidth() + (_nRowOffset * (g_nPrintsProElement * getCharWidth() + getSideDelta())));
                        rectSymbol.setTop(getLineHeight() * record.nRow + getLineDelta());
                        rectSymbol.setHeight(getLineHeight() - getLineDelta());

                        int nWidth = (record.nSize / g_nElementByteSize) * (g_nPrintsProElement * getCharWidth() + getSideDelta());

                        if ((record.bLastRowSymbol) || (bIsHighlighted && (!bIsHighlightedNext))) {
                            nWidth -= getSideDelta();
                        }

                        rectSymbol.setWidth(nWidth);
                    } else if (nColumn == COLUMN_SYMBOLS) {
                        rectSymbol.setLeft((record.nRowViewPos + 1) * getCharWidth());
                        rectSymbol.setTop(getLineHeight() * record.nRow + getLineDelta());
                        rectSymbol.setWidth(getCharWidth() * (record.nSize / g_nSymbolByteSize));
                        rectSymbol.setHeight(getLineHeight() - getLineDelta());
                    }

                    if (record.bIsBold) {
                        painterPixmap.save();
                        painterPixmap.setFont(fontBold);
                    }

                    QString sSymbol;

                    if (nColumn == COLUMN_ELEMENTS) {
                        sSymbol = record.sElement;
                    } else if (nColumn == COLUMN_SYMBOLS) {
                        sSymbol = record.sSymbol;
                    }

                    if (record.bIsSymbolError) {
                        painterPixmap.fillRect(rectSymbol, QBrush(Qt::red));  // TODO
                    } else if (bIsHighlighted) {
                        painterPixmap.fillRect(rectSymbol, record.colBackground);
                    }

                    if (nColumn == COLUMN_ELEMENTS) {
                        painterPixmap.drawText(rectSymbol, sSymbol, Qt::AlignVCenter | Qt::AlignHCenter);
                    } else if (nColumn == COLUMN_SYMBOLS) {
                        if (sSymbol != "") {
                            painterPixmap.drawText(rectSymbol, sSymbol);
                        }
                    }

                    if (record.bIsBold) {
                        painterPixmap.restore();
                    }
                }
            }

            g_pixmapCache.insert(sKey, pixmap);

            pPainter->drawPixmap(nLeft, nTop, nWidth, nHeight, pixmap);
        }
    }

#ifdef QT_DEBUG
//    qDebug("Elapsed XHexView::paintColumn %lld", timer.elapsed());
#endif
}

void XHexView::paintTitle(QPainter *pPainter, qint32 nColumn, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight, const QString &sTitle)
{
    if (nColumn == COLUMN_ELEMENTS) {
        for (qint32 i = 0; i < g_nBytesProLine / g_nElementByteSize; i++) {
            QString sSymbol = QString("%1").arg(i * g_nElementByteSize, 2, getLocationBase(), QChar('0'));
            ;

            QRectF rectSymbol;

            rectSymbol.setLeft(nLeft + getCharWidth() + (i * g_nPrintsProElement) * getCharWidth() + i * getSideDelta());
            rectSymbol.setTop(nTop);
            rectSymbol.setWidth(g_nPrintsProElement * getCharWidth() + getSideDelta());
            rectSymbol.setHeight(nHeight);

            if ((rectSymbol.left()) < (nLeft + nWidth)) {
                pPainter->drawText(rectSymbol, Qt::AlignVCenter | Qt::AlignHCenter, sSymbol);
            }
        }
    } else {
        XAbstractTableView::paintTitle(pPainter, nColumn, nLeft, nTop, nWidth, nHeight, sTitle);
    }
}

void XHexView::wheelEvent(QWheelEvent *pEvent)
{
    if ((g_nViewStartDelta) && (pEvent->angleDelta().y() > 0)) {
        if (getCurrentViewPosFromScroll() == g_nViewStartDelta) {
            setCurrentViewPosToScroll(0);
            adjust(true);
        }
    }

    XAbstractTableView::wheelEvent(pEvent);
}

void XHexView::keyPressEvent(QKeyEvent *pEvent)
{
    // Move commands
    if (pEvent->matches(QKeySequence::MoveToNextChar) || pEvent->matches(QKeySequence::MoveToPreviousChar) || pEvent->matches(QKeySequence::MoveToNextLine) ||
        pEvent->matches(QKeySequence::MoveToPreviousLine) || pEvent->matches(QKeySequence::MoveToStartOfLine) || pEvent->matches(QKeySequence::MoveToEndOfLine) ||
        pEvent->matches(QKeySequence::MoveToNextPage) || pEvent->matches(QKeySequence::MoveToPreviousPage) || pEvent->matches(QKeySequence::MoveToStartOfDocument) ||
        pEvent->matches(QKeySequence::MoveToEndOfDocument)) {
        STATE state = getState();
        qint64 nViewStart = getViewPosStart();

        state.nSelectionViewSize = 1;

        if (pEvent->matches(QKeySequence::MoveToNextChar)) {
            state.nSelectionViewPos += g_nElementByteSize;  // TODO fix UTF8
        } else if (pEvent->matches(QKeySequence::MoveToPreviousChar)) {
            state.nSelectionViewPos -= g_nElementByteSize;
        } else if (pEvent->matches(QKeySequence::MoveToNextLine)) {
            state.nSelectionViewPos += g_nBytesProLine;
        } else if (pEvent->matches(QKeySequence::MoveToPreviousLine)) {
            state.nSelectionViewPos -= g_nBytesProLine;
        } else if (pEvent->matches(QKeySequence::MoveToStartOfLine)) {
            // TODO
        } else if (pEvent->matches(QKeySequence::MoveToEndOfLine)) {
            // TODO
        }

        if ((state.nSelectionViewPos < 0) || (pEvent->matches(QKeySequence::MoveToStartOfDocument))) {
            state.nSelectionViewPos = 0;
            g_nViewStartDelta = 0;
        }

        if ((state.nSelectionViewPos >= getViewSize()) || (pEvent->matches(QKeySequence::MoveToEndOfDocument))) {
            state.nSelectionViewPos = getViewSize() - 1;
            g_nViewStartDelta = 0;
        }

        if (isViewPosValid(state.nSelectionViewPos)) {
            SHOWRECORD showRecord = _getShowRecordByViewPos(state.nSelectionViewPos);

            if (showRecord.nSize) {
                state.nSelectionViewPos = showRecord.nViewPos;
                state.nSelectionViewSize = showRecord.nSize;
            }
        }

        setState(state);

        if (pEvent->matches(QKeySequence::MoveToNextChar) || pEvent->matches(QKeySequence::MoveToPreviousChar) || pEvent->matches(QKeySequence::MoveToNextLine) ||
            pEvent->matches(QKeySequence::MoveToPreviousLine)) {
            qint64 nRelOffset = state.nSelectionViewPos - nViewStart;

            if (nRelOffset >= g_nBytesProLine * getLinesProPage()) {
                _goToViewPos(nViewStart + g_nBytesProLine, true);
            } else if (nRelOffset < 0) {
                if (!_goToViewPos(nViewStart - g_nBytesProLine, true)) {
                    _goToViewPos(0);
                }
            }
        } else if (pEvent->matches(QKeySequence::MoveToNextPage) || pEvent->matches(QKeySequence::MoveToPreviousPage)) {
            if (pEvent->matches(QKeySequence::MoveToNextPage)) {
                _goToViewPos(nViewStart + g_nBytesProLine * getLinesProPage());
            } else if (pEvent->matches(QKeySequence::MoveToPreviousPage)) {
                _goToViewPos(nViewStart - g_nBytesProLine * getLinesProPage());
            }
        } else if (pEvent->matches(QKeySequence::MoveToStartOfDocument) || pEvent->matches(QKeySequence::MoveToEndOfDocument))  // TODO
        {
            _goToViewPos(state.nSelectionViewPos);
        }

        adjust();
        viewport()->update();
    }
    //    else if(pEvent->matches(QKeySequence::SelectAll))
    //    {
    //        _selectAllSlot();
    //    }
    else {
        XAbstractTableView::keyPressEvent(pEvent);
    }
}

qint64 XHexView::getCurrentViewPosFromScroll()
{
    qint64 nResult = 0;

    qint32 nValue = verticalScrollBar()->value();

    qint64 nMaxValue = getMaxScrollValue() * g_nBytesProLine;

    if (getViewSize() > nMaxValue) {
        if (nValue == getMaxScrollValue()) {
            nResult = getViewSize() - g_nBytesProLine;
        } else {
            nResult = ((double)nValue / (double)getMaxScrollValue()) * getViewSize() + g_nViewStartDelta;
        }
    } else {
        nResult = (qint64)nValue * g_nBytesProLine + g_nViewStartDelta;
    }

    return nResult;
}

void XHexView::setCurrentViewPosToScroll(qint64 nOffset)
{
    setViewPosStart(nOffset);
    g_nViewStartDelta = (nOffset) % g_nBytesProLine;

    qint32 nValue = 0;

    if (getViewSize() > (getMaxScrollValue() * g_nBytesProLine)) {
        if (nOffset == getViewSize() - g_nBytesProLine) {
            nValue = getMaxScrollValue();
        } else {
            nValue = ((double)(nOffset - g_nViewStartDelta) / ((double)getViewSize())) * (double)getMaxScrollValue();
        }
    } else {
        nValue = (nOffset) / g_nBytesProLine;
    }

    {
        const bool bBlocked1 = verticalScrollBar()->blockSignals(true);

        verticalScrollBar()->setValue(nValue);
        _verticalScroll();

        verticalScrollBar()->blockSignals(bBlocked1);
    }
}

void XHexView::adjustColumns()
{
    const QFontMetricsF fm(getTextFont());

    // if (XBinary::getWidthModeFromSize(getStartLocation() + getViewSize()) == XBinary::MODE_64) {
    if (XBinary::getWidthModeFromSize(getViewSize()) == XBinary::MODE_64) {
        g_nAddressWidth = 16;
        setColumnWidth(COLUMN_LOCATION, 2 * getCharWidth() + fm.boundingRect("00000000:00000000").width());
    } else {
        g_nAddressWidth = 8;
        setColumnWidth(COLUMN_LOCATION, 2 * getCharWidth() + fm.boundingRect("0000:0000").width());
    }

    qint32 nNumberOfElements = g_nBytesProLine / g_nElementByteSize;
    qint32 nNumberOfSymbols = g_nBytesProLine / g_nSymbolByteSize;

    setColumnWidth(COLUMN_ELEMENTS, nNumberOfElements * g_nPrintsProElement * getCharWidth() + 2 * getCharWidth() + getSideDelta() * nNumberOfElements);
    setColumnWidth(COLUMN_SYMBOLS, (nNumberOfSymbols + 2) * getCharWidth());
}

void XHexView::adjustHeader()
{
    if (getlocationMode() == LOCMODE_ADDRESS) {
        setColumnTitle(COLUMN_LOCATION, tr("Address"));
    } else if ((getlocationMode() == LOCMODE_OFFSET) || (getlocationMode() == LOCMODE_THIS)) {
        setColumnTitle(COLUMN_LOCATION, tr("Offset"));
    }
}

void XHexView::_headerClicked(qint32 nColumn)
{
    if (nColumn == COLUMN_LOCATION) {
        // // TODO Context Menu with
        // if (getAddressMode() == LOCMODE_ADDRESS) {
        //     setColumnTitle(COLUMN_ADDRESS, tr("Offset"));
        //     setAddressMode(LOCMODE_OFFSET);
        // } else if ((getAddressMode() == LOCMODE_OFFSET) || (getAddressMode() == LOCMODE_THIS)) {
        //     setColumnTitle(COLUMN_ADDRESS, tr("Address"));
        //     setAddressMode(LOCMODE_ADDRESS);
        // }
        QMenu contextMenu(this);

        QList<XShortcuts::MENUITEM> listMenuItems;

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = QString("10");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeLocationBase());
            menuItem.nSubgroups = XShortcuts::GROUPID_BASE;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = (getLocationBase() == 10);
            menuItem.sPropertyName = "base";
            menuItem.varProperty = 10;

            listMenuItems.append(menuItem);
        }

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = QString("16");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeLocationBase());
            menuItem.nSubgroups = XShortcuts::GROUPID_BASE;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = (getLocationBase() == 16);
            menuItem.sPropertyName = "base";
            menuItem.varProperty = 16;

            listMenuItems.append(menuItem);
        }

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = tr("Address");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeLocationMode());
            menuItem.nSubgroups = XShortcuts::GROUPID_LOCATION;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = (getlocationMode() == LOCMODE_ADDRESS);
            menuItem.sPropertyName = "mode";
            menuItem.varProperty = LOCMODE_ADDRESS;

            listMenuItems.append(menuItem);
        }

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = tr("Offset");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeLocationMode());
            menuItem.nSubgroups = XShortcuts::GROUPID_LOCATION;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = (getlocationMode() == LOCMODE_OFFSET);
            menuItem.sPropertyName = "mode";
            menuItem.varProperty = LOCMODE_OFFSET;

            listMenuItems.append(menuItem);
        }

        QList<QObject *> listObjects = getShortcuts()->adjustContextMenu(&contextMenu, &listMenuItems);

        contextMenu.exec(QCursor::pos());

        XOptions::deleteQObjectList(&listObjects);

        // adjust(true);
    } else if (nColumn == COLUMN_ELEMENTS) {
        QMenu contextMenu(this);  // TODO

        QList<XShortcuts::MENUITEM> listMenuItems;

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = tr("Hex");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeElementMode());
            menuItem.nSubgroups = XShortcuts::GROUPID_MODE;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = (g_mode == ELEMENT_MODE_HEX);
            menuItem.sPropertyName = "mode";
            menuItem.varProperty = ELEMENT_MODE_HEX;

            listMenuItems.append(menuItem);
        }

        getShortcuts()->_addMenuSeparator(&listMenuItems, XShortcuts::GROUPID_MODE);

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = QString("byte");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeElementMode());
            menuItem.nSubgroups = XShortcuts::GROUPID_MODE;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = (g_mode == ELEMENT_MODE_BYTE);
            menuItem.sPropertyName = "mode";
            menuItem.varProperty = ELEMENT_MODE_BYTE;

            listMenuItems.append(menuItem);
        }

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = QString("word");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeElementMode());
            menuItem.nSubgroups = XShortcuts::GROUPID_MODE;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = (g_mode == ELEMENT_MODE_WORD);
            menuItem.sPropertyName = "mode";
            menuItem.varProperty = ELEMENT_MODE_WORD;

            listMenuItems.append(menuItem);
        }

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = QString("dword");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeElementMode());
            menuItem.nSubgroups = XShortcuts::GROUPID_MODE;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = (g_mode == ELEMENT_MODE_DWORD);
            menuItem.sPropertyName = "mode";
            menuItem.varProperty = ELEMENT_MODE_DWORD;

            listMenuItems.append(menuItem);
        }

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = QString("qword");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeElementMode());
            menuItem.nSubgroups = XShortcuts::GROUPID_MODE;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = (g_mode == ELEMENT_MODE_QWORD);
            menuItem.sPropertyName = "mode";
            menuItem.varProperty = ELEMENT_MODE_QWORD;

            listMenuItems.append(menuItem);
        }

        getShortcuts()->_addMenuSeparator(&listMenuItems, XShortcuts::GROUPID_MODE);

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = QString("uint8");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeElementMode());
            menuItem.nSubgroups = XShortcuts::GROUPID_MODE;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = (g_mode == ELEMENT_MODE_UINT8);
            menuItem.sPropertyName = "mode";
            menuItem.varProperty = ELEMENT_MODE_UINT8;

            listMenuItems.append(menuItem);
        }

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = QString("int8");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeElementMode());
            menuItem.nSubgroups = XShortcuts::GROUPID_MODE;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = (g_mode == ELEMENT_MODE_INT8);
            menuItem.sPropertyName = "mode";
            menuItem.varProperty = ELEMENT_MODE_INT8;

            listMenuItems.append(menuItem);
        }

        getShortcuts()->_addMenuSeparator(&listMenuItems, XShortcuts::GROUPID_MODE);

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = QString("uint16");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeElementMode());
            menuItem.nSubgroups = XShortcuts::GROUPID_MODE;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = (g_mode == ELEMENT_MODE_UINT16);
            menuItem.sPropertyName = "mode";
            menuItem.varProperty = ELEMENT_MODE_UINT16;

            listMenuItems.append(menuItem);
        }

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = QString("int16");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeElementMode());
            menuItem.nSubgroups = XShortcuts::GROUPID_MODE;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = (g_mode == ELEMENT_MODE_INT16);
            menuItem.sPropertyName = "mode";
            menuItem.varProperty = ELEMENT_MODE_INT16;

            listMenuItems.append(menuItem);
        }

        getShortcuts()->_addMenuSeparator(&listMenuItems, XShortcuts::GROUPID_MODE);

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = QString("uint32");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeElementMode());
            menuItem.nSubgroups = XShortcuts::GROUPID_MODE;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = (g_mode == ELEMENT_MODE_UINT32);
            menuItem.sPropertyName = "mode";
            menuItem.varProperty = ELEMENT_MODE_UINT32;

            listMenuItems.append(menuItem);
        }

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = QString("int32");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeElementMode());
            menuItem.nSubgroups = XShortcuts::GROUPID_MODE;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = (g_mode == ELEMENT_MODE_INT32);
            menuItem.sPropertyName = "mode";
            menuItem.varProperty = ELEMENT_MODE_INT32;

            listMenuItems.append(menuItem);
        }

        getShortcuts()->_addMenuSeparator(&listMenuItems, XShortcuts::GROUPID_MODE);

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = QString("uint64");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeElementMode());
            menuItem.nSubgroups = XShortcuts::GROUPID_MODE;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = (g_mode == ELEMENT_MODE_UINT64);
            menuItem.sPropertyName = "mode";
            menuItem.varProperty = ELEMENT_MODE_UINT64;

            listMenuItems.append(menuItem);
        }

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = QString("int64");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeElementMode());
            menuItem.nSubgroups = XShortcuts::GROUPID_MODE;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = (g_mode == ELEMENT_MODE_INT64);
            menuItem.sPropertyName = "mode";
            menuItem.varProperty = ELEMENT_MODE_INT64;

            listMenuItems.append(menuItem);
        }

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = QString("8");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeElementWidth());
            menuItem.nSubgroups = XShortcuts::GROUPID_WIDTH;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = (g_nBytesProLine == 8);
            menuItem.sPropertyName = "width";
            menuItem.varProperty = 8;

            listMenuItems.append(menuItem);
        }

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = QString("16");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeElementWidth());
            menuItem.nSubgroups = XShortcuts::GROUPID_WIDTH;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = (g_nBytesProLine == 16);
            menuItem.sPropertyName = "width";
            menuItem.varProperty = 16;

            listMenuItems.append(menuItem);
        }

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = QString("24");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeElementWidth());
            menuItem.nSubgroups = XShortcuts::GROUPID_WIDTH;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = (g_nBytesProLine == 24);
            menuItem.sPropertyName = "width";
            menuItem.varProperty = 24;

            listMenuItems.append(menuItem);
        }

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = QString("32");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeElementWidth());
            menuItem.nSubgroups = XShortcuts::GROUPID_WIDTH;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = (g_nBytesProLine == 32);
            menuItem.sPropertyName = "width";
            menuItem.varProperty = 32;

            listMenuItems.append(menuItem);
        }

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = QString("48");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeElementWidth());
            menuItem.nSubgroups = XShortcuts::GROUPID_WIDTH;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = (g_nBytesProLine == 48);
            menuItem.sPropertyName = "width";
            menuItem.varProperty = 48;

            listMenuItems.append(menuItem);
        }

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = QString("64");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeElementWidth());
            menuItem.nSubgroups = XShortcuts::GROUPID_WIDTH;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = (g_nBytesProLine == 64);
            menuItem.sPropertyName = "width";
            menuItem.varProperty = 64;

            listMenuItems.append(menuItem);
        }

        QList<QObject *> listObjects = getShortcuts()->adjustContextMenu(&contextMenu, &listMenuItems);

        contextMenu.exec(QCursor::pos());

        XOptions::deleteQObjectList(&listObjects);
    } else if (nColumn == COLUMN_SYMBOLS) {
#if (QT_VERSION_MAJOR < 6) || defined(QT_CORE5COMPAT_LIB)
        g_pCodePageMenu->exec(QCursor::pos());
#endif
    }

    XAbstractTableView::_headerClicked(nColumn);
}

void XHexView::_cellDoubleClicked(qint32 nRow, qint32 nColumn)
{
    if (nColumn == COLUMN_LOCATION) {
        setColumnTitle(COLUMN_LOCATION, "");
        setLocationMode(LOCMODE_THIS);

        if (nRow < g_listLocationRecords.count()) {
            g_nThisBase = g_listLocationRecords.at(nRow).nViewPos;
        }

        adjust(true);
    }
}

void XHexView::adjustScrollCount()
{
    qint64 nTotalLineCount = getViewSize() / g_nBytesProLine;

    if (getViewSize() % g_nBytesProLine == 0) {
        nTotalLineCount--;
    }

    //    if((getDataSize()>0)&&(getDataSize()<g_nBytesProLine))
    //    {
    //        nTotalLineCount=1;
    //    }

    setTotalScrollCount(nTotalLineCount);
}

void XHexView::adjustMap()
{
    if (isMapEnable()) {
        if (getDevice()) {
            qint64 nNumberOfLines = getDevice()->size() / g_nBytesProLine;
            if (nNumberOfLines > 100) {
                nNumberOfLines = 100;
            } else if (nNumberOfLines == 0) {
                nNumberOfLines = 1;
            }

            setMapCount((qint32)nNumberOfLines);
        }
    }
}

// XHexView::SMODE XHexView::getSmode()
//{
//     return g_smode;
// }

// void XHexView::setSmode(SMODE smode)
//{
//     g_smode=smode;

////    if(smode==SMODE_UNICODE)
////    {
////        g_nPieceSize=2;
////    }
////    else
////    {
////        g_nPieceSize=1;
////    }
//}

void XHexView::_disasmSlot()
{
    if (g_hexOptions.bMenu_Disasm) {
        emit followLocation(getDeviceState().nSelectionDeviceOffset, XBinary::LT_OFFSET, 0, XOptions::WIDGETTYPE_DISASM);
    }
}

void XHexView::_memoryMapSlot()
{
    if (g_hexOptions.bMenu_MemoryMap) {
        emit followLocation(getDeviceState().nSelectionDeviceOffset, XBinary::LT_OFFSET, 0, XOptions::WIDGETTYPE_MEMORYMAP);
    }
}

void XHexView::_mainHexSlot()
{
    if (g_hexOptions.bMenu_MainHex) {
        DEVICESTATE deviceState = getDeviceState();
        emit followLocation(getDeviceState().nSelectionDeviceOffset, XBinary::LT_OFFSET, deviceState.nSelectionSize, XOptions::WIDGETTYPE_HEX);
    }
}

void XHexView::_setCodePage(const QString &sCodePage)
{
#if (QT_VERSION_MAJOR < 6) || defined(QT_CORE5COMPAT_LIB)
    g_sCodePage = sCodePage;

    QString sTitle = tr("Symbols");

    if (g_sCodePage != "") {
        sTitle = g_sCodePage;
        g_pCodec = QTextCodec::codecForName(g_sCodePage.toLatin1().data());
    }

    setColumnTitle(COLUMN_SYMBOLS, sTitle);

    adjust(true);
#else
    Q_UNUSED(sCodePage)
#endif
}

void XHexView::changeElementWidth()
{
    QAction *pAction = qobject_cast<QAction *>(sender());

    if (pAction) {
        g_nBytesProLine = pAction->property("width").toUInt();

        adjustMap();
        adjustScrollCount();
        adjustColumns();
        adjust(true);
    }
}

void XHexView::changeElementMode()
{
    QAction *pAction = qobject_cast<QAction *>(sender());

    if (pAction) {
        ELEMENT_MODE mode = (ELEMENT_MODE)pAction->property("mode").toUInt();

        _setMode(mode);

        adjustColumns();
        adjust(true);
    }
}

void XHexView::_setMode(ELEMENT_MODE mode)
{
    g_mode = mode;

    if (mode == ELEMENT_MODE_HEX) {
        g_nPrintsProElement = 2;
        g_nElementByteSize = 1;
    } else if (mode == ELEMENT_MODE_BYTE) {
        g_nPrintsProElement = 2;
        g_nElementByteSize = 1;
    } else if (mode == ELEMENT_MODE_UINT8) {
        g_nPrintsProElement = 3;
        g_nElementByteSize = 1;
    } else if (mode == ELEMENT_MODE_INT8) {
        g_nPrintsProElement = 4;
        g_nElementByteSize = 1;
    } else if (mode == ELEMENT_MODE_WORD) {
        g_nPrintsProElement = 4;
        g_nElementByteSize = 2;
    } else if (mode == ELEMENT_MODE_UINT16) {
        g_nPrintsProElement = 5;
        g_nElementByteSize = 2;
    } else if (mode == ELEMENT_MODE_INT16) {
        g_nPrintsProElement = 6;
        g_nElementByteSize = 2;
    } else if (mode == ELEMENT_MODE_DWORD) {
        g_nPrintsProElement = 8;
        g_nElementByteSize = 4;
    } else if (mode == ELEMENT_MODE_UINT32) {
        g_nPrintsProElement = 10;
        g_nElementByteSize = 4;
    } else if (mode == ELEMENT_MODE_INT32) {
        g_nPrintsProElement = 11;
        g_nElementByteSize = 4;
    } else if (mode == ELEMENT_MODE_QWORD) {
        g_nPrintsProElement = 16;
        g_nElementByteSize = 8;
    } else if (mode == ELEMENT_MODE_UINT64) {
        g_nPrintsProElement = 18;
        g_nElementByteSize = 8;
    } else if (mode == ELEMENT_MODE_INT64) {
        g_nPrintsProElement = 19;
        g_nElementByteSize = 8;
    }
    // TODO make g_nSymbolsProElement make dynamic if UTF8
}
