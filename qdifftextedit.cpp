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

#include "qdifftextedit.h"
#include <QPainter>
#include <QDebug>
#include <QScrollBar>
#include <math.h>
#include "mainwindow.h"
#include <QApplication>


#define OTHERVIEW(n) ((!(n-1))+1)


const std::string QDiffTextEdit::DEFAULT_FONT_NAME = "Monospace";
const int QDiffTextEdit::DEFAULT_FONT_SIZE = 8;
const int QDiffTextEdit::DEFAULT_TAB_SIZE = 4;

QDiffTextEdit::QDiffTextEdit(QWidget *parent) :
    QTextEdit(parent),
    m_font(DEFAULT_FONT_NAME.c_str(), DEFAULT_FONT_SIZE)
{
    m_font.setStyleHint(QFont::TypeWriter);
    setTextAttributes();

    m_pDiffDoc = NULL;
    m_nView = 0;
    m_nLineHeight = 0;
    m_nTopBorder = 0;

    connect( verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(onVScroll(int)) );
}

void QDiffTextEdit::paintEvent(QPaintEvent *_event)
{
    QTextEdit::paintEvent( _event );
    QPainter pnt(viewport());

//    QRect charRect;
//    pnt.drawText(0, 0, 1000, 1000, 0, QString("0"), &charRect);

//    int nLineHeight = charRect.height();//pnt.fontMetrics().height();

    QTextCursor c = textCursor();
    c.movePosition(QTextCursor::Start);
    int y1 = cursorRect(c).bottom();
    c.movePosition(QTextCursor::Down);

    pnt.setPen( QColor( 200, 200, 200, 0xff ));

    int y = cursorRect(c).bottom();
//    QRect rect(0, y, viewport()->rect().right(), 0);
//    pnt.drawRect(rect);

    //visible pixels of the document. Needed so we only render the visible section speparators
    int vvalue_min = verticalScrollBar()->value();
    int vvalue_max = pnt.viewport().bottom() + vvalue_min;
    qDebug() << "vvalue_min:" << vvalue_min << "; vvalue_max: " << vvalue_max;

    int nLineHeight = y-y1;
    m_nLineHeight = nLineHeight;
    int nStart = (y1)-nLineHeight;  //there seems to be a bit of a border (of a few pixels) at the top. Calculate this in nStart.

//    for (int n=0; n<10; n++) {
//        QRect rect(0, nStart+(nLineHeight*n), viewport()->rect().right(), 0);
//        pnt.drawRect(rect);
//    }


    if ((m_pDiffDoc == NULL) || (nLineHeight == 0))
        return;

    int nTopBorder = vvalue_min+y1-nLineHeight;
    m_nTopBorder = nTopBorder;

    int nFirstLine = floor((vvalue_min - nTopBorder)/nLineHeight);
    int nLastLine = floor((vvalue_max - nTopBorder)/nLineHeight);

    //let's start drawing from the line before the top most visible one to be sure.
    if (nFirstLine > 0)
        nFirstLine--;

    int nSection = getSectionAt(nFirstLine);
    section_list& secs = m_pDiffDoc->GetSecs(m_nView);
    int nLineSectionEnd = nFirstLine;
    while ((nLineSectionEnd < nLastLine) && (nSection<secs.size())) {
        nLineSectionEnd = secs[nSection].m_nLastLine;
        qDebug() << "nLineSectionEnd: " << nLineSectionEnd << "; nFirstLine: " << nFirstLine; //TEMP
        nSection++;
        QRect rect(0, nStart+(nLineHeight*(nLineSectionEnd+1)), viewport()->rect().right(), 0);
        pnt.drawRect(rect);
    }

}

int QDiffTextEdit::getSectionAt(int nLine)
{
    section_list& secs = m_pDiffDoc->GetSecs(m_nView);
    for (int nSection=0; nSection<secs.size(); nSection++)
    {
        if ((nLine>=secs[nSection].m_nFirstLine) && (nLine<=secs[nSection].m_nLastLine))
            return nSection;
    }

    return -1;
}

