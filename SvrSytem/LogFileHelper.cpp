// LogFileHelper.cpp: implementation of the CLogFileHelper class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LogFileHelper.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLogFileHelper::CLogFileHelper()
{
	SYSTEMTIME tm ;
	GetLocalTime(&tm) ;

	nCur = 0 ;	
	y = tm.wYear ; 
	m = tm.wMonth ; 
	d = tm.wDay ;
	bOpen = OpenFile() ;
}

CLogFileHelper::~CLogFileHelper()
{
	WriteFile() ;
	CloseFile() ;
}


//////////////////////////////////////////////////////////////////////
// functin
//////////////////////////////////////////////////////////////////////
void	CLogFileHelper::AddLogRecord(ushort nYear,ushort nMonth,ushort nDay,char* pBuf)
{
	if ( nYear == y && nMonth == m && nDay == d )
	{
		if ( nCur == 0 )
		{
			strcpy(buf[nCur++],pBuf) ;
		}
		else
		{
			if ( strcmp(pBuf+9,buf[nCur-1]+9) != 0 )
				strcpy(buf[nCur++],pBuf) ;
		}
		if ( nCur >= BUFSIZE )
		{
			WriteFile() ;
			nCur = 0 ;
		}
	}
	else
	{
		WriteFile() ;
		CloseFile() ;
		y = nYear ;
		m = nMonth ;
		d = nDay ;
		OpenFile() ;
		strcpy(buf[nCur++],pBuf) ;
	}
}

//----------------------------------------------------------------------------------------
bool	CLogFileHelper::OpenFile() 
{
	CString		sPath ;
	CString		sFilename ;
	sFilename.Format("%04d%02d%02d",y,m,d) ;
	sPath.Format(LOGFILEPATH,sFilename) ;
	
	if ( !file.Open(sPath,CFile::modeCreate | CFile::modeWrite |CFile::modeNoTruncate ) )
		return false ;

	file.Seek(0,CFile::end) ;
	return true ;
}

//----------------------------------------------------------------------------------------
bool	CLogFileHelper::WriteFile() 
{
	if ( !bOpen )
		return false ;

	for(ushort i = 0 ; i < nCur ; i++)
		file.WriteString(buf[i]) ;
	return true ;
}

//----------------------------------------------------------------------------------------
bool	CLogFileHelper::CloseFile() 
{
	file.Close() ;
	return true ;
}
