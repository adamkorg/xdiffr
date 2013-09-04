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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "diffdoc.h"


//Used for settings
#define ORG_NAME "AdamKozicki"
#define APP_NAME "xdiffr"


class QDiffTextEdit;
class QSettings;
class QComboBox;
class FoldersDlg;


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    static MainWindow* getInstance();
    QDiffTextEdit* getDiffEdit(int nView);
    CDiffDoc* getDoc() { return &m_diffDoc; }

    void setFileCombosAndDoCompare(const char *pStrPath1, const char *pStrPath2);

    //used in main diff window and folders dialog
    void writeSettingsPathsCombo(QSettings* pSettings, const QComboBox* pCombo, const QString& strSettingName);
    void readSettingsPathsCombo(QSettings* pSettings, QComboBox* pCombo, const QString& strSettingName);
    void addPathComboTextToDropdown(QComboBox* pCombo);


public slots:
    void actionCompare_triggered();
    void actionFolders_triggered();
    void onFoldersDlgDestroyed();
    void onClickPreviousChange();
    void onClickNextChange();
    void onBtnPath1Pressed();
    void onBtnPath2Pressed();
    void onClickAbout();
    void onClickSettings();

private:
    Ui::MainWindow *ui;
    CDiffDoc m_diffDoc;
    static MainWindow* m_pInstance;
    FoldersDlg* m_pFoldersDlg;

    //overrides
    void closeEvent(QCloseEvent *event);
    void paintEvent (QPaintEvent * event);
    void mousePressEvent(QMouseEvent * event);

    void doFileCompare();

    void LoadDocsIntoEditControls();
    void LoadDocIntoEditControl(int nView, QDiffTextEdit* diffEdit);

    void readSettings();
    void writeSettings();

    void drawOutline(int nView);

    void handleClickInOutline(int nView, QPoint posClick);    

    void setStatusBarMsg(const char *pStrText);
};


#endif // MAINWINDOW_H
