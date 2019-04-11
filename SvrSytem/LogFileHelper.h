// LogFileHelper.h: interface for the CLogFileHelper class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LOGFILEHELPER_H__3A74819F_DD0A_4AE9_B8F9_E37B1EF26446__INCLUDED_)
#define AFX_LOGFILEHELPER_H__3A74819F_DD0A_4AE9_B8F9_E37B1EF26446__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define BUFSIZE	300



typedef  unsigned int ushort;

class CLogFileHelper  
{
private:
	char	buf[BUFSIZE][100] ;
	ushort	nCur ;
	ushort  y,m,d ;
	bool	bOpen ;
	CStdioFile file ;

public:
	CLogFileHelper() ;
	virtual ~CLogFileHelper();

public:
	void	AddLogRecord(ushort nYear,ushort nMonth,ushort nDay,char* pBuf) ;

private:
	bool	WriteFile() ;
	bool	OpenFile() ;
	bool	CloseFile() ;
};

#endif // !defined(AFX_LOGFILEHELPER_H__3A74819F_DD0A_4AE9_B8F9_E37B1EF26446__INCLUDED_)
