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

#include "foldersdlg.h"
#include "mainwindow.h"
#include <sstream>
#include <QPushButton>
#include <QHBoxLayout>
#include <QComboBox>
#include <QMessageBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QDir>
#include <QTextStream>
#include <QDateTime>
#include <QSettings>
#include <QCloseEvent>
#include <QFileDialog>
#include <QDebug>
#include <QCoreApplication>


#define FINDOTHER_END		-1
#define FINDOTHER_NOTFOUND	0
#define FINDOTHER_FOUND		1

//FoldersDlg states
#define FOLDERSSTATE_READY		0
#define FOLDERSSTATE_COMPARING	1
#define FOLDERSSTATE_DONE		2

//text lookup for states
static char *stateLookup[] = {"Select your two folders and press Go.",
                              "Comparing...", "Done."};

//FolderItem states (used in the table widget)
#define FI_STATE_THESAME		0
#define FI_STATE_ONLYIN1		1
#define FI_STATE_ONLYIN2		2
#define FI_STATE_1MORERECENT	3
#define FI_STATE_2MORERECENT	4
#define FI_STATE_ERROR			5

//text lookup for FolderItem states
static char *g_stateLookup[] = {"The same", "Only in folder1", "Only in folder2",
                                "Different - 1 is more recent", "Different - 2 is more recent"};

#define GOBTNLABEL_GO		"Go"
#define GOBTNLABEL_ABORT	"Abort"



FoldersDlg::FoldersDlg(MainWindow* pMainWnd, QWidget *parent, Qt::WindowFlags f) :
    QDialog(parent, f)
{
    //TODO: check registry for saved size and position
    resize(QSize(600,450));  //default size

    createControls();

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_bComparing = false;

    m_pMainWnd = pMainWnd;

    readSettings();

    setWindowIcon(QIcon(":/resources/images/res/toolbar-folderdiff.png"));
}

void FoldersDlg::createControls()
{
    m_pComboPath1 = new QComboBox();
    m_pComboPath1->setEditable(true);
    QPushButton* pBtnPath1 = new QPushButton("...");
    pBtnPath1->setMaximumWidth(16);
    m_pBtnGo = new QPushButton("Go");
    m_pBtnGo->setMaximumWidth(50);
    m_pComboPath2 = new QComboBox();
    m_pComboPath2->setEditable(true);
    QPushButton* pBtnPath2 = new QPushButton("...");
    pBtnPath2->setMaximumWidth(16);
    m_pTable = new QTableWidget();
    m_pTable->setColumnCount(2);
    m_pTable->setHorizontalHeaderLabels(QStringList() << "Relative path" << "State");
    QVBoxLayout* vlayout = new QVBoxLayout(this);
    QHBoxLayout* layoutTopRow = new QHBoxLayout(this);
    QHBoxLayout* layoutStatusRow = new QHBoxLayout(this);

    layoutTopRow->addWidget(m_pComboPath1,1);
    layoutTopRow->addWidget(pBtnPath1);
    layoutTopRow->addSpacing(15);
    layoutTopRow->addWidget(m_pBtnGo);
    layoutTopRow->addSpacing(15);
    layoutTopRow->addWidget(m_pComboPath2,1);
    layoutTopRow->addWidget(pBtnPath2);

    m_pStatusText = new QLabel("Done.");
    m_pStatusCount = new QLabel("0 Objects.");
    m_pStatusCount->setMaximumWidth(100);
    m_pStatusCount->setAlignment(Qt::AlignRight);
    layoutStatusRow->addWidget(m_pStatusText);
    layoutStatusRow->addWidget(m_pStatusCount);

    vlayout->addLayout(layoutTopRow);
    vlayout->addWidget(m_pTable);
    vlayout->addLayout(layoutStatusRow);

    setLayout(vlayout);

    m_pTable->setColumnWidth(0,360);
    m_pTable->setColumnWidth(1,160);
    m_pTable->verticalHeader()->setDefaultSectionSize(18);  //the default vertical size of the rows is quite big, so lets make it a bit smaller.
    m_pTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_pTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(pBtnPath1,SIGNAL(pressed()),this,SLOT(onBtnPath1Pressed()));
    connect(pBtnPath2,SIGNAL(pressed()),this,SLOT(onBtnPath2Pressed()));
    connect(m_pBtnGo,SIGNAL(pressed()),this,SLOT(onBtnGoPressed()));
    connect(m_pTable, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(tableItemDblClicked(int,int)));
}

