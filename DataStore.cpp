#include "StdAfx.h"
#include "DataStore.h"
#include "CsvFileHelper.h"


IMPLEMENT_SERIAL(CDataStore, CObject, 1)

//-----------------------------------------------------------------------
CDataStore::CDataStore(void)
{
	for(int i = 0 ; i < 10000 ; i++)	m_InputReg[i] = 0;
	for(int i = 0 ; i < 400 ; i++)		m_HoldReg[i] = 0;
	m_HoldReg[1] = 1 ;	//开启PC104电源
	pCCD = NULL ;

	InitializeCriticalSection(&m_cs);
}


//-----------------------------------------------------------------------
CDataStore::~CDataStore(void)
{
	DeleteCriticalSection(&m_cs);
}

//-----------------------------------------------------------------------
//初始化
//-----------------------------------------------------------------------
void CDataStore::Init(void)
{
	for(int i = 0 ; i < 10000 ; i++)	m_InputReg[i] = 0;
	for(int i =  0 ; i < 5 ; i++)		m_HoldReg[i] = 0 ;
	m_HoldReg[1] = 1 ;
	pCCD = NULL ;

}

//-----------------------------------------------------------------------
//序列化
//-----------------------------------------------------------------------
void CDataStore::Serialize(CArchive& ar )
{    
	CObject::Serialize(ar);	//关键代码    
	if(ar.IsStoring()) 
	{		//序列化 
		for(int i = 0 ; i < 400 ; i++)
			ar<<m_HoldReg[i] ;
	} 
	else 
	{		//反序列化        
		for(int i = 0 ; i < 400 ; i++)
			ar>>m_HoldReg[i] ;	
	}
}

//-----------------------------------------------------------------------
// 读取单个寄存器中的值
//-----------------------------------------------------------------------
short CDataStore::GetRegVal(unsigned short uAdr)
{
	if ( uAdr >= INPUTADR_L && uAdr < INPUTADR_H )
		return m_InputReg[uAdr - INPUTADR_L] ;

	if ( uAdr >= HOLDADR_L && uAdr < HOLDADR_H )
		return m_HoldReg[uAdr - HOLDADR_L] ;

	return 0 ;
}

//-----------------------------------------------------------------------
//读取多个寄存器的值
//-----------------------------------------------------------------------
void CDataStore::GetMRegVal(unsigned short uAdr,int nLen, short* pData) 
{
	short* pReg = NULL ;

	if ( uAdr >= INPUTADR_L && (uAdr + nLen) < INPUTADR_H )
		pReg = m_InputReg + uAdr - INPUTADR_L ;

	if ( uAdr >= HOLDADR_L && (uAdr + nLen) < HOLDADR_H )
		pReg = m_HoldReg + uAdr - HOLDADR_L ;

	while(pReg && nLen--)
			*pData++ = *pReg++ ;
}


//-----------------------------------------------------------------------
//读取当寄存器的值，并按字符返回
//-----------------------------------------------------------------------
bool CDataStore::GetRegByChar(unsigned short uAdr,unsigned char* Val) 
{
	short t = 0 ;
	bool b = false ;
	if ( uAdr >= INPUTADR_L && uAdr < INPUTADR_H )
	{
		t = m_InputReg[uAdr - INPUTADR_L] ;
		b = true ;
	}
	else 
		if ( uAdr >= HOLDADR_L && uAdr < HOLDADR_H )
		{
			t = m_HoldReg[uAdr - HOLDADR_L] ;
			b = true ;
		}

	Val[0] = t >> 8 ;
	Val[1] = t & 0x00FF ;
	return b ;
}

//-----------------------------------------------------------------------
//读取多个寄存器的值，按字节返回
//-----------------------------------------------------------------------
bool CDataStore::GetMRegByChar(unsigned short uAdr,int nLen, unsigned char* Val) 
{
	unsigned char *pVal ;
	short *pReg ;
	bool bFound = false ;

	pVal = Val ;
	if ( uAdr >= INPUTADR_L && (uAdr + nLen) <= INPUTADR_H )
	{
		pReg = m_InputReg + uAdr - INPUTADR_L ;
		bFound = true ;
	}

	if ( uAdr >= HOLDADR_L && (uAdr + nLen) <= HOLDADR_H )
	{
		pReg = m_HoldReg + uAdr - HOLDADR_L;
		bFound = true ;
	}


	if ( bFound )
	{
		while(nLen--)
		{
			*pVal++ = *pReg >> 8 ;
			*pVal++ = *pReg++ & 0x00FF ;
		}
	}

	return bFound ;
}


