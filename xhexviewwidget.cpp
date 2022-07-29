/* Copyright (c) 2020-2022 hors<horsicq@gmail.com>
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
#include "xhexviewwidget.h"
#include "ui_xhexviewwidget.h"

XHexViewWidget::XHexViewWidget(QWidget *pParent) :
    XShortcutsWidget(pParent),
    ui(new Ui::XHexViewWidget)
{
    ui->setupUi(this);

    connect(ui->scrollAreaHex,SIGNAL(showOffsetDisasm(qint64)),this,SIGNAL(showOffsetDisasm(qint64)));
    connect(ui->scrollAreaHex,SIGNAL(showOffsetMemoryMap(qint64)),this,SIGNAL(showOffsetMemoryMap(qint64)));
    connect(ui->scrollAreaHex,SIGNAL(errorMessage(QString)),this,SLOT(errorMessageSlot(QString)));
    connect(ui->scrollAreaHex,SIGNAL(cursorChanged(qint64)),this,SLOT(cursorChanged(qint64)));
    connect(ui->scrollAreaHex,SIGNAL(selectionChanged()),this,SLOT(selectionChanged()));
    connect(ui->scrollAreaHex,SIGNAL(dataChanged()),this,SIGNAL(dataChanged()));

    ui->checkBoxHex->setChecked(true);

    setReadonlyVisible(false);
    ui->checkBoxReadonly->setChecked(true);
}

XHexViewWidget::~XHexViewWidget()
{
    delete ui;
}

void XHexViewWidget::setGlobal(XShortcuts *pShortcuts,XOptions *pXOptions)
{
    ui->scrollAreaHex->setGlobal(pShortcuts,pXOptions);
    XShortcutsWidget::setGlobal(pShortcuts,pXOptions);
}

void XHexViewWidget::setData(QIODevice *pDevice,XHexView::OPTIONS options)
{
    ui->checkBoxReadonly->setEnabled(pDevice->isWritable());

    ui->scrollAreaHex->setData(pDevice,options);
}

void XHexViewWidget::setDevice(QIODevice *pDevice)
{
    ui->scrollAreaHex->setDevice(pDevice);
}

void XHexViewWidget::setBackupDevice(QIODevice *pDevice)
{
    ui->scrollAreaHex->setBackupDevice(pDevice);
}

void XHexViewWidget::reload()
{
    ui->scrollAreaHex->reload(true);
}

void XHexViewWidget::setReadonly(bool bState)
{
    ui->scrollAreaHex->setReadonly(bState);

    ui->checkBoxReadonly->setChecked(bState);
}

void XHexViewWidget::setReadonlyVisible(bool bState)
{
    if(bState)
    {
        ui->checkBoxReadonly->show();
    }
    else
    {
        ui->checkBoxReadonly->hide();
    }
}

void XHexViewWidget::setEdited()
{
    ui->scrollAreaHex->setEdited();

//    emit changed();
}

qint64 XHexViewWidget::getStartAddress()
{
    return ui->scrollAreaHex->getStartAddress();
}

void XHexViewWidget::setSelection(qint64 nOffset,qint64 nSize)
{
    ui->scrollAreaHex->setSelection(nOffset,nSize);
    ui->scrollAreaHex->goToOffset(nOffset);
}

void XHexViewWidget::cursorChanged(qint64 nOffset)
{
    Q_UNUSED(nOffset)

    adjust();
}

void XHexViewWidget::selectionChanged()
{
    adjust();
}

void XHexViewWidget::adjust()
{
    XAbstractTableView::STATE state=ui->scrollAreaHex->getState();

    QString sCursor;
    QString sSelectionStart;
    QString sSelectionSize;

    if(ui->checkBoxHex->isChecked())
    {
        sCursor=XBinary::valueToHexEx(state.nCursorOffset);
        sSelectionStart=XBinary::valueToHexEx(state.nSelectionOffset);
        sSelectionSize=XBinary::valueToHexEx(state.nSelectionSize);
    }
    else
    {
        sCursor=QString::number(state.nCursorOffset);
        sSelectionStart=QString::number(state.nSelectionOffset);
        sSelectionSize=QString::number(state.nSelectionSize);
    }

    ui->lineEditCursor->setText(sCursor);
    ui->lineEditSelectionStart->setText(sSelectionStart);
    ui->lineEditSelectionSize->setText(sSelectionSize);
}

void XHexViewWidget::registerShortcuts(bool bState)
{
    Q_UNUSED(bState)
}

void XHexViewWidget::on_checkBoxReadonly_toggled(bool bChecked)
{
    ui->scrollAreaHex->setReadonly(bChecked);
}

void XHexViewWidget::on_checkBoxHex_toggled(bool bChecked)
{
    Q_UNUSED(bChecked)

    adjust();
}
