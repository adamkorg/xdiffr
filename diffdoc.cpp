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

#include "diffdoc.h"

#include "QFile"
#include "QMessageBox"
#include "QTextStream"
#include "QtDebug"
#include <QSettings>
#include "mainwindow.h"


const QColor CDiffDoc::CLR_DEFAULT_IDENTICAL = QColor(0,0,0);
const QColor CDiffDoc::CLR_DEFAULT_DIFFERENT = QColor(0x99,0x00,0x99);
const QColor CDiffDoc::CLR_DEFAULT_ONLYLEFT  = QColor(0xD0,0x00,0x00);
const QColor CDiffDoc::CLR_DEFAULT_ONLYRIGHT = QColor(0x00,0x00,0xD0);


CDiffDoc::CDiffDoc()
{
    m_bIsCompared = false;
    m_bShowSelections = true;
    m_bAutoSelect = true;
    m_bExceptionStringsEnabled = true; //remove this, should get it from registry/settings pp

    loadClrSettings();
    loadFolderExceptions();
}

CDiffDoc::~CDiffDoc()
{
}

bool CDiffDoc::LoadFile(const char *pStrFilePath, line_array& lines)
{
    QFile file(pStrFilePath);
    if(!file.open(QIODevice::ReadOnly)) {
        QMessageBox::information(0, "error", file.errorString());
        return false;
    }

    lines.clear();

    QTextStream in(&file);

    CLine line;
    while(!in.atEnd()) {
        QString strLine = in.readLine();
        line.m_strLine = strLine.toLocal8Bit().constData();
        lines.push_back(line);
    }

    file.close();

    return true;
}

bool CDiffDoc::LoadFiles(const char *pStrFilePath1, const char *pStrFilePath2)
{
    if (!LoadFile(pStrFilePath1, m_lines1))
        return false;
    if (!LoadFile(pStrFilePath2, m_lines2))
        return false;

    m_secs1.clear();
    m_secs2.clear();

    return true;
}

void CDiffDoc::Compare()
{
    bool bChanges = true;

    while (bChanges)
    {
        bChanges = false;

        CSection secWhole1(0, m_lines1.size()-1, &m_lines1);
        CSection secWhole2(0, m_lines2.size()-1, &m_lines2);

        if (SectionMatch(secWhole1, secWhole2))
            bChanges = true;

        //Temporarily made members, was local vars
        section_list secs1, secs2;
        //MakeSectionList(m_secs1, TRUE, m_lines1);
        //MakeSectionList(m_secs2, FALSE, m_lines2);
        MakeSectionList(secs1, true, m_lines1);
        MakeSectionList(secs2, false, m_lines2);

        //if (MatchSectionLists(m_secs1, m_secs2))
        //	bChanges = TRUE;
        if (MatchSectionLists(secs1, secs2))
            bChanges = true;

        //debugging...
        qDebug() << "SectionList1:\n";
        DebugSectionList(secs1);
        qDebug() << "\n";
        qDebug() << "SectionList2:\n";
        DebugSectionList(secs2);
        qDebug() << "\n";

        if (!bChanges) //last time in loop
            SetFinalSectionLists(secs1, secs2);

    }

//    if (m_bAutoSelect)
//        AutoSelectSections();

    m_bIsCompared = true;
}

bool CDiffDoc::SectionMatch(CSection& section1, CSection& section2)
{
    CLineMap map1, map2;
    bool bLinked = false;

    map1.MakeMap(section1);
    map2.MakeMap(section2);

//	map1.DebugMap();

    for (int nLine=section1.m_nFirstLine; nLine <= section1.m_nLastLine; nLine++)
    {
        if ((m_lines1[nLine].m_nLink < 0) &&
            (map1.GetLine(m_lines1[nLine].m_strLine.c_str()) >= 0) &&
            (map2.GetLine(m_lines1[nLine].m_strLine.c_str()) >= 0))
        {
            int nLine2 = map2.GetLine(m_lines1[nLine].m_strLine.c_str());

            if (m_lines2[nLine2].m_nLink >=0)
                continue;

            if (ExpandAnchor(nLine, nLine2))
                bLinked = true;
        }
    }

    return (bLinked);
}

bool CDiffDoc::LineLink(int nLine1, int nLine2)
{
    if (m_lines1[nLine1].m_strLine == m_lines2[nLine2].m_strLine)
    {
        m_lines1[nLine1].m_nLink = nLine2;
        m_lines2[nLine2].m_nLink = nLine1;
        return true;
    }
    return false;
}

