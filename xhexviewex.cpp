/* Copyright (c) 2025 hors<horsicq@gmail.com>
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
#include "xhexviewex.h"

XHexViewEx::XHexViewEx(QWidget *pParent) : XHexView(pParent)
{
    addShortcut(X_ID_HEX_STRUCTS, this, SLOT(_structs()));
}

QList<XShortcuts::MENUITEM> XHexViewEx::getMenuItems()
{
    QList<XShortcuts::MENUITEM> listResults = XHexView::getMenuItems();

    getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_STRUCTS, this, SLOT(_structs()), XShortcuts::GROUPID_NONE);

    return listResults;
}

void XHexViewEx::_structs()
{
    // DEVICESTATE deviceState = getDeviceState();

    // DialogSetGenericWidget dialogSetGenericWidget(this);
    // dialogSetGenericWidget.setData(getDevice(), deviceState.nSelectionDeviceOffset, deviceState.nSelectionSize);
    // dialogSetGenericWidget.exec();
    XStructWidget::OPTIONS options = {};

    DialogXStruct dialog(this);
    dialog.setGlobal(getShortcuts(), getGlobalOptions());
    dialog.setData(getDevice(), getXInfoDB(), options);  // TODO options
    connect(this, SIGNAL(currentLocationChanged(quint64, qint32, qint64)), &dialog, SLOT(currentLocationChangedSlot(quint64, qint32, qint64)));
    connect(this, SIGNAL(dataChanged(qint64, qint64)), &dialog, SLOT(dataChangedSlot(qint64, qint64)));
    connect(&dialog, SIGNAL(dataChanged(qint64, qint64)), this, SLOT(_setEdited(qint64, qint64)));
    connect(this, SIGNAL(closeWidget_Structs()), &dialog, SLOT(close()));
    XOptions::_adjustStayOnTop(&dialog, true);
    dialog.exec();
}
