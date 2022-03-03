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
#ifndef DIALOGHEXVIEW_H
#define DIALOGHEXVIEW_H

#include "xhexview.h"

namespace Ui {
class DialogHexView;
}

class DialogHexView : public XShortcutsDialog
{
    Q_OBJECT

public:
    explicit DialogHexView(QWidget *pParent);
    explicit DialogHexView(QWidget *pParent,QIODevice *pDevice,XHexView::OPTIONS options,QIODevice *pBackupDevice=nullptr);
    ~DialogHexView();

    void setData(QIODevice *pDevice,XHexView::OPTIONS options,QIODevice *pBackupDevice=nullptr);

    void setGlobal(XShortcuts *pShortcuts,XOptions *pXOptions);

signals:
    void editState(bool bState);

private slots:
    void on_pushButtonClose_clicked();

private:
    Ui::DialogHexView *ui;
};

#endif // DIALOGHEXVIEW_H
