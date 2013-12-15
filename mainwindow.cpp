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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QMessageBox"
#include "QFile"
#include "QTextStream"
#include <QSettings>
#include <QCloseEvent>
#include <QPainter>
#include <QDebug>
#include <QScrollBar>
#include "foldersdlg.h"
#include <QFileDialog>
#include <QStringList>
#include <aboutdlg.h>
#include <settingsdlg.h>
#include "math.h"


MainWindow* MainWindow::m_pInstance = NULL;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->textEditDiff1->init(&m_diffDoc, VIEW_LEFT);
    ui->textEditDiff2->init(&m_diffDoc, VIEW_RIGHT);

    m_pInstance = this;
    m_pFoldersDlg = NULL;

    readSettings();

    connect(ui->pushButtonBrowse1,SIGNAL(pressed()),this,SLOT(onBtnPath1Pressed()));
    connect(ui->pushButtonBrowse2,SIGNAL(pressed()),this,SLOT(onBtnPath2Pressed()));

    setStatusBarMsg("Select two file paths and then press the Compare toolbar button.");
}

MainWindow* MainWindow::getInstance()
{
    return m_pInstance;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setStatusBarMsg(const char *pStrText)
{
    ui->statusBar->showMessage(pStrText);
}

void MainWindow::actionCompare_triggered()
{
    doFileCompare();
}

void MainWindow::doFileCompare()
{
    ui->textEditDiff1->setWordWrapMode(QTextOption::NoWrap);

    QString qstrPath2 = ui->comboBoxPath2->currentText();
    std::string strPath1 = ui->comboBoxPath1->currentText().toLocal8Bit().constData();
    std::string strPath2 = ui->comboBoxPath2->currentText().toLocal8Bit().constData();
    if ((strPath1 == "") || (strPath2 == "")) {
        QMessageBox::information(this, APP_NAME, "Paths must be selected before comparing.");
        return;
    }

    setStatusBarMsg("Busy comparing...");

    m_diffDoc.LoadFiles(strPath1.c_str(), strPath2.c_str());
    m_diffDoc.Compare();
    LoadDocsIntoEditControls();

    addPathComboTextToDropdown(ui->comboBoxPath1);
    addPathComboTextToDropdown(ui->comboBoxPath2);

    ui->textEditDiff1->scrollToLine(0);
    ui->textEditDiff2->scrollToLine(0);

    setStatusBarMsg("Done compare");

    repaint();  //to show outline bars
}

//run from folders compare dialog
void MainWindow::setFileCombosAndDoCompare(const char *pStrPath1, const char *pStrPath2)
{
    ui->comboBoxPath1->setEditText(pStrPath1);
    ui->comboBoxPath2->setEditText(pStrPath2);

    doFileCompare();
}

void MainWindow::actionFolders_triggered()
{
    if (m_pFoldersDlg) {
        m_pFoldersDlg->close();
    }
    else {
        m_pFoldersDlg = new FoldersDlg(this, NULL);
        m_pFoldersDlg->setAttribute(Qt::WA_DeleteOnClose);
        m_pFoldersDlg->show();

        connect(m_pFoldersDlg, SIGNAL(destroyed()), this, SLOT(onFoldersDlgDestroyed()));
    }
}

void MainWindow::onFoldersDlgDestroyed()
{
    ui->actionFolders->setChecked(false);

    m_pFoldersDlg = NULL;
}

void MainWindow::addPathComboTextToDropdown(QComboBox *pCombo)
{
    QString qstrPath = pCombo->currentText();
    int nFound = -1;

    //see if the edit box text is already at position 0 in the dropdown, in which case we do nothing.
    if (pCombo->itemText(0) != qstrPath) {
        if ((nFound = pCombo->findText(qstrPath)) != -1)  //otherwise we attempt to find the text in the dropdown in other positions
            pCombo->removeItem(nFound);  //and remove it from the drop down
        pCombo->insertItem(0, qstrPath); //now re-add it at position 0, which is Most Recently Used.
    }

    //if we deleted a dropdown entry then it will also have selected other text in the edit box, so
    //let's re-set the edit box contents.
    pCombo->setEditText(qstrPath);
}

void MainWindow::LoadDocsIntoEditControls()
{
    LoadDocIntoEditControl(VIEW_LEFT, ui->textEditDiff1);
    LoadDocIntoEditControl(VIEW_RIGHT, ui->textEditDiff2);
}

void MainWindow::LoadDocIntoEditControl(int nView, QDiffTextEdit* diffEdit)
{
    diffEdit->clear();

    QColor tcOriginal = diffEdit->textColor();
    QColor tcRed = QColor("red");

    bool bBlankFirstLine = false;

    std::string strAppend;
    strAppend.reserve(100000); //Allocate 100KB in one go to speed up appends

    line_array& lines = m_diffDoc.GetLines(nView);
    section_list& secs = m_diffDoc.GetSecs(nView);

    int nLines = lines.size();
    int nLine = 0;

    int nSecs = secs.size();
    int nSec = 0;

    line_array::iterator it = lines.begin();
    for ( ; it != lines.end(); ++it) {
        bool bDifferent = (it->m_nLink == -1);

        //Append optimisation. Big files were very slow to load because of the string appends.
        //Let's look ahead and generate a big string of all the lines from the current section.
        strAppend = "";
        if (nLine > secs[nSec].m_nLastLine)
            nSec++;
        if ((nLine >= secs[nSec].m_nFirstLine) && (nLine <= secs[nSec].m_nLastLine)) {
            //first set the text colour. Can be Identical, Different, OnlyInLeft, OnlyInRight
            QColor clr;
            if (bDifferent) {
                if (secs[nSec].m_nLink != -1)
                    clr =  m_diffDoc.getClrDifferent();
                else
                    clr = (nView == VIEW_LEFT) ? m_diffDoc.getClrOnlyLeft() : m_diffDoc.getClrOnlyRight();
            }
            else
                clr = m_diffDoc.getClrIdentical();
            diffEdit->setTextColor(clr);

            //now construct a string with all the lines in this section
            while (nLine < secs[nSec].m_nLastLine) {
                strAppend += lines[nLine].m_strLine;
                strAppend += "\n";
                ++it;
                ++nLine;
            }
            strAppend += lines[nLine].m_strLine;
            strAppend += "\n";

            //now add this section's text to the edit control
            diffEdit->appendPlainText(strAppend.c_str());
        }
        nLine++;
    }

    diffEdit->setTextColor(tcOriginal);

    //Reset cursor to top of document
    QTextCursor c = diffEdit->textCursor();
    c.movePosition(QTextCursor::Start);
}

QDiffTextEdit* MainWindow::getDiffEdit(int nView)
{
    return nView == VIEW_LEFT ? ui->textEditDiff1 : ui->textEditDiff2;
}

void MainWindow::readSettings()
{
    QSettings settings(ORG_NAME, APP_NAME);

    settings.beginGroup("MainWindow");
    resize(settings.value("size", size()).toSize());
    move(settings.value("pos", pos()).toPoint());
    settings.endGroup();

    readSettingsPathsCombo(&settings, ui->comboBoxPath1, "filePaths1");
    readSettingsPathsCombo(&settings, ui->comboBoxPath2, "filePaths2");
}

void MainWindow::writeSettings()
{
    QSettings settings(ORG_NAME, APP_NAME);

    settings.beginGroup("MainWindow");
    settings.setValue("size", size());
    settings.setValue("pos", pos());
    settings.endGroup();

    writeSettingsPathsCombo(&settings, ui->comboBoxPath1, "filePaths1");
    writeSettingsPathsCombo(&settings, ui->comboBoxPath2, "filePaths2");
}

void MainWindow::writeSettingsPathsCombo(QSettings* pSettings, const QComboBox* pCombo, const QString& strSettingName)
{
    pSettings->beginWriteArray(strSettingName);
    for (int n=0; n<pCombo->count(); ++n) {
        pSettings->setArrayIndex(n);
        pSettings->setValue("path", pCombo->itemText(n));
    }
    pSettings->endArray();
}

void MainWindow::readSettingsPathsCombo(QSettings* pSettings, QComboBox* pCombo, const QString& strSettingName)
{
    int nSize = pSettings->beginReadArray(strSettingName);
    for (int n=0; n<nSize; ++n) {
        pSettings->setArrayIndex(n);
        pCombo->addItem(pSettings->value("path").toString());
    }
    pSettings->endArray();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    //TODO: Check if user really wants to quit if an doc is open and if diffDir dialog is open. Use event->ignore() to cancel quit.

    writeSettings();
    event->accept();
}

void MainWindow::paintEvent(QPaintEvent *)
{
    if (!m_diffDoc.IsCompared())
        return;

    drawOutline(VIEW_LEFT);
    drawOutline(VIEW_RIGHT);
}

void MainWindow::drawOutline(int nView)
{
    QDiffTextEdit* pDiffEdit = (nView == VIEW_LEFT) ? ui->textEditDiff1 : ui->textEditDiff2;

    QPainter painter(this);

    QPoint posDiff = pDiffEdit->pos();
    qDebug() << "possDiff " << posDiff;
    int nOutlineHeight = pDiffEdit->height() - 2; //taking off 2 pixels seems to give a more accurate height.

    int nOffsetY = 34; //fudge factor. pos() seems to give a slightly unexpected result. Possibly due to the toolbar.
    float fY = posDiff.y()+nOffsetY;  //gets calculated for each bit of the outline view
    float fX = posDiff.x();
    //offset a bit to the left of the control for left view. A bit to the right + control width for right view.
    if (nView == VIEW_LEFT)
        fX -= 8;
    else //RIGHT_VIEW
        fX += (pDiffEdit->width() + 3);

//    painter.drawLine(0,70,500,70);

    line_array lines = m_diffDoc.GetLines(nView);
    int nLines = lines.size();

    int nLineOutlineHeight = ceil(float(nOutlineHeight) / nLines);
    if (nLineOutlineHeight == 0)
        nLineOutlineHeight = 1;
    int nLineOutlineWidth = 5;

    qDebug() << "outlineLineHeight old: " << (nOutlineHeight / nLines) << " new: " << nLineOutlineHeight;

    //for debugging: show top and bottom of outline areas.
//    painter.drawLine(QPoint(0, fY), QPoint(posDiff.x(),fY));
//    painter.drawLine(QPoint(0, fY+nOutlineHeight), QPoint(posDiff.x(),fY+nOutlineHeight));

    for (int nLine=0; nLine<nLines; nLine++)
    {
        if (lines[nLine].m_nLink == -1)
        {
            fY = posDiff.y()+nOffsetY + ((nLine*nOutlineHeight)/nLines);
            painter.fillRect(fX, fY, nLineOutlineWidth, nLineOutlineHeight, Qt::black);
//            qDebug() << "outline fillRect(): " << fX << fY << nLineOutlineWidth << nLineOutlineHeight << posDiff;
        }
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
     if (event->button() == Qt::LeftButton) {
         QPoint point = event->pos();
         handleClickInOutline(VIEW_LEFT, point);
         handleClickInOutline(VIEW_RIGHT, point);
     }
}

void MainWindow::handleClickInOutline(int nView, QPoint posClick)
{
    QDiffTextEdit* pDiffEdit = (nView == VIEW_LEFT) ? ui->textEditDiff1 : ui->textEditDiff2;

    QPoint posDiff = pDiffEdit->pos();
    int nOutlineHeight = pDiffEdit->height() - 2; //taking off 2 pixels seems to give a more accurate height.
    int nLineHeight = pDiffEdit->getLineHeight();

    int nLines = m_diffDoc.GetLines(nView).size();

    int nOffsetY = 34; //fudge factor. pos() seems to give a slightly unexpected result. There must be some GUI element that I'm not aware of.
    int nY = posDiff.y()+nOffsetY;
    int nX = posDiff.x();
    if (nView == VIEW_LEFT)
        nX -= 8;
    else //RIGHT_VIEW
        nX += (pDiffEdit->width() + 3);

    //Calculate position in document and then scroll there
    if (((posClick.x() > nX) && (posClick.x() < (nX+5))) &&
            ((posClick.y() > nY) && (posClick.y() < (nY+nOutlineHeight))))
    {
        //first focus on diffEdit1, so that other window gets scrolled as well
        pDiffEdit->setFocus(Qt::OtherFocusReason);

        //clicked in outline area.
        int nScrollY = (((posClick.y() - nY)*nLineHeight*nLines) / nOutlineHeight) - (nOutlineHeight/2);
        pDiffEdit->verticalScrollBar()->setValue(nScrollY);
//        qDebug() << "mousePressEvent() " << nScrollY;
    }
}

void MainWindow::onClickPreviousChange()
{
    QDiffTextEdit* pDiffEdit1 = ui->textEditDiff1;

    QWidget* pFocus =  QApplication::focusWidget();
    if (pFocus != pDiffEdit1) {
        pDiffEdit1->setFocus(Qt::OtherFocusReason); //set focus, so that other diffEdit gets scrolled
    }

    int nNextChange1 = pDiffEdit1->getPreviousChangeLine();

    if (nNextChange1 != -1)
        pDiffEdit1->scrollToLine(nNextChange1);
    else
        QMessageBox::information(this, APP_NAME, "No more changes.");

    if (pFocus != QApplication::focusWidget())
        pFocus->setFocus(Qt::OtherFocusReason); //we changed the focus above, so let's change it back.

}

void MainWindow::onClickNextChange()
{
    QDiffTextEdit* pDiffEdit1 = ui->textEditDiff1;

    QWidget* pFocus =  QApplication::focusWidget();
    if (pFocus != pDiffEdit1) {
        pDiffEdit1->setFocus(Qt::OtherFocusReason); //set focus, so that other diffEdit gets scrolled
    }

    int nNextChange1 = pDiffEdit1->getNextChangeLine();

    if (nNextChange1 != -1)
        pDiffEdit1->scrollToLine(nNextChange1);
    else
        QMessageBox::information(this, APP_NAME, "No more changes.");

    if (pFocus != QApplication::focusWidget())
        pFocus->setFocus(Qt::OtherFocusReason); //we changed the focus above, so let's change it back.

}

void MainWindow::onBtnPath1Pressed()
{
    QString strPath = ui->comboBoxPath1->currentText();
    QStringList fileNames = QFileDialog::getOpenFileNames(this, "Select first file", strPath);
    if (fileNames.size() == 0)
        return;

    ui->comboBoxPath1->setEditText(fileNames[0]);

    //if another file is selected then use this as combo2 value
    if (fileNames.size() > 1)
        ui->comboBoxPath2->setEditText(fileNames[1]);
}

void MainWindow::onBtnPath2Pressed()
{
    QString strPath = ui->comboBoxPath2->currentText();
    QString fileName = QFileDialog::getOpenFileName(this, "Select second file", strPath);
    if (fileName == NULL)
        return;

    ui->comboBoxPath2->setEditText(fileName);
}

void MainWindow::onClickAbout()
{
    //QMessageBox::about(this, "xdiffr", "Created by Adam Kozicki");
    AboutDlg dlg;
    dlg.exec();
}

void MainWindow::onClickSettings()
{
    SettingsDlg dlg;
    dlg.exec();
}

