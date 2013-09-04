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

#include "settingsdlg.h"
#include <QTabWidget>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QPainter>
#include <QDebug>
#include "mainwindow.h"
#include "diffdoc.h"
#include <QColorDialog>
#include <QInputDialog>
#include <QLineEdit>


SettingsDlg::SettingsDlg(QWidget *parent) :
    QDialog(parent)
{
    m_pTabWidget = new QTabWidget;
    m_pTabWidget->addTab(new FoldersTab(), tr("Folders"));
    m_pTabWidget->addTab(new ColoursTab(), tr("Colours"));

    m_pButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(m_pButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_pButtonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_pTabWidget);
    mainLayout->addWidget(m_pButtonBox);
    setLayout(mainLayout);

    setWindowIcon(QIcon(":/resources/images/res/toolbar-settings.png"));
}

void SettingsDlg::accept()
{
    //TODO: write changed settings to registry/ini and enable them.
    //      At the moment the settings are saved immediately when the individual settings are changed.

    QDialog::accept();
}

void SettingsDlg::reject()
{
    QDialog::reject();
}

FoldersTab::FoldersTab(QWidget *parent)
     : QWidget(parent)
{
    CDiffDoc* doc = MainWindow::getInstance()->getDoc();
    bool bExceptionsEnabled = doc->getFolderExceptionsEnabled();

    QCheckBox *pCheckShowSame = new QCheckBox(tr("Show files that are the same"));
    connect(pCheckShowSame, SIGNAL(stateChanged(int)),this, SLOT(onClickCheckShowSame(int)));
    bool bShowSame = doc->getFoldersShowSame();
    pCheckShowSame->setChecked(bShowSame);
    QSpacerItem *pSpacer = new QSpacerItem(10, 20);

    QCheckBox *pCheckExceptions = new QCheckBox(tr("Exception Strings"));
    pCheckExceptions->setChecked(bExceptionsEnabled);
    connect(pCheckExceptions, SIGNAL(stateChanged(int)),this, SLOT(onClickCheckExceptions(int)));
    QLabel *pExceptionsDescr = new QLabel(tr("Compared files with any of these strings in their relative paths will not be shown in the folders dialog list"));
    pExceptionsDescr->setMaximumWidth(300);
    pExceptionsDescr->setWordWrap(true);
    pExceptionsDescr->setIndent(18);

    //Fill exceptions list with entries from registry/ini
    m_pListFolderExceptions = new QListWidget();
    QStringList exceptions = doc->getFolderExceptions();
    for (int n=0; n<exceptions.size(); n++)
      m_pListFolderExceptions->addItem(exceptions[n]);

    m_pBtnAddExc = new QPushButton("Add");
    connect(m_pBtnAddExc, SIGNAL(released()),this, SLOT(onClickBtnAddExc()));
    m_pBtnRemoveExc = new QPushButton("Remove");
    connect(m_pBtnRemoveExc, SIGNAL(released()),this, SLOT(onClickBtnRemoveExc()));
    QVBoxLayout *pRowExcButtons = new QVBoxLayout;
    pRowExcButtons->addSpacerItem(new QSpacerItem(10,40));
    pRowExcButtons->addWidget(m_pBtnAddExc,0);
    pRowExcButtons->addWidget(m_pBtnRemoveExc,0);
    pRowExcButtons->addStretch(1);

    QHBoxLayout *pLayoutExceptions = new QHBoxLayout;
    pLayoutExceptions->addWidget(m_pListFolderExceptions);
    pLayoutExceptions->addLayout(pRowExcButtons);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(pCheckShowSame);
    mainLayout->addSpacerItem(pSpacer);
    mainLayout->addWidget(pCheckExceptions);
    mainLayout->addWidget(pExceptionsDescr);
    mainLayout->addLayout(pLayoutExceptions);
    mainLayout->addStretch(1);
    setLayout(mainLayout);

    enableExceptionsControls(bExceptionsEnabled);
}

