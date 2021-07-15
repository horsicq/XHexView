// copyright (c) 2020-2021 hors<horsicq@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
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

    ui->checkBoxHex->setChecked(true);
}

XHexViewWidget::~XHexViewWidget()
{
    delete ui;
}

void XHexViewWidget::setShortcuts(XShortcuts *pShortcuts)
{
    ui->scrollAreaHex->setShortcuts(pShortcuts);
    XShortcutsWidget::setShortcuts(pShortcuts);
}

void XHexViewWidget::setData(QIODevice *pDevice, XHexView::OPTIONS options)
{
    ui->scrollAreaHex->setData(pDevice,options);
}

void XHexViewWidget::reload()
{
    ui->scrollAreaHex->reload(true);
}

void XHexViewWidget::setReadonly(bool bState)
{
    Q_UNUSED(bState)
    // TODO
}

void XHexViewWidget::enableReadOnly(bool bState)
{
    Q_UNUSED(bState)
    // TODO
}

void XHexViewWidget::setEdited(bool bState)
{
    Q_UNUSED(bState)
    // TODO
}

qint64 XHexViewWidget::getStartAddress()
{
    return ui->scrollAreaHex->getStartAddress();
}

void XHexViewWidget::setSelection(qint64 nOffset, qint64 nSize)
{
    ui->scrollAreaHex->setSelection(nOffset,nSize);
    ui->scrollAreaHex->goToOffset(nOffset);
}

void XHexViewWidget::errorMessageSlot(QString sErrorMessage)
{
    QMessageBox::critical(this,tr("Error"),sErrorMessage);
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

void XHexViewWidget::on_checkBoxHex_stateChanged(int nState)
{
    Q_UNUSED(nState)

    adjust();
}
