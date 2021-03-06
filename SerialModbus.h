#pragma once

typedef unsigned short u_short;
typedef unsigned char u_char;

class CSerialModbus
{
public:
	CSerialModbus(void);
	~CSerialModbus(void);

private:
	unsigned char serialBuf[SERIALRECV_LEN];
	unsigned char sendFrame[SERIALRECV_LEN];
	bool bStartFrame;
	int nCurPos;
	int nFrameLen;
	int nTimer;
	int nInitCount;
	u_short Reg[100] ;

	unsigned short GetCRC(unsigned char *CmmBuf, unsigned char Len);
	void HandleComm(unsigned char *buf, unsigned char Len,u_short* val);

public:
	bool RecvOneChar(unsigned char c);
	void OnTimer_200ms();
	unsigned char *ProductFrame(char nCmd, u_short nAdr, u_short nNum, u_short* val, int *len);
	void TransTo(u_short *val, int len) ;
	void TransFrom(u_short *val, int len) ;
};