//-----------------------------------------------------------------------
//设定单个寄存器的值
//-----------------------------------------------------------------------
void CDataStore::SetRegVal(unsigned short uAdr, short Val)
{
	Lock() ;
	if ( uAdr >= INPUTADR_L && uAdr < INPUTADR_H )
		m_InputReg[uAdr - INPUTADR_L] = Val;

	if ( uAdr >= HOLDADR_L && uAdr < HOLDADR_H )
		m_HoldReg[uAdr - HOLDADR_L] = Val;

	Unlock() ;

	On_RegChange(uAdr,1) ;
}


//-----------------------------------------------------------------------
//设定多个寄存器的值
//-----------------------------------------------------------------------
void CDataStore::SetMRegVal(unsigned short uAdr, int nLen,short* Val)
{
	short* pVal ;

	Lock();
	if ( uAdr >= INPUTADR_L && (uAdr + nLen) <= INPUTADR_H )
	{
		pVal = m_InputReg + uAdr - INPUTADR_L ;
		while( nLen--)
			*pVal++ = *Val++ ;
	}

	if ( uAdr >= HOLDADR_L && (uAdr + nLen) <= HOLDADR_H )
	{
		pVal = m_HoldReg + uAdr - HOLDADR_L ;
		while( nLen--)
			*pVal++ = *Val++ ;
	}
	Unlock() ;

	On_RegChange(uAdr,nLen) ;
}


//-----------------------------------------------------------------------
//根据字节数据，设定多个寄存器的值
//-----------------------------------------------------------------------
void CDataStore::SetMRegByChar(unsigned short uAdr, int nLen, unsigned char* Val)
{
	short* pReg ;
	unsigned char* pVal ;
	int n = nLen ;
	pVal = Val ;


	if ( uAdr < HOLDADR_L || (uAdr + nLen) >= HOLDADR_H )
		return ;

	Lock() ;
	pReg = m_HoldReg + uAdr - HOLDADR_L ;
	while( n--)
	{
		*pReg++ = (pVal[0] << 8 | pVal[1]) ;
		pVal += 2;
	}
	Unlock() ;


	On_RegChange(uAdr,nLen) ;
}

//-----------------------------------------------------------------------
//根据字节数据，设定单个寄存器的值
//-----------------------------------------------------------------------
void CDataStore::SetRegByChar(unsigned short uAdr, unsigned char* Val) 
{
	Lock() ;
	if ( uAdr >= HOLDADR_L && uAdr <= HOLDADR_H )
		m_HoldReg[uAdr - HOLDADR_L] = (Val[0] << 8 | Val[1]) ;
	Unlock() ;

	On_RegChange(uAdr,1) ;
}


void CDataStore::On_RegChange(unsigned short uAdr, int nLen) 
{
	int nAdr = uAdr - HOLDADR_L ;
	int uEAdr = uAdr + nLen ;

	//读历史记录命令
	if ( uAdr <= RH_LOADREC && uEAdr >= RH_LOADREC )
		if ( m_HoldReg[17] == 1 )
		{
			ReadHistoryData(m_HoldReg[13], m_HoldReg[14], m_HoldReg[15], m_HoldReg[16]);
			m_HoldReg[17] = 0 ;
		}

	//设定系统时间
	if ( uAdr <= RH_DATESET && uEAdr >= RH_DATESET && m_HoldReg[19] == 1 )
	{
		DTimeSetting() ;
		m_HoldReg[19] = 0 ;
	}

	//开启拉曼仪器扫描
	if ( uAdr <= RH_SCANING && uEAdr >= RH_SCANING  && m_HoldReg[6] == 1  )
	{
		m_HoldReg[6] = 0 ;
		if (  m_InputReg[13] == 30 )	m_InputReg[13] = 10 ;
		pCCD->Scanning() ;
	}

	//CCD开关控制
	if ( uAdr >= RH_SW1 && uAdr <= RH_SW5)
	{
		if ( IsCCDOn() && ( m_HoldReg[0] == 0 || m_HoldReg[2] == 0 || m_HoldReg[4] == 0 ))
		{
			pCCD->Stop() ;
			pCCD->SetCCDStatus(0) ;
			pCCD->SetCoolStatus(0) ;
			m_InputReg[23] = 0 ;
		}
		if ( (!IsCCDOn() || m_InputReg[13] < 10) &&  m_HoldReg[0] == 1 && m_HoldReg[2] == 1 && m_HoldReg[4] == 1 )
		{
			m_InputReg[14] = m_HoldReg[105] ;
			pCCD->InitialCCD() ;
		}
	}

	//程序启动停止控制
	if ( uAdr <= RH_PROGSART && uEAdr >= RH_PROGSART )
		m_InputReg[24] = m_HoldReg[18]  ;

	//CCD温度控制
	if ( uAdr <= RH_CCDCOOLER && uEAdr >= RH_CCDCOOLER )
	{
		m_InputReg[14] = m_HoldReg[105] ;
		pCCD->Cooler((bool)m_HoldReg[5]) ;
	}
}


