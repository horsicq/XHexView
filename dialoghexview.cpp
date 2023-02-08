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
#include "dialoghexview.h"

#include "ui_dialoghexview.h"

DialogHexView::DialogHexView(QWidget *pParent) : XShortcutsDialog(pParent), ui(new Ui::DialogHexView)
{
    ui->setupUi(this);

    setWindowFlags(Qt::Window);

    connect(ui->widgetHex, SIGNAL(dataChanged(qint64, qint64)), this, SIGNAL(dataChanged(qint64, qint64)));

    ui->widgetHex->setReadonlyVisible(true);
}

DialogHexView::DialogHexView(QWidget *pParent, QIODevice *pDevice, XHexView::OPTIONS options, QIODevice *pBackupDevice) : DialogHexView(pParent)
{
    setData(pDevice, options, pBackupDevice);
}

void DialogHexView::setData(QIODevice *pDevice, XHexView::OPTIONS options, QIODevice *pBackupDevice)
{
    ui->widgetHex->setData(pDevice, options);
    ui->widgetHex->setBackupDevice(pBackupDevice);

    if (options.sTitle != "") {
        setWindowTitle(options.sTitle);
    }
}

DialogHexView::~DialogHexView()
{
    delete ui;
}

void DialogHexView::setGlobal(XShortcuts *pShortcuts, XOptions *pXOptions)
{
    ui->widgetHex->setGlobal(pShortcuts, pXOptions);
    XShortcutsDialog::setGlobal(pShortcuts, pXOptions);
}

void DialogHexView::on_pushButtonClose_clicked()
{
    this->close();
}