bool CDiffDoc::ExpandAnchor(int nLine1, int nLine2)
{
    bool bChanges = false;
    int nCurLine1, nCurLine2;

    //forwards
    nCurLine2 = nLine2;
    for (nCurLine1=nLine1; nCurLine1<m_lines1.size(); nCurLine1++)
    {
        if (nCurLine2 >= m_lines2.size())
            break;
        if ((m_lines1[nCurLine1].m_nLink >= 0) || (m_lines2[nCurLine2].m_nLink >= 0))
            break;
        if (LineLink(nCurLine1, nCurLine2))
            bChanges = true;
        else
            break;
        nCurLine2++;
    }

    if (!bChanges)
        return false;


    //backwards
    nCurLine2 = nLine2-1;
    for (nCurLine1=nLine1-1; nCurLine1>=0; nCurLine1--)
    {
        if (nCurLine2 < 0)
            break;
        if ((m_lines1[nCurLine1].m_nLink >= 0) || (m_lines2[nCurLine2].m_nLink >= 0))
            break;
        if (!LineLink(nCurLine1, nCurLine2))
            break;
        nCurLine2--;
    }

    return true;
}

void CDiffDoc::MakeSectionList(section_list& secs, bool bLeft, line_array& lines)
{
    bool bMatched;
    int nEnd;

    for (int nLine=0; nLine < lines.size(); nLine++)
    {
        if (lines[nLine].m_nLink != -1)
        {
            nEnd = FindEndOfMatched(lines, nLine);
            bMatched = true;
        }
        else
        {
            nEnd = FindEndOfUnmatched(lines, nLine);
            bMatched = false;
        }

        CSection sec(nLine, nEnd, &lines);
        sec.m_nState = ( bMatched ? STATE_SAME : (bLeft ? STATE_LEFTONLY : STATE_RIGHTONLY) );
        secs.push_back(sec);

        nLine = nEnd; //go to end of section
    }
}

bool CDiffDoc::MatchSectionLists(section_list& secs1, section_list& secs2)
{
    bool bLinked = false;

    //link same sections
    int nSection, nSection2, nLink;
    for (nSection=0; nSection<secs1.size(); nSection++)
    {
        if (secs1[nSection].m_nState == STATE_SAME)
        {
            nLink = m_lines1[secs1[nSection].m_nFirstLine].m_nLink;
            for (nSection2=0; nSection2<secs2.size(); nSection2++)
            {
                if (nLink == secs2[nSection2].m_nFirstLine)
                {
                    secs1[nSection].m_nLink = nSection2;
                    secs2[nSection2].m_nLink = nSection;
                }

            }
        }
    }

    //link corresponding/diff sections
    int nOtherSection;
    for (nSection=0; nSection<secs1.size(); nSection++)
    {
        if (secs1[nSection].m_nState == STATE_SAME)
            continue;

        //match up nSection with a corresponding section in the other file (nOtherSection)
        if (nSection != 0)
            nOtherSection = secs1[nSection-1].m_nLink + 1;  //up one section -> across to other file -> down to next section.
        else if (secs1.size()>1)
        {
            nOtherSection = secs1[nSection+1].m_nLink -1;  //down one section -> across to other file -> up to previous section.
            if (nOtherSection == -1)
                nOtherSection = 0; //we correspond to the first section in the other file.
            //above if() fixes the bug Andy spotted where we have an inserted line at the
            //beginning of the first file.
        }
        else
            break;

        if (nOtherSection >= secs2.size()) //last section
            secs1[nSection].m_nCorrespond = secs2.size();
        else if (secs2[nOtherSection].m_nState != STATE_SAME)
        {
            secs1[nSection].m_nLink = nOtherSection;
            secs2[nOtherSection].m_nLink = nSection;

            //addition for build 14, thorough diffing
            if (SectionMatch(secs1[nSection], secs2[nOtherSection]))
                bLinked = true;
        }
        else   //only in left file (ie. deleted section)
            secs1[nSection].m_nCorrespond = nOtherSection;
    }

    //all thats left is correspond(inserted) sections in the second file
    for (nSection2=0; nSection2<secs2.size(); nSection2++)
    {
        if ((secs2[nSection2].m_nLink == -1) && (secs2[nSection2].m_nCorrespond == -1))
        {
            if (nSection2 != 0)
                nOtherSection = secs2[nSection2-1].m_nLink + 1;
            else if (secs1.size()>1)
                nOtherSection = secs2[nSection2+1].m_nLink -1;
            else
                break;

            secs2[nSection2].m_nCorrespond = nOtherSection;
        }
    }

    return bLinked;
}

