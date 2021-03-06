//////////////////////////////////////////////////////////////////////
//
// Instruments.h: interface for the CInstruments class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_INSTRUMENTS_H__B8841629_94B5_4D97_91F5_056AC9639AB4__INCLUDED_)
#define AFX_INSTRUMENTS_H__B8841629_94B5_4D97_91F5_056AC9639AB4__INCLUDED_

#include "LogFileHelper.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

typedef unsigned short u_short;
typedef unsigned char u_char;

class CInstrument
{
private:
	u_short m_InputReg[400];		//输入寄存器
	u_short m_HoldReg[400];			//保持寄存器
	u_short m_History[PIXEL_FLAG];	//历史记录数据
	u_short m_ScanData[PIXEL_FLAG]; //扫描数据

	CRITICAL_SECTION m_cs;
	HWND m_hWnd;

	CLogFileHelper log;	 
public:
	CInstrument();
	~CInstrument();
	inline void SetWindowHandle(HWND hWnd) { m_hWnd = hWnd; } //保持窗口句柄

private:
	inline void LockData() { EnterCriticalSection(&m_cs); }		//锁存关键段
	inline void UnlockData() { LeaveCriticalSection(&m_cs); } //解锁关键段

public:
	bool GetSingleReg(u_short Adr, u_char *Val);
	u_short GetSingleReg(u_short Adr) ;
	bool GetSingleReg(u_short Adr, unsigned short *Val);
	bool SetSingleReg(u_short Adr, u_char *Val);
	bool SetSingleReg(u_short Adr, u_short Val) ;
	bool GetMultReg(u_short Adr, byte Len, u_char *aVal);
	bool SetMultReg(u_short Adr, byte Len, u_char *aVal);

	inline bool IsValidAddressOfGet(u_short Adr, u_short Len); //判断读取地址是否合法
	inline bool IsValidAddressOfSet(u_short Adr, u_short Len); //判断写入地址是否合法

	//----------------Instrumnent Control-----------------------------------------
public:
	unsigned short m_nCurParaNo;
	unsigned short m_nCurProg;
	unsigned short m_nScaning;
	unsigned short m_nCurPowerNO;
	int nInitCount;

	unsigned int InitialCCD(void);
	unsigned int Scanning(void);

	static DWORD WINAPI InitialCCD_Thread(void *pParam);
	static DWORD WINAPI ScanSample_Thread(void *pParam);

	unsigned int Cooler(bool bSwitch);
	unsigned int GetTempValue(int *tempature);
	inline unsigned int GetCCDStatus() { return m_InputReg[13]; }

	void OnTimer_1s();
	void Halt_byFault();
};

#endif // !defined(AFX_INSTRUMENTS_H__B8841629_94B5_4D97_91F5_056AC9639AB4__INCLUDED_)
