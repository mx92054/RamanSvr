// Instruments.h: interface for the CInstruments class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_INSTRUMENTS_H__B8841629_94B5_4D97_91F5_056AC9639AB4__INCLUDED_)
#define AFX_INSTRUMENTS_H__B8841629_94B5_4D97_91F5_056AC9639AB4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define SERIALRECV_LEN	100
#define SERIALSEND_LEN	100
#define PIXEL_FLAG 1024
#define WM_MODBUSSVR  WM_USER+221


typedef unsigned short u_short ;
typedef unsigned char  u_char ;

class CInstrument
{
private:
	u_short m_InputReg[400] ;
	u_short m_HoldReg[400] ;
	u_short m_History[PIXEL_FLAG] ;
	u_short m_ScanData[PIXEL_FLAG];

	u_short m_iPower[10] ;
	u_short m_hPower[5] ;

	CRITICAL_SECTION  m_cs ;
	HWND		m_hWnd ;

	//CLogFileHelper log ;

public:
	CInstrument()
	{
		for(int i=0 ; i < 400 ; i++)
		{
			m_InputReg[i] = 0 ;
			m_HoldReg[i] = 0 ;
		}
		for(int j=0 ; j < PIXEL_FLAG ; j++)
		{
			m_History[j] = 0 ;
			m_ScanData[j] = 0 ;
		}

		InitializeCriticalSection(&m_cs) ;

		int k = LoadData() ;
		if ( k )
		{
			char buf[20] ;
			sprintf(buf,"Load data error!(code:%d)",k) ;
			MessageBox(NULL,buf,"Alarm",0) ;
		}

		m_HoldReg[0] = 0 ;				//set initialize state
		m_HoldReg[1] = 1 ;
		m_HoldReg[2] = 0 ;
		m_HoldReg[3] = 0 ;
		m_HoldReg[5] = 0 ;

		for(k = 0 ; k < 5 ; k++)
		{
			m_hPower[k] = m_HoldReg[k] ;
		}

		//serial function
		bStartFrame = false ;
		nCurPos = 0 ;
		nFrameLen = 0 ;
		nTimer = 0 ;

		//andor camera
		m_nCurParaNo = 0 ;
		m_nCurProg = 0 ;
		nInitCount = 0 ;
	}

	~CInstrument()
	{
		DeleteCriticalSection(&m_cs) ;
		if ( SaveData() )
			MessageBox(NULL,"Save data error!","Alarm",0) ;
	}

	inline void SetWindowHandle(	HWND hWnd)		{ m_hWnd = hWnd ;	}

private:
	inline void LockData()		{	EnterCriticalSection(&m_cs) ;	}
	inline void UnlockData()	{	LeaveCriticalSection(&m_cs) ;	}

	bool	SaveData() ;	//save current config data to file
	int     LoadData() ;	//load config data from file

	bool	ReadHistoryData(u_short year,u_short month, u_short day,u_short index) ;	//Read Spectrum History data

public:
	bool GetSingleReg(u_short Adr,u_char* Val) ;
	bool GetSingleReg(u_short Adr,unsigned short* Val) ;

	bool SetSingleReg(u_short Adr,u_char* Val) ;

	bool GetMultReg(u_short Adr,byte Len,u_char* aVal) ;
	bool SetMultReg(u_short Adr,byte Len,u_char* aVal) ;


	inline bool IsValidAddressOfGet(u_short Adr, u_short Len) 
	{
		if ( (Adr >= 20000 && ((Adr + Len) < 20400)) ||						//status reg
			 (Adr >= 24000 && ((Adr + Len) <= 24000 + PIXEL_FLAG)) ||		//real data
			 (Adr >= 27000 && ((Adr + Len) <= 27000 + PIXEL_FLAG)) ||		//histrory data
			 (Adr >= 30000 ) && ((Adr + Len) < 30401) )						//setting reg
			return true ;
		else
			return false ;
	}

	inline bool IsValidAddressOfSet(u_short Adr,u_short Len) 
	{
		if ( Adr >= 30000  && (Adr + Len) < 30400 )
			return true ;
		else
			return false ;
	}

//serial handle
private:
	unsigned char	serialBuf[SERIALRECV_LEN] ;
	unsigned char	sendFrame[SERIALRECV_LEN] ;
	bool			bStartFrame ;
	int				nCurPos ;
	int				nFrameLen ;
	int				nTimer ;
	int				nInitCount ;

	unsigned short	GetCRC(unsigned char* CmmBuf,unsigned char Len) ;
	void	HandleComm(unsigned char* buf,unsigned char Len) ;
	
public:
	void	RecvOneChar(unsigned char c) ;
	void	OnTimer_200ms() 
	{
		if ( bStartFrame )
		{
			if ( nTimer++ > 3 )
				bStartFrame = false ;
		}
	}

	unsigned char* ProductFrame(char nCmd,u_short nAdr,u_short nNum,int* len) ;


	//----------------Instrumnent Control-----------------------------------------
public:
	unsigned short  m_nCurParaNo ;
	unsigned short  m_nCurProg ;
	unsigned short  m_nScaning ;
	unsigned short  m_nCurPowerNO ;

	unsigned int	InitialCCD(void) ;
	unsigned int	Scanning(void)
	{
		DWORD dwThreadID ;
		CreateThread(NULL,0,ScanSample_Thread,this,0,&dwThreadID) ;

		return 0 ;
	}

	static DWORD WINAPI	InitialCCD_Thread(void* pParam) ;
	static DWORD WINAPI ScanSample_Thread(void* pParam) ;

	unsigned int	Cooler(bool bSwitch) ;
	unsigned int	GetTempValue(int* tempature) ;
	inline unsigned int		GetCCDStatus()		{	return m_InputReg[13] ;	} 
	inline unsigned short	GetProgramStatus()	{	return m_HoldReg[18] ;	}

	void	OnTimer_1s() ;
	void	Halt_byFault() ;
} ;

#endif // !defined(AFX_INSTRUMENTS_H__B8841629_94B5_4D97_91F5_056AC9639AB4__INCLUDED_)