void FoldersDlg::onBtnPath1Pressed()
{
    QString strPath = m_pComboPath1->currentText();

    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly);
    if (strPath != "")
        dialog.setDirectory(strPath);
    if (dialog.exec()){
        QStringList listFiles = dialog.selectedFiles();
        if (listFiles.size()>0) {
            m_pComboPath1->setEditText(listFiles[0]);
        }
    }
}

void FoldersDlg::onBtnPath2Pressed()
{
    QString strPath = m_pComboPath1->currentText();

    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly);
    if (strPath != "")
        dialog.setDirectory(strPath);
    if (dialog.exec()){
        QStringList listFiles = dialog.selectedFiles();
        if (listFiles.size()>0) {
            m_pComboPath2->setEditText(listFiles[0]);
        }
    }
}

void FoldersDlg::onBtnGoPressed()
{
    //TODO: check if a compare is already in progress. If so, then cancel it.

    if ((getStrFolder1() == "") || (getStrFolder2() == "")) {
        QMessageBox::information(this, APP_NAME, "Paths must be selected before comparing.");
        return;
    }

    m_pBtnGo->setText(GOBTNLABEL_ABORT);

    doCompare();

    m_pBtnGo->setText(GOBTNLABEL_GO);
}

void FoldersDlg::tableItemDblClicked(int row, int column)
{
    //Show in main window and perform a compare.
    std::string strRelativePath = m_pTable->item(row, 0)->text().toLocal8Bit().constData();
    std::string strPath1 = getStrFolder1();
    strPath1 += strRelativePath;
    std::string strPath2 = getStrFolder2();
    strPath2 += strRelativePath;

    m_pMainWnd->setFileCombosAndDoCompare(strPath1.c_str(), strPath2.c_str());
}

void FoldersDlg::addTableRow(const char *pStrRelativePath, const char *pStrState)
{
    int row = m_pTable->rowCount();
    m_pTable->insertRow(row);
    QTableWidgetItem *newItemPath = new QTableWidgetItem(pStrRelativePath);
    m_pTable->setItem(row, 0, newItemPath);
    QTableWidgetItem *newItemState = new QTableWidgetItem(pStrState);
    m_pTable->setItem(row, 1, newItemState);

//    m_pTable->repaint();  //This seems to cause a lock up. Find another way to refresh the list

    //The refresh calls below are slow, so we'll only do them every 1000 inserts (takes about a second)
    if ((row == 100) || (row % 1000 == 0)) {
        updateCount();
        QCoreApplication::processEvents();
    }
}

void FoldersDlg::addTableRow(const char *pStrRelativePath, int nState)
{
    addTableRow(pStrRelativePath, g_stateLookup[nState]);
}

void FoldersDlg::doCompare()
{
    m_pTable->setRowCount(0);  //clear all existing rows

    setStatus(FOLDERSSTATE_COMPARING);
    QCoreApplication::processEvents(); //update UI

    std::string strPath1 = m_pComboPath1->currentText().toLocal8Bit().constData();
    std::string strPath2 = m_pComboPath2->currentText().toLocal8Bit().constData();

    compareFolders(strPath1.c_str(), strPath2.c_str());

    setStatus(FOLDERSSTATE_DONE);

    updateCount();

    m_pMainWnd->addPathComboTextToDropdown(m_pComboPath1);
    m_pMainWnd->addPathComboTextToDropdown(m_pComboPath2);

    //TODO: set go button back to Go. Check for app exit.
}

void FoldersDlg::updateCount()
{
    std::stringstream oss;
    oss << m_pTable->rowCount() << " Objects.";
    m_pStatusCount->setText(oss.str().c_str());
}

void FoldersDlg::setStatus(int nState)
{
    m_pStatusText->setText(stateLookup[nState]);
}

void FoldersDlg::compareFolders(const char *pStrPath1, const char *pStrPath2)
{
//	DeletePathLists();

    m_bComparing = true;

    CDiffDirectoryNode ddn(this);
    ddn.ReadDirectoryTree(pStrPath1, pStrPath2);
    ddn.DoCompare();

    m_bComparing = false;
}