int CDiffDoc::FindEndOfMatched(line_array& lines, int nStart)
{
    Q_ASSERT(lines[nStart].m_nLink > -1);

    int nOldLink = lines[nStart].m_nLink;

    for (;;)
    {
        nStart++;
        if ((nStart) == lines.size())
            break;
        if (lines[nStart].m_nLink == -1)
            break;
        if (lines[nStart].m_nLink!=(nOldLink+1))
            break;
        nOldLink = lines[nStart].m_nLink;
    }

    return nStart-1;
}

int CDiffDoc::FindEndOfUnmatched(line_array& lines, int nStart)
{
    Q_ASSERT(lines[nStart].m_nLink == -1);

    for (;;)
    {
        nStart++;
        if ((nStart) == lines.size())
            break;
        if (lines[nStart].m_nLink > -1)
            break;
    }

    return nStart-1;
}

void CDiffDoc::SetFinalSectionLists(section_list& secs1, section_list& secs2)
{
    m_secs1 = secs1;
    m_secs2 = secs2;
}

void CDiffDoc::DebugSectionList(section_list& secs)
{
    for (int nSection=0; nSection<secs.size(); nSection++)
    {
        int nState = secs[nSection].m_nState;
        int nLink = secs[nSection].m_nLink, nCorrespond = secs[nSection].m_nCorrespond, nFirstLine = secs[nSection].m_nFirstLine, nLastLine=secs[nSection].m_nLastLine;
        qDebug() << "DS: n:" << nSection << ", state:" << nState << ", link:"<< nLink << ", corr:" << nCorrespond << ", firstL:" << nFirstLine << ", lastL:" << nLastLine << "\n";
    }
}

void CDiffDoc::saveClrSetting(const char *pStrSettingName, const QColor& clr)
{
    QSettings settings(ORG_NAME, APP_NAME);

    settings.beginGroup("colours");
    settings.setValue(pStrSettingName, clr.rgba());
    settings.endGroup();
}

void CDiffDoc::loadClrSettings()
{
    QSettings settings(ORG_NAME, APP_NAME);

    settings.beginGroup("colours");
    m_clrIdentical.setRgba(settings.value("identical", CLR_DEFAULT_IDENTICAL.rgba()).toUInt());
    settings.endGroup();
}

void CDiffDoc::loadFolderExceptions()
{
    QSettings settings(ORG_NAME, APP_NAME);

    m_bExceptionStringsEnabled = settings.value("folderExceptionsEnabled", true).toBool();

    QStringList defaultList;
    defaultList << "/Debug/" << "/Release/" << "/WinRel/" << "/WinDebug/";
    defaultList << ".dsw" << ".ncb" << ".opt" << ".aps" << ".clw" << ".plg" << ".dsp";

    m_listExceptions = settings.value("folderExceptions", defaultList).toStringList();

    m_bFoldersShowSame = settings.value("foldersShowSame", false).toBool();
}

void CDiffDoc::saveFolderExceptions(const QStringList& list)
{
    QSettings settings(ORG_NAME, APP_NAME);
    settings.setValue("folderExceptions", list);
    m_listExceptions = list;
}

void CDiffDoc::setFolderExceptionsEnabled(bool bEnabled)
{
    QSettings settings(ORG_NAME, APP_NAME);
    settings.setValue("folderExceptionsEnabled", bEnabled);
    m_bExceptionStringsEnabled = bEnabled;
}

void CDiffDoc::setFoldersShowSame(bool bShow)
{
    QSettings settings(ORG_NAME, APP_NAME);
    settings.setValue("foldersShowSame", bShow);
    m_bFoldersShowSame = bShow;
}

////////////////// CLineMap /////////////////////////

int CLineMap::GetLine(const char *pStrLine)
{
    std::map<std::string, int>::iterator it = m_map.find(pStrLine);
    if (it == m_map.end())
        return MI_NOTFOUND;

    return it->second;
}

void CLineMap::AddItem(const char *pStrLine, int nLine)
{
    std::map<std::string, int>::iterator it = m_map.find(pStrLine);
    if (it == m_map.end())
        m_map[pStrLine] = nLine;
    else
        m_map[pStrLine] = MI_NOTUNIQUE;
}

void CLineMap::MakeMap(CSection& section)
{
    m_map.clear();

    line_array* pLines = section.m_pLines;

    for (int nLine = section.m_nFirstLine; nLine <= section.m_nLastLine; nLine++)
    {
        CLine& line = pLines->at(nLine);

        //only add if line is not matched yet
        if (line.m_nLink == -1)
            AddItem(line.m_strLine.c_str(), nLine);
    }
}

void CLineMap::DebugMap()
{
    std::map<std::string, int>::iterator it = m_map.begin();
    for ( ; it != m_map.end(); ++it) {
        qDebug() << it->second << " " << it->first.c_str();
    }
}


