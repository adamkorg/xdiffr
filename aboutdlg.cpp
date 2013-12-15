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

#include "aboutdlg.h"
#include "ui_aboutdlg.h"
#include "version.h"

AboutDlg::AboutDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDlg)
{
    ui->setupUi(this);

    ui->label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->label->setOpenExternalLinks(true);

    setWindowIcon(QIcon(":/resources/images/res/toolbar-about.png"));

    //Update version information from version.h define
    QString strLabel = ui->label->text();
    strLabel.replace("1.0 build 1", VER_PRODUCTVERSION_STR);
    ui->label->setText(strLabel);

}

AboutDlg::~AboutDlg()
{
    delete ui;
}
