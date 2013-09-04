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

#ifndef DIFFDOC_H
#define DIFFDOC_H

#include <string>
#include <vector>
#include <map>
#include <QColor>


#define STATE_SAME			1
#define STATE_LEFTONLY		2
#define STATE_RIGHTONLY		4
#define STATE_SELECTED		8

//MAP ITEM defines
#define MI_NOTUNIQUE		-1   //ie has more than one entry
#define MI_NOTFOUND			-2

#define VIEW_LEFT           1
#define VIEW_RIGHT          2


class CLine
{
public:
    std::string m_strLine;
    int m_nLink;

    CLine() {m_strLine=""; m_nLink = -1;}

    CLine(const CLine& l) {m_strLine=l.m_strLine; m_nLink=l.m_nLink;}

    const CLine& operator =(const CLine& l)
    {
        m_strLine  = l.m_strLine;
        m_nLink = l.m_nLink;
        return *this;
    }
};

typedef std::vector<CLine> line_array;


class CSection
{
public:
    line_array* m_pLines; //this should be a member in CSectionList, which should be a custom CArray class

    int m_nFirstLine;
    int m_nLastLine;

    int m_nLink;
    int m_nCorrespond;

    int m_nState;


    CSection() {m_pLines=NULL;m_nFirstLine=m_nLastLine=m_nLink=m_nCorrespond=-1; m_nState=0;}

    CSection(int nFirst, int nLast, line_array* pLines)
    {
        m_pLines = pLines;
        m_nFirstLine = nFirst;
        m_nLastLine = nLast;

        m_nLink=m_nCorrespond=-1; m_nState=0;
    }

    const CSection& operator =(const CSection& s)
    {
        m_pLines = s.m_pLines;
        m_nFirstLine = s.m_nFirstLine;
        m_nLastLine = s.m_nLastLine;
        m_nLink = s.m_nLink;
        m_nCorrespond = s.m_nCorrespond;
        m_nState = s.m_nState;
        return *this;
    }
};

typedef std::vector<CSection> section_list;


class CLineMap
{
public:
    void AddItem(const char *pStrLine, int nLine);

    int GetLine(const char *pStrLine);
    void MakeMap(CSection& section);
    void DebugMap();

private:
    std::map<std::string, int> m_map;  //key=lineString; value=lineNumber
};


class CDiffDoc
{
private:
    line_array m_lines1;
    line_array m_lines2;
    section_list m_secs1, m_secs2, m_secsMerged;
    bool m_bIsCompared;

    bool LoadFile(const char *pStrFilePath, line_array& lines);
    bool LineLink(int nLine1, int nLine2);
    bool ExpandAnchor(int nLine1, int nLine2);
    bool SectionMatch(CSection& section1, CSection& section2);
    int FindEndOfUnmatched(line_array& lines, int nStart);
    int FindEndOfMatched(line_array& lines, int nStart);
    void MakeSectionList(section_list& secs1, bool bLeft, line_array& lines);
    bool MatchSectionLists(section_list& secs1, section_list& secs2);
    void AutoSelectSections();
    void SetFinalSectionLists(section_list& secs1, section_list& secs2);

    void DebugSectionList(section_list& secs);

    void saveClrSetting(const char *pStrSettingName, const QColor& clr);
    void loadClrSettings();
    void loadFolderExceptions();

    //view options -
    bool m_bShowSelections;
    bool m_bAutoSelect;
    std::string m_strMergePath;
    bool m_bFoldersShowSame;
    bool m_bExceptionStringsEnabled;
    int m_nBigLine1, m_nBigLine2;
    QColor m_clrIdentical, m_clrDifferent, m_clrOnlyLeft, m_clrOnlyRight;
    QStringList m_listExceptions;

public:
    CDiffDoc();
    virtual ~CDiffDoc();

    //main compare interface:
    bool IsCompared() {return m_bIsCompared;}
    bool LoadFiles(const char *pStrFilePath1, const char *pStrFilePath2);
    void Compare();

    line_array& GetLines(int nView) {if (nView==1) return m_lines1;return m_lines2;}
    section_list& GetSecs(int nView) { return (nView==1) ? m_secs1 : m_secs2;}

    //Colours
    QColor getClrIdentical() { return m_clrIdentical.isValid() ? m_clrIdentical : CLR_DEFAULT_IDENTICAL; }
    QColor getClrDifferent() { return m_clrDifferent.isValid() ? m_clrDifferent : CLR_DEFAULT_DIFFERENT; }
    QColor getClrOnlyLeft() { return m_clrOnlyLeft.isValid() ? m_clrOnlyLeft : CLR_DEFAULT_ONLYLEFT; }
    QColor getClrOnlyRight() { return m_clrOnlyRight.isValid() ? m_clrOnlyRight : CLR_DEFAULT_ONLYRIGHT; }
    void setClrIdentical(const QColor& clr) { m_clrIdentical = clr; saveClrSetting("identical", clr); }
    void setClrDifferent(const QColor& clr) { m_clrDifferent = clr; }
    void setClrOnlyLeft(const QColor& clr) { m_clrOnlyLeft = clr; }
    void setClrOnlyRight(const QColor& clr) { m_clrOnlyRight = clr; }

    static const QColor CLR_DEFAULT_IDENTICAL;
    static const QColor CLR_DEFAULT_DIFFERENT;
    static const QColor CLR_DEFAULT_ONLYLEFT;
    static const QColor CLR_DEFAULT_ONLYRIGHT;

    //folder exceptions
    QStringList getFolderExceptions() { return m_listExceptions; }
    void saveFolderExceptions(const QStringList& list);
    bool getFolderExceptionsEnabled() { return m_bExceptionStringsEnabled; }
    void setFolderExceptionsEnabled(bool bEnabled);
    bool getFoldersShowSame() { return m_bFoldersShowSame; }
    void setFoldersShowSame(bool bShow);

};

#endif // DIFFDOC_H