void FoldersTab::onClickBtnAddExc()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("File name exception"),
                                         tr("Enter a part of a filename or path:"), QLineEdit::Normal,
                                         "", &ok);
    if (ok && !text.isEmpty()) {
        //add item to UI list and save it to the reg/inifile
        m_pListFolderExceptions->addItem(text);
        CDiffDoc* doc = MainWindow::getInstance()->getDoc();
        QStringList list = doc->getFolderExceptions();
        list.push_back(text);
        doc->saveFolderExceptions(list);
    }
}

void FoldersTab::onClickBtnRemoveExc()
{
    qDeleteAll(m_pListFolderExceptions->selectedItems());

    QStringList list;

    for (int n=0; n<m_pListFolderExceptions->count(); n++) {
        list << m_pListFolderExceptions->item(n)->text();
    }

    CDiffDoc* doc = MainWindow::getInstance()->getDoc();
    doc->saveFolderExceptions(list);
}

void FoldersTab::onClickCheckExceptions(int n)
{
    bool bChecked = (n != 0);

    enableExceptionsControls(bChecked);

    CDiffDoc* doc = MainWindow::getInstance()->getDoc();
    doc->setFolderExceptionsEnabled(bChecked);
}

void FoldersTab::enableExceptionsControls(bool bEnable)
{
    m_pBtnAddExc->setEnabled(bEnable);
    m_pBtnRemoveExc->setEnabled(bEnable);
    m_pListFolderExceptions->setEnabled(bEnable);
}

void FoldersTab::onClickCheckShowSame(int n)
{
    bool bChecked = (n != 0);

    CDiffDoc* doc = MainWindow::getInstance()->getDoc();
    doc->setFoldersShowSame(bChecked);
}


ColoursTab::ColoursTab(QWidget *parent)
     : QWidget(parent)
{
    QGroupBox *pGroupTextColour = new QGroupBox(tr("Text Colour"));

    QVBoxLayout *pLayoutTextColours = new QVBoxLayout;

    m_pLabelIdentical = new QLabel(tr("Identical:"));
    m_pBtnIdentical = new QPushButton(tr("Change"));
    addColourControlsRow(pLayoutTextColours, m_pLabelIdentical, m_pBtnIdentical);
    connect(m_pBtnIdentical, SIGNAL(released()),this, SLOT(onClickChangeIdentical()));
    m_pLabelDifferent = new QLabel(tr("Different:"));
    m_pBtnDifferent = new QPushButton(tr("Change"));
    addColourControlsRow(pLayoutTextColours, m_pLabelDifferent, m_pBtnDifferent);
    connect(m_pBtnDifferent, SIGNAL(released()),this, SLOT(onClickChangeDifferent()));
    m_pLabelOnlyLeft = new QLabel(tr("Only in left:"));
    m_pBtnOnlyLeft = new QPushButton(tr("Change"));
    addColourControlsRow(pLayoutTextColours, m_pLabelOnlyLeft, m_pBtnOnlyLeft);
    connect(m_pBtnOnlyLeft, SIGNAL(released()),this, SLOT(onClickChangeOnlyLeft()));
    m_pLabelOnlyRight = new QLabel(tr("Only in right:"));
    m_pBtnOnlyRight = new QPushButton(tr("Change"));
    addColourControlsRow(pLayoutTextColours, m_pLabelOnlyRight, m_pBtnOnlyRight);
    connect(m_pBtnOnlyRight, SIGNAL(released()),this, SLOT(onClickChangeOnlyRight()));

    pGroupTextColour->setLayout(pLayoutTextColours);

    //Restore defaults button with appropriate sizing.
    QPushButton* pBtnRestoreDefaults = new QPushButton("Restore Defaults");
    pBtnRestoreDefaults->setMaximumWidth(120);
    connect(pBtnRestoreDefaults, SIGNAL(released()),this, SLOT(onClickRestoreDefaults()));

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(pGroupTextColour);
    mainLayout->addWidget(pBtnRestoreDefaults);
    mainLayout->addStretch(1);
    setLayout(mainLayout);

}

void ColoursTab::addColourControlsRow(QVBoxLayout* parent, QLabel* label, QPushButton* btn)
{
    QSpacerItem* spacer = new QSpacerItem(100,10);  //colour block will be drawn here
    QHBoxLayout *row = new QHBoxLayout;
    row->addWidget(label);
    row->addSpacerItem(spacer);
    row->addWidget(btn);
    parent->addLayout(row);
}