void FoldersDlg::compareFile(const char *pStrPath1, const char *pStrPath2)
{
    //Use qt file classes to read the two files and compare their contents.
    //Then add an entry to the table widget.

    //work out relative path
    std::string strRelativePath;
    if (strcmp(pStrPath1, "") != 0)
    {
        strRelativePath = pStrPath1;
        std::string strFolder1 = getStrFolder1();
        strRelativePath = strRelativePath.substr(strFolder1.length());
    }
    else if (strcmp(pStrPath2, "") != 0)
    {
        strRelativePath = pStrPath2;
        std::string strFolder2 = getStrFolder2();
        strRelativePath = strRelativePath.substr(strFolder2.length());
    }
    else
    {
        Q_ASSERT(0);
        return;  //something has gone wrong - both paths seem to be empty
    }

//    qDebug() << strRelativePath.c_str();

    //check if relative path contains any exception strings
    if (doesPathContainAnException(strRelativePath.c_str()))
        return;

    //open files
    QFile file1(pStrPath1);
    if (!file1.open(QIODevice::ReadOnly)) {
        addTableRow(strRelativePath.c_str(), FI_STATE_ONLYIN2);
        return;
    }
    QFile file2(pStrPath2);
    if (!file2.open(QIODevice::ReadOnly)) {
        addTableRow(strRelativePath.c_str(), FI_STATE_ONLYIN1);
        return;
    }

    //read files and compare contents
    QTextStream in1(&file1);
    QTextStream in2(&file2);
    QString line1, line2;
    bool bDifferent = false;
    while (!in1.atEnd() && !in2.atEnd()) {
        QString line1 = in1.readLine();
        QString line2 = in2.readLine();
        if (line1 != line2) {
            bDifferent = true;
            break;
        }
    }

    bool bShowSameSetting = MainWindow::getInstance()->getDoc()->getFoldersShowSame();
    if (!bDifferent) {
        if (bShowSameSetting)
            addTableRow(strRelativePath.c_str(), FI_STATE_THESAME);
        return;
    }

    //files are different, so find out which is more recent
    QFileInfo info1(pStrPath1);
    QFileInfo info2(pStrPath2);
    QDateTime dt1 = info1.lastModified();
    QDateTime dt2 = info2.lastModified();
    int nState;
    if (dt1 > dt2)
        nState = FI_STATE_1MORERECENT;
    else if (dt2 > dt1)
        nState = FI_STATE_2MORERECENT;
    else
        nState = FI_STATE_ERROR; //this should never happen (files are different but created at exactly the same time!)

    addTableRow(strRelativePath.c_str(), nState);
}

bool FoldersDlg::doesPathContainAnException(const std::string& strPath)
{
    //first check if exceptions are enabled
    CDiffDoc* doc = MainWindow::getInstance()->getDoc();
    if (!doc->getFolderExceptionsEnabled())
        return false;

    //go through each exception checking if our relative path (strPath) contains any of them.
    QStringList exceptions = doc->getFolderExceptions();
    for (int n=0; n<exceptions.size(); n++) {
        if (strPath.find(exceptions.at(n).toLocal8Bit().constData()) != std::string::npos)
            return true;
    }

    return false;
}

std::string FoldersDlg::getStrFolder1()
{
    return m_pComboPath1->currentText().toLocal8Bit().constData();
}

std::string FoldersDlg::getStrFolder2()
{
    return m_pComboPath2->currentText().toLocal8Bit().constData();
}

void FoldersDlg::readSettings()
{
    QSettings settings(ORG_NAME, APP_NAME);

    settings.beginGroup("FoldersDialog");
    resize(settings.value("size", size()).toSize());
    move(settings.value("pos", pos()).toPoint());
    settings.endGroup();

    m_pMainWnd->readSettingsPathsCombo(&settings, m_pComboPath1, "folderPaths1");
    m_pMainWnd->readSettingsPathsCombo(&settings, m_pComboPath2, "folderPaths2");
}

void FoldersDlg::writeSettings()
{
    QSettings settings(ORG_NAME, APP_NAME);

    settings.beginGroup("FoldersDialog");
    settings.setValue("size", size());
    settings.setValue("pos", pos());
    settings.endGroup();

    m_pMainWnd->writeSettingsPathsCombo(&settings, m_pComboPath1, "folderPaths1");
    m_pMainWnd->writeSettingsPathsCombo(&settings, m_pComboPath2, "folderPaths2");
}

void FoldersDlg::closeEvent(QCloseEvent *event)
{
    writeSettings();
    event->accept();
}

////////////////////////////////////////


bool CDiffDirectoryNode::ReadDirectoryTree(const char *pStrPath1, const char *pStrPath2)
{
    ParseDirectory(pStrPath1, true);
    ParseDirectory(pStrPath2, false);
    return true;
}

