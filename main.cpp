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
#include <QApplication>
#include <QDebug>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    std::string strPath1, strPath2;
    bool bDoCompare = false;
    if (argc == 3) {
        strPath1 = argv[1];
        strPath2 = argv[2];
        bDoCompare = true;
    }

    MainWindow w;
    w.show();

    if (bDoCompare)
        w.setFileCombosAndDoCompare(strPath1.c_str(), strPath2.c_str());
    
    return a.exec();
}
