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

#ifndef FOLDERSDLG_H
#define FOLDERSDLG_H

#include <QDialog>
#include <map>

class QTableWidget;
class QLabel;
class QComboBox;
class MainWindow;


class FoldersDlg : public QDialog
{
    Q_OBJECT
public:
    explicit FoldersDlg(MainWindow* pMainWnd, QWidget *parent = 0, Qt::WindowFlags f=0);
    
    void compareFile(const char *pStrPath1, const char *pStrPath2); //compares individual file. Called from CDiffDirectoryNode. Adds entry to table widget.

signals:
    
public slots:
    void onBtnPath1Pressed();
    void onBtnPath2Pressed();
    void onBtnGoPressed();
    void tableItemDblClicked(int row, int column);

private:
    QTableWidget* m_pTable;
    QLabel* m_pStatusText;
    QLabel* m_pStatusCount;
    QComboBox* m_pComboPath1;
    QComboBox* m_pComboPath2;
    QPushButton* m_pBtnGo;

    MainWindow* m_pMainWnd;
    bool m_bComparing;

    //overrides
    void closeEvent(QCloseEvent *event);

    void addTableRow(const char *pStrRelativePath, const char *pStrState);
    void addTableRow(const char *pStrRelativePath, int nState);
    void createControls();
    void setStatus(int nState);
    void updateCount();

    void doCompare();
    void compareFolders(const char *pStrPath1, const char *pStrPath2);

    std::string getStrFolder1();
    std::string getStrFolder2();

    void readSettings();
    void writeSettings();

    bool doesPathContainAnException(const std::string& strPath);

};


//I'm going to use a tree structre to represent the directories being compared.
//CDirectoryNode will contain a map of child CDirectoryNode elements and
//a map of files: CFileNode - which will have an enum for inLeft, inRight, inBoth.

class CDiffFileNode
{
public:
    std::string m_strFilename;
    enum InFile { left, right, both };
    InFile m_inFile;
};

class CDiffDirectoryNode
{
public:
    CDiffDirectoryNode() { m_pFoldersDlg = NULL; }
    CDiffDirectoryNode(FoldersDlg* pFoldersDlg) { m_pFoldersDlg = pFoldersDlg; }
    bool ReadDirectoryTree(const char *pStrPath1, const char *pStrPath2);  //creates internal tree structures, so we know which files to compare
    bool DoCompare();  //compares all the files that appear in both paths and updates the UI with the compare state
private:
    bool ParseDirectory(const char *pStrPath, bool bLeft);  //walks through the directory structure creating a tree of CDiffDirectoryNodes and CDiffFileNodes. Recursive.
    QString combinePaths(const QString& strPath1, const QString& strPath2);
private:
    std::map<std::string, CDiffDirectoryNode> m_mapDirectories;
    std::map<std::string, CDiffFileNode> m_mapFiles;
    std::string m_strLeftPath;
    std::string m_strRightPath;
    FoldersDlg* m_pFoldersDlg;
};


#endif // FOLDERSDLG_H
