// Copyright (c) 2013 Adam Kozicki <adam@adamk.org>
// All rights reserved
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation. For the terms of this
// license, see <http://www.gnu.org/licenses/>.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#ifndef QDIFFTEXTEDIT_H
#define QDIFFTEXTEDIT_H

#include <QTextEdit>
#include "diffdoc.h"

class QDiffTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit QDiffTextEdit(QWidget *parent = 0);
    void init(CDiffDoc* pDiffDoc, int nView) { m_pDiffDoc = pDiffDoc; m_nView=nView;}

    int getLineHeight() { return m_nLineHeight; }
    int getSectionAt(int nLine);
    int getMidLine();
    int getNextChangeLine();
    int getPreviousChangeLine();
    void scrollToLine(int nLine);

signals:
    
public slots:
    
private slots:
    void onVScroll(int n);

protected:
    virtual void paintEvent(QPaintEvent *_event);

    CDiffDoc* m_pDiffDoc;
    int m_nView;
    int m_nLineHeight;
    int m_nTopBorder;
};

#endif // QDIFFTEXTEDIT_H