void ColoursTab::paintEvent(QPaintEvent *_event)
{
    QWidget::paintEvent( _event );
    QPainter pnt(this);

    CDiffDoc* doc = MainWindow::getInstance()->getDoc();

    drawColourBox(&pnt, m_pLabelIdentical, m_pBtnIdentical, doc->getClrIdentical());
    drawColourBox(&pnt, m_pLabelDifferent, m_pBtnDifferent, doc->getClrDifferent());
    drawColourBox(&pnt, m_pLabelOnlyLeft, m_pBtnOnlyLeft, doc->getClrOnlyLeft());
    drawColourBox(&pnt, m_pLabelOnlyRight, m_pBtnOnlyRight, doc->getClrOnlyRight());
}

void ColoursTab::drawColourBox(QPainter* pnt, QWidget* wgLeft, QWidget* wgRight, QColor clr)
{
    QRect rLbl = wgLeft->rect();
    QPoint ptLblTR = rLbl.topRight();
    QRect rBtn = wgRight->rect();
    QPoint ptBtnBL = rBtn.bottomLeft();
    ptLblTR = wgLeft->mapTo(this, ptLblTR);
    ptBtnBL = wgRight->mapTo(this, ptBtnBL);

    QRect rect(ptLblTR.x()+5, ptLblTR.y()+2, ptBtnBL.x()-ptLblTR.x()-10, ptBtnBL.y()-ptLblTR.y()-4);
    pnt->fillRect(rect, clr);
}

void ColoursTab::onClickChangeIdentical()
{
    QColor clr = MainWindow::getInstance()->getDoc()->getClrIdentical();
    QColor clrNew = QColorDialog::getColor(clr);
    if (clrNew.isValid() && (clr != clrNew)) {
        //Save colour in reg/ini and doc variables
        CDiffDoc* doc = MainWindow::getInstance()->getDoc();
        doc->setClrIdentical(clrNew);
    }
    reloadDiffViews();
}

void ColoursTab::onClickChangeDifferent()
{
    QColor clr = MainWindow::getInstance()->getDoc()->getClrDifferent();
    QColor clrNew = QColorDialog::getColor(clr);
    if (clrNew.isValid() && (clr != clrNew)) {
        MainWindow::getInstance()->getDoc()->setClrDifferent(clrNew);
        reloadDiffViews();
    }
}

void ColoursTab::onClickChangeOnlyLeft()
{
    QColor clr = MainWindow::getInstance()->getDoc()->getClrOnlyLeft();
    QColor clrNew = QColorDialog::getColor(clr);
    if (clrNew.isValid() && (clr != clrNew)) {
        MainWindow::getInstance()->getDoc()->setClrOnlyLeft(clrNew);
        reloadDiffViews();
    }
}

void ColoursTab::onClickChangeOnlyRight()
{
    QColor clr = MainWindow::getInstance()->getDoc()->getClrOnlyRight();
    QColor clrNew = QColorDialog::getColor(clr);
    if (clrNew.isValid() && (clr != clrNew)) {
        MainWindow::getInstance()->getDoc()->setClrOnlyRight(clrNew);
        reloadDiffViews();
    }
}

void ColoursTab::onClickRestoreDefaults()
{
    CDiffDoc* doc = MainWindow::getInstance()->getDoc();
    doc->setClrIdentical(CDiffDoc::CLR_DEFAULT_IDENTICAL);
    doc->setClrDifferent(CDiffDoc::CLR_DEFAULT_DIFFERENT);
    doc->setClrOnlyLeft(CDiffDoc::CLR_DEFAULT_ONLYLEFT);
    doc->setClrOnlyRight(CDiffDoc::CLR_DEFAULT_ONLYRIGHT);
    reloadDiffViews();
}

void ColoursTab::reloadDiffViews()
{
    MainWindow::getInstance()->actionCompare_triggered();  //recompare
    repaint(); //to update colour blocks
}