void QDiffTextEdit::onVScroll(int n)
{
    //We set the scroll bar position of the other diffEdit view towards the end of this function, and
    //this will cause an onVScroll() call for that other diffEdit. Let's make sure that this doesn't
    //bounce back and forth. So make sure that onVScroll() is only processed if the current focused
    //control is this diffEdit. Otherwise, it is a bounce back and we don't want to process it.
    QWidget* pFocus =  QApplication::focusWidget ();
    if (pFocus != this)
        return;

    //Make sure the diffEdit is initialised.
    if (m_nLineHeight == 0)
        return;

    //Logic:
    //Scroll other diffTextEdit to match the position of this one at the mid line:
    // 1. Calculate mid line number of this diffEdit
    // 2. lines[nMidLine].m_nLink
    // 3. scroll other view so that link line is in the middle vertically.

    //midLineY = (scrollPosY + (windowHeight/2)) / m_nLineHeight;
    int vvalue_min = verticalScrollBar()->value();  //scrollPosY
    int nWndSizeY = viewport()->height();
    int nMidLine = (vvalue_min + (nWndSizeY/2)) / m_nLineHeight;

    //we will need a fractional line offset because of the rounding done to get midLine.
    int nOffset = vvalue_min - ((nMidLine*m_nLineHeight) - (nWndSizeY/2));

    qDebug() << "onSroll[" << m_nView << "] vvalue_min: " << vvalue_min << "; nWndSize: " << nWndSizeY << "; nMidLine: " << nMidLine << " n: " << n << "nOffset: " << nOffset;

    line_array& lines = m_pDiffDoc->GetLines(m_nView);
    int nMidLineOther = lines[nMidLine].m_nLink;
    //TODO: refactor. This is getting messy.
    if (nMidLineOther == -1) {
        int nMidSec = getSectionAt(nMidLine);
        if (nMidSec > 0) {
            section_list& secs = m_pDiffDoc->GetSecs(m_nView);
            int nLink = secs[nMidSec-1].m_nLink;
            if (nLink == -1)
                nLink = secs[nMidSec-1].m_nCorrespond;
            if (nLink != -1){
                section_list& otherSecs = m_pDiffDoc->GetSecs(OTHERVIEW(m_nView));
                nMidLineOther = otherSecs[nLink].m_nLastLine + 1;
            }
        }
    }

    int nOtherY = (nMidLineOther*m_nLineHeight) - (nWndSizeY/2) + nOffset;

    //Get other editCtrl and setScrollPosition on it.
    QDiffTextEdit* viewOther = MainWindow::getInstance()->getDiffEdit(OTHERVIEW(m_nView));
    qDebug() << "set other scroll to: " << nOtherY << " otherView: " << OTHERVIEW(m_nView);
    viewOther->verticalScrollBar()->setValue(nOtherY);
}

int QDiffTextEdit::getMidLine()
{
    int vvalue_min = verticalScrollBar()->value();  //scrollPosY
    int nWndSizeY = viewport()->height();
//    float fMidLine = ((vvalue_min + (nWndSizeY/2)) / m_nLineHeight);
    int nMidLine = ((vvalue_min + (nWndSizeY/2)) / m_nLineHeight);
    qDebug() << "getMidLine() " << nMidLine;// << fMidLine;
    return nMidLine;
}

int QDiffTextEdit::getNextChangeLine()
{
    int nMid = getMidLine();
    int nMidSec = getSectionAt(nMid);
    section_list& secs = m_pDiffDoc->GetSecs(m_nView);

    for (int n=nMidSec + 1; n<secs.size(); n++)
    {
        //make sure this is there is at least one line between the last position and the new position
        int nNextLine = secs[n].m_nFirstLine;
        if (nNextLine > (nMid+1))
            return nNextLine;
    }

    return -1;
}

int QDiffTextEdit::getPreviousChangeLine()
{
    int nMid = getMidLine();
    int nMidSec = getSectionAt(nMid);
    section_list& secs = m_pDiffDoc->GetSecs(m_nView);

    for (int n=nMidSec - 1; n>0; n--)
    {
        int nPrevLine = secs[n].m_nFirstLine;
        if (nPrevLine < (nMid-1))
            return nPrevLine;
    }

    return -1;
}

void QDiffTextEdit::scrollToLine(int nLine)
{
    int nScrollY = ((nLine)*m_nLineHeight) - (viewport()->height()/2);
    if (nScrollY < 0)
        nScrollY = 0;

    verticalScrollBar()->setValue(nScrollY);
}

void QDiffTextEdit::setTextAttributes()
{
    setCurrentFont(m_font);
    setLineWrapMode(QTextEdit::NoWrap);

    QFontMetrics fm(m_font);
    const int tabStop = DEFAULT_TAB_SIZE;  // 4 characters
    setTabStopWidth(tabStop * fm.width(' '));
}

void QDiffTextEdit::appendPlainText(const char *pStrText)
{
    setTextAttributes();
    insertPlainText (pStrText);
}