bool CDiffDirectoryNode::ParseDirectory(const char *pStrPath, bool bLeft)
{
//	using namespace boost::filesystem;

//	if (!exists(pStrPath))
//		return false;
    QDir dir(pStrPath);
    if (!dir.exists())
        return false;

    if (bLeft)
        m_strLeftPath = pStrPath;
    else
        m_strRightPath = pStrPath;

    QFileInfoList list = dir.entryInfoList(QDir::Files|QDir::Dirs|QDir::NoDotAndDotDot, QDir::DirsFirst);
    for (int i=0; i<list.size(); ++i) {
//	directory_iterator end ;
//	for( directory_iterator it(pStrPath) ; it != end ; ++it )
//	{
        QFileInfo fileInfo = list.at(i);
        std::string strPath = fileInfo.filePath().toLocal8Bit().constData();
        std::string strFilename = fileInfo.fileName().toLocal8Bit().constData();
//		std::string strPath = it->path().string();
//		std::string strFilename = it->path().filename().string();
//		if (is_directory(*it))
        if (fileInfo.isDir())
        {
            std::map<std::string, CDiffDirectoryNode>::iterator itFind = m_mapDirectories.find(strFilename);
            if (itFind != m_mapDirectories.end())
                itFind->second.ParseDirectory(strPath.c_str(), bLeft); //directory node already exists, so this must be a right item
            else
            { //not found, so let's create a new entry and continue parsing inside that
                m_mapDirectories[strFilename] = CDiffDirectoryNode(m_pFoldersDlg);
                m_mapDirectories[strFilename].ParseDirectory(strPath.c_str(), bLeft);
            }
        }
        else
        {
            std::map<std::string, CDiffFileNode>::iterator itFind = m_mapFiles.find(strFilename);
            if (itFind != m_mapFiles.end())
                itFind->second.m_inFile = CDiffFileNode::both;  //if there is already an entry then it must have been for left and this must be right, so we have both.
            else
            {
                CDiffFileNode file;
                file.m_strFilename = strFilename;
                file.m_inFile = bLeft ? CDiffFileNode::left : CDiffFileNode::right;
                m_mapFiles[strFilename] = file;
            }
        }
    }
    return true;
}

bool CDiffDirectoryNode::DoCompare()
{
    //compare all files in this directory and then call DoCompare() for each sub-directory

    //files
    std::map<std::string, CDiffFileNode>::iterator itFile = m_mapFiles.begin();
    for ( ; itFile != m_mapFiles.end(); ++itFile)
    {
        std::string strLeftFile = itFile->second.m_strFilename;
        std::string strLeftPath = "";
        if ((strLeftFile != "") && (m_strLeftPath != "")) {
            strLeftPath = m_strLeftPath;
            //XDUtil::CombinePaths(strLeftPath, strLeftFile.c_str());
            strLeftPath = combinePaths(strLeftPath.c_str(),strLeftFile.c_str()).toLocal8Bit().constData();
        }

        std::string strRightFile = itFile->second.m_strFilename;
        std::string strRightPath = "";
        if ((strRightFile != "") && (m_strRightPath != "")) {
            strRightPath = m_strRightPath;
            //XDUtil::CombinePaths(strRightPath, strRightFile.c_str());
            strRightPath = combinePaths(strRightPath.c_str(),strRightFile.c_str()).toLocal8Bit().constData();
        }

        m_pFoldersDlg->compareFile(strLeftPath.c_str(), strRightPath.c_str());  //does file compare and updates UI
    }

    //directories
    std::map<std::string, CDiffDirectoryNode>::iterator itDir = m_mapDirectories.begin();
    for ( ; itDir != m_mapDirectories.end(); ++itDir)
    {
        itDir->second.DoCompare();
    }

    return false;
}

QString CDiffDirectoryNode::combinePaths(const QString& strPath1, const QString& strPath2)
{
//    qDebug() << "combinePaths" << strPath1 << strPath2;
    return QDir::cleanPath(strPath1 + QDir::separator() + strPath2);
/*
    if (strPath1.endsWith(QDir::separator())) {
        if (strPath2.startsWith(QDir::separator()))
            return strPath1 + strPath2.right(strPath2.size()-1);
        else
            return strPath1 + strPath2;
    }
    else {
        if (strPath2.startsWith(QDir::separator()))
            return strPath1 + strPath2;
        else
            return strPath1 + QDir::separator() + strPath2;
    }
*/
}


