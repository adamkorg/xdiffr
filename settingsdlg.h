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

#ifndef SETTINGSDLG_H
#define SETTINGSDLG_H

#include <QDialog>

class QTabWidget;
class QDialogButtonBox;
class QListWidget;
class QLabel;
class QPushButton;
class QVBoxLayout;


class SettingsDlg : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDlg(QWidget *parent = 0);
    
signals:
    
public slots:
    virtual void accept();
    virtual void reject();
    
private:
    QTabWidget* m_pTabWidget;
    QDialogButtonBox *m_pButtonBox;
};


class FoldersTab : public QWidget
{
    Q_OBJECT

public:
    FoldersTab(QWidget *parent = 0);

private slots:
    void onClickBtnAddExc();
    void onClickBtnRemoveExc();
    void onClickCheckExceptions(int n);
    void onClickCheckShowSame(int n);

private:
    QListWidget* m_pListFolderExceptions;
    QPushButton *m_pBtnAddExc;
    QPushButton *m_pBtnRemoveExc;

    void enableExceptionsControls(bool bEnable);
};

class ColoursTab : public QWidget
{
    Q_OBJECT

public:
    ColoursTab(QWidget *parent = 0);

protected:
    virtual void paintEvent(QPaintEvent *_event);
    void drawColourBox(QPainter* pnt, QWidget* wgLeft, QWidget* wgRight, QColor clr);

    void addColourControlsRow(QVBoxLayout* parent, QLabel* label, QPushButton* btn);

    void reloadDiffViews();


//    enum ColourIndex { ciSame, ciDiff, ciOnlyLeft, ciOnlyRight };

    QLabel *m_pLabelIdentical;
    QLabel *m_pLabelDifferent;
    QLabel *m_pLabelOnlyLeft;
    QLabel *m_pLabelOnlyRight;
    QPushButton *m_pBtnIdentical;
    QPushButton *m_pBtnDifferent;
    QPushButton *m_pBtnOnlyLeft;
    QPushButton *m_pBtnOnlyRight;

private slots:
    void onClickChangeIdentical();
    void onClickChangeDifferent();
    void onClickChangeOnlyLeft();
    void onClickChangeOnlyRight();
    void onClickRestoreDefaults();
};


#endif // SETTINGSDLG_H
