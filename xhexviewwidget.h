/* Copyright (c) 2020-2023 hors<horsicq@gmail.com>
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
#ifndef XHEXVIEWWIDGET_H
#define XHEXVIEWWIDGET_H

#include "xhexview.h"
#include "dialogdatainspector.h"

namespace Ui {
class XHexViewWidget;
}

class XHexViewWidget : public XShortcutsWidget {
    Q_OBJECT

    //    enum DATAINS {
    //        //        DATAINS_BINARY=0,
    //        DATAINS_BYTE,
    //        DATAINS_WORD,
    //        DATAINS_DWORD,
    //        DATAINS_QWORD,
    //        DATAINS_UINT8,
    //        DATAINS_INT8,
    //        DATAINS_UINT16,
    //        DATAINS_INT16,
    //        DATAINS_UINT32,
    //        DATAINS_INT32,
    //        DATAINS_UINT64,
    //        DATAINS_INT64,
    //        // TODO Strings
    //    };

    //    enum LIED {
    //        //        LIED_BINARY,
    //        LIED_BYTE,
    //        LIED_WORD,
    //        LIED_DWORD,
    //        LIED_QWORD,
    //        LIED_UINT8,
    //        LIED_INT8,
    //        LIED_UINT16,
    //        LIED_INT16,
    //        LIED_UINT32,
    //        LIED_INT32,
    //        LIED_UINT64,
    //        LIED_INT64,
    //        __LIED_size
    //    };

public:
    explicit XHexViewWidget(QWidget *pParent = nullptr);
    ~XHexViewWidget();

    void setGlobal(XShortcuts *pShortcuts, XOptions *pXOptions);
    void setData(QIODevice *pDevice, XHexView::OPTIONS options);
    void setDevice(QIODevice *pDevice);
    void setBackupDevice(QIODevice *pDevice);
    void setXInfoDB(XInfoDB *pXInfoDB);
    void reload();
    void setReadonly(bool bState);
    void setReadonlyVisible(bool bState);
    void setEdited(qint64 nDeviceOffset, qint64 nDeviceSize);
    qint64 getStartAddress();
    void setSelection(qint64 nOffset, qint64 nSize);
    //    void blockSignals(bool bState);
    //    void addValue(QString sTitle, DATAINS datains, LIED lied);

private slots:
    void cursorChangedSlot(qint64 nOffset);
    void selectionChangedSlot();
    void adjust();
    void on_checkBoxReadonly_toggled(bool bChecked);
    //    void valueChangedSlot(quint64 nValue);
    //    void setValue(quint64 nValue, DATAINS nType);
    void on_pushButtonDataInspector_clicked();

signals:
    void dataChanged(qint64 nDeviceOffset, qint64 nDeviceSize);
    void showOffsetDisasm(qint64 nDeviceOffset);
    void showOffsetMemoryMap(qint64 nDeviceOffset);
    void selectionChanged(qint64 nDeviceOffset, qint64 nSize);

protected:
    virtual void registerShortcuts(bool bState);

private:
    Ui::XHexViewWidget *ui;
    //    bool g_bIsEdited;
    //    XLineEditHEX *g_lineEdit[__LIED_size];
    //    bool g_bIsDataInspector;
};

#endif  // XHEXVIEWWIDGET_H
