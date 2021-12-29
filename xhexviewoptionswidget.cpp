/* Copyright (c) 2020-2021 hors<horsicq@gmail.com>
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
#include "xhexviewoptionswidget.h"
#include "ui_xhexviewoptionswidget.h"

XHexViewOptionsWidget::XHexViewOptionsWidget(QWidget *pParent) :
    QWidget(pParent),
    ui(new Ui::XHexViewOptionsWidget)
{
    ui->setupUi(this);
}

XHexViewOptionsWidget::~XHexViewOptionsWidget()
{
    delete ui;
}

void XHexViewOptionsWidget::setOptions(XOptions *pOptions)
{
    g_pOptions=pOptions;

    g_pOptions->setLineEdit(ui->lineEditHexFont,XOptions::ID_HEX_FONT);
}

void XHexViewOptionsWidget::save()
{
    g_pOptions->getLineEdit(ui->lineEditHexFont,XOptions::ID_HEX_FONT);
}

void XHexViewOptionsWidget::setDefaultValues(XOptions *pOptions)
{       
#ifdef Q_OS_WIN
    pOptions->addID(XOptions::ID_HEX_FONT,"Courier,10,-1,5,50,0,0,0,0,0");
#endif
#ifdef Q_OS_LINUX
    pOptions->addID(XOptions::ID_HEX_FONT,"Monospace,10,-1,5,50,0,0,0,0,0");
#endif
#ifdef Q_OS_OSX
    pOptions->addID(XOptions::ID_HEX_FONT,"Menlo,10,-1,5,50,0,0,0,0,0"); // TODO Check
#endif
}

void XHexViewOptionsWidget::on_toolButtonHexFont_clicked()
{
    QFont _font;
    _font.fromString(ui->lineEditHexFont->text());

    bool bOK=false;
    _font=QFontDialog::getFont(&bOK,_font,this);

    if(bOK)
    {
        ui->lineEditHexFont->setText(_font.toString());
    }
}
