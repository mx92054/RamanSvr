// CsvFileHelper.h: interface for the CCsvFileHelper class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSVFILEHELPER_H__92BC73A4_059F_4C2B_906B_1F3ACFBD160F__INCLUDED_)
#define AFX_CSVFILEHELPER_H__92BC73A4_059F_4C2B_906B_1F3ACFBD160F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CCsvFileHelper  
{
public:
	CCsvFileHelper();
	virtual ~CCsvFileHelper();

	static bool WriteCsvFile(unsigned short* pData,int nLen) ;
	static bool ReadCsvFile(CString fName,unsigned short* pData,int* len) ;
	static bool WriteLogFile(CListBox* plst);
};

#endif // !defined(AFX_CSVFILEHELPER_H__92BC73A4_059F_4C2B_906B_1F3ACFBD160F__INCLUDED_)
