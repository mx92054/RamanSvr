#pragma once
#include "SerialPort.h"

extern unsigned char TxPowerRd[8];
extern unsigned char TxPowerWr[19];

#define COMPORT 1
#define POW_INPUT_ADR	20001
#define POW_INPUT_LEN	20
#define POW_INPUT_FRAM	45
#define POW_HOLD_ADR	30001
#define POW_HOLD_LEN	5
#define POW_HOLD_FRAM	8

class CPowerBd
{
private:
	HWND hWnd ;
	bool bOpen;
	bool bWrOrRd ;

	unsigned char RxBuf[256];
	int  nCurPos ;

public:
	int nRxNo ;
	int nTxNo ;
	CSerialPort com ;
	short sSwitch[POW_HOLD_LEN] ;
	short sRegister[POW_INPUT_LEN] ;

public:
	CPowerBd(void);
	~CPowerBd(void);

	void SetCurWnd(HWND hwnd) ;
	bool Open(void);
	unsigned short GetCRC(unsigned char *CmmBuf, unsigned char Len);
	void ProduceWrFrame(void); 
	void Translate(void) ;
	bool RxOneChar(unsigned char c) ;
	bool Task(void) ;
};

