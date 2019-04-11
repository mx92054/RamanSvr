// CsvFileHelper.cpp: implementation of the CCsvFileHelper class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CsvFileHelper.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCsvFileHelper::CCsvFileHelper()
{

}

CCsvFileHelper::~CCsvFileHelper()
{

}

//----------------------------------------------------------------------------------------------------
bool CCsvFileHelper::WriteCsvFile(unsigned short* pData,int nLen) 
{
	CStdioFile file ;
	CTime		tTime = CTime::GetCurrentTime() ;
	CString		sPath ;
	sPath.Format(DATAFILEPATH,tTime.Format("%Y%m%d_%H%M%S")) ;
	
	if ( !file.Open(sPath,CFile::modeCreate | CFile::modeWrite) )
		return false ;

	CString str ;

	for(int i = 0 ; i < nLen ; i++)
	{
		str.Format("%d,%d\n",i+1,pData[i]) ;
		file.WriteString(str) ;
	}	

	file.Close() ;
	return true ;
}

//----------------------------------------------------------------------------------------------------
bool CCsvFileHelper::ReadCsvFile(CString fName,unsigned short* pData,int* len) 
{
	CStdioFile file ;
	CString	   sPath ;
	sPath.Format(DATAFILEPATH2,fName) ;

	if ( !file.Open(sPath,CFile::modeRead)  )
		return false ;

	int i = 0 ;
	int n = 0 ;
	CString sText,sItem ;
	while ( file.ReadString(sText) )
	{
		n = sText.Find(",") ;
		sItem = sText.Right(sText.GetLength() - sText.Find(",") - 1) ;
		pData[i] = atoi(sItem) ;
		i++ ;
		if ( i >= PIXEL )		//max 1024 numbers
			break ;
	}

	*len = i ;
	return true ;
}


//----------------------------------------------------------------------------------------------------
bool CCsvFileHelper::WriteLogFile(CListBox* plst)
{
	CStdioFile file ;
	CTime		tTime = CTime::GetCurrentTime() ;
	CString		sPath ;
	sPath.Format(RECFILEPATH,tTime.Format("%Y%m%d")) ;
	
	if ( !file.Open(sPath,CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite) )
		return false ;

	file.SeekToEnd() ;

	CString str ;
	int nCount = plst->GetCount() ;
	for(int i=nCount-1 ; i >= 0 ; i--)
	{
		plst->GetText(i,str) ;
		str += "\n" ;
		file.WriteString(str) ;
	}	

	file.Close() ;

	return true ;
}
