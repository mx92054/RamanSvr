#pragma once
#include "afx.h"
#include <atlstr.h>
#include "Newton.h"

using namespace std;

#define INPUTADR_L		20000
#define INPUTADR_H		30000
#define HOLDADR_L		30000
#define HOLDADR_H		30400

#define RI_SW1			20000
#define RI_SW2			20001
#define RI_SW3			20002
#define RI_SW4			20003
#define RI_SW5			20004

#define RI_CCDTEMPVAL	20010
#define RI_CCDTEMPSTAT	20011
#define RI_CCDERROR		20012
#define RI_CCDSTATUS	20013
#define RI_CCDTEMPSET	20014
#define RI_CCDISDATA	20015
#define RI_INSYEAR		20016
#define RI_INSMONTH		20017
#define RI_INSDAY		20018
#define RI_INSHOUR		20019
#define RI_INSMONTIUE	20020
#define RI_INSSECOND	20021
#define RI_RECNUM		20022
#define RI_PROGSTEP		20023
#define RI_PROGFLAG		20024

#define RI_SCANDATA		27000 


#define RH_SW1			30000
#define RH_SW2			30001
#define RH_SW3			30002
#define RH_SW4			30003
#define RH_SW5			30004
#define RH_CCDCOOLER	30005
#define RH_SCANING		30006
#define RH_LOADREC		30017
#define RH_PROGSART		30018
#define	RH_DATESET		30019

#define RH_ACQMODE1		30100
#define	RH_READMODE1	30101
#define RH_EXPTIME1		30102
#define RH_EXPNUM1		30103
#define RH_EXPCYL1		30104
#define RH_COOLTEMP1	30105

#define RH_BOOTNUM		30200
#define RH_BOOTHOUR1	30201
#define RH_BOOTMIN1		30202
#define RH_BOOTSEC1		30203
#define RH_BOOTPARA1	30204
#define RH_BOOTSCANNUM	30205

#define RH_MAXSCANUM 100

class CDataStore :	public CObject
{
private:
	short m_InputReg[10000] ;	//输入存储器
	short m_HoldReg[400] ;		//保持寄存器
	CNewton *pCCD ;

	CRITICAL_SECTION m_cs;

private:
	inline void Lock()		{ EnterCriticalSection(&m_cs);	}	//锁存关键段
	inline void Unlock()	{ LeaveCriticalSection(&m_cs);	}	//解锁关键段	
	bool ReadHistoryData(u_short year, u_short month, u_short day, u_short index);

public:
	DECLARE_SERIAL( CDataStore )
	void Serialize( CArchive& ar );

	CDataStore(void);
	~CDataStore(void);

	void Init(void);

public:
	short GetRegVal(unsigned short uAdr) ;
	void  GetMRegVal(unsigned short uAdr,int nLen, short* pData) ;

	bool GetRegByChar(unsigned short uAdr,unsigned char* Val) ;
	bool GetMRegByChar(unsigned short uAdr,int nLen, unsigned char* Val) ;

	//----------------------------------------------------------------------------
	void SetRegVal(unsigned short uAdr, short Val) ;
	void SetMRegVal(unsigned short uAdr, int nLen,short* Val) ;

	void SetRegByChar(unsigned short uAdr, unsigned char* Val) ;
	void SetMRegByChar(unsigned short uAdr, int nLen,unsigned char* Val) ;

	void  SwitchCCD(bool bOn) ;
	void  SwitchLaser(bool bOn) ;
	void  DTimeSetting(void) ;
	void  DTimeGet(void) ;
	int	  IsTimeForPower(int& nGrp, int& nScan) ;

public:
	inline short GetCCDStatus(void)		{	return m_InputReg[13];} 
	inline void  SetCCDStatus(short val){	SetRegVal(RI_CCDSTATUS, val) ;	}
	inline short GetCCDErr(void)		{	return m_InputReg[12];	} 
	inline void  SetCCDErr(short val)	{	SetRegVal(RI_CCDERROR, val) ;	}
	inline short GetProgStep(void)		{	return m_InputReg[23];	} 
	inline void  SetProgStep(short val)	{	SetRegVal(RI_PROGSTEP, val) ;	}	
	inline bool  IsCCDOn(void)			{	return ( m_InputReg[0] && m_InputReg[2] && m_InputReg[4] );	}
	inline bool  IsLaserOn(void)		{	return ( m_InputReg[3] != 0); }
	inline void  IncProgStep(void)		{   if (m_InputReg[23])   m_InputReg[23]++ ;	}
	inline void  GetNewton(CNewton* p)	{	pCCD = p ;	} 

private:
	void On_RegChange(unsigned short uAdr, int nLen) ;
};