//-----------------------------------------------------------------------
//开关CCD电源
//-----------------------------------------------------------------------
void  CDataStore::SwitchCCD(bool bOn) 
{
	if ( bOn )
	{
		SetRegVal(RH_SW1,1) ;
		SetRegVal(RH_SW3,1) ;
		SetRegVal(RH_SW5,1) ;
	}
	else
	{
		SetRegVal(RH_SW1,0) ;
		SetRegVal(RH_SW3,0) ;
		SetRegVal(RH_SW5,0) ;
	}
}


//-----------------------------------------------------------------------
//开关激光器电源
//-----------------------------------------------------------------------
void  CDataStore::SwitchLaser(bool bOn) 
{
	if ( bOn )
		SetRegVal(RH_SW4,1) ;
	else
		SetRegVal(RH_SW4,0) ;
}
//-----------------------------------------------------------------------
//设定系统时间
//-----------------------------------------------------------------------
void  CDataStore::DTimeSetting(void) 
{
	SYSTEMTIME tm;
	tm.wYear = m_HoldReg[7];
	tm.wMonth = m_HoldReg[8];
	tm.wDay = m_HoldReg[9];
	tm.wHour = m_HoldReg[10];
	tm.wMinute = m_HoldReg[11];
	tm.wSecond = m_HoldReg[12];
	tm.wMilliseconds = 0;
	SetLocalTime(&tm);
}

//-----------------------------------------------------------------------
//更新系统时间
//-----------------------------------------------------------------------
void  CDataStore::DTimeGet(void) 
{
	SYSTEMTIME tm;
	GetLocalTime(&tm) ;

	m_InputReg[16] = tm.wYear ;
	m_InputReg[17] = tm.wMonth ;
	m_InputReg[18] = tm.wDay ;
	m_InputReg[19] = tm.wHour ;
	m_InputReg[20] = tm.wMinute ;
	m_InputReg[21]= tm.wSecond ;
}

//-----------------------------------------------------------------------
//检查是否到达预设的开机时间
//-----------------------------------------------------------------------
int  CDataStore::IsTimeForPower(int& nGrp, int& nScan) 
{
	for(int i = 0 ; i <= m_HoldReg[200] ; i++)
	{
		if ( m_InputReg[19] == m_HoldReg[201+i*5] &&
			 m_InputReg[20] == m_HoldReg[202+i*5] &&
			 m_InputReg[21] == m_HoldReg[203+i*5])
		{
			nGrp = m_HoldReg[204+i*5];
			if ( nGrp < 0 )		nGrp = 0 ;
			if ( nGrp > 19 )	nGrp = 19 ;
			nScan = m_HoldReg[205+i*5];
			if ( nScan < 0 )	nScan = 0 ;
			if ( nScan > 100 )	nScan = 100 ;
			return i ;
		}
	}

	return -1 ;
}

//--------------------------------------------------------------------
// Function:  Read Spectrum History data
// return true when success
// return false when failure
//--------------------------------------------------------------------
bool CDataStore::ReadHistoryData(u_short year, u_short month, u_short day, u_short index)
{
	CFileFind fFind;
	char buf[255];
	bool bRes = false;

	sprintf_s(buf, DATAPATYMD, year, month, day);
	int bFind = fFind.FindFile(buf);
	bool bHaveFile = false;
	u_short nIndex = 0;
	while (bFind)
	{
		bFind = fFind.FindNextFile();
		if (nIndex == index)
		{
			bHaveFile = true;
			break;
		}
		nIndex++;
	}

	if (bHaveFile)
	{
		CString sFName = fFind.GetFileName();
		if (sFName.GetLength() >= 11)
		{
			int len;
			CCsvFileHelper::ReadCsvFile(sFName, (unsigned short*)&m_InputReg[4000], &len);

			CString sTime = sFName.Right(10).Left(6);
			m_InputReg[4000 + PIXEL] = atoi(sTime.Left(2));
			m_InputReg[4001 + PIXEL] = atoi(sTime.Left(4).Right(2));
			m_InputReg[4002 + PIXEL] = atoi(sTime.Right(2));
		}
	}
	else
	{
		m_InputReg[4000 + PIXEL] = 0x00AA;
		m_InputReg[4001 + PIXEL] = 0X00BB;
		m_InputReg[4002 + PIXEL] = 0X00CC;
	}

	fFind.Close();
	return bRes;
}


