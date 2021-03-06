#pragma once
#include "ATMCD32D.H"

#define PIXEL		2048
#define PIXEL_FLAG	2051

class CNewton
{
private:
	int nStatus ;		//仪器状态
	unsigned int nError ;//仪器故障代号

	unsigned short nCCDSetting[6] ;
		//0 查询模式
		//1 读取模式
		//2 曝光时间
		//3 曝光次数
		//4 曝光总周期时间
		//5 冷却温度

	unsigned short CoolSwitch ;		//降温开关
	unsigned short CoolStatus;		//降温状态
	unsigned short CoolTemp ;		//实际温度
	unsigned int CoolError ;

private:
	HANDLE	hThd_Init ;
	HANDLE	hThd_Scan ;

public:
	unsigned short pixel_data[PIXEL_FLAG] ;	//采样数据
	bool bValidData;						//仪器数据有效性	

public:
	CNewton(void);
	~CNewton(void);

	unsigned short GetCCDStatus()	{	return nStatus;	}
	unsigned short GetCCDError()	{	return nError;	}
	unsigned short IsCCDDataValid()	{	return bValidData;	} 
	unsigned short GetCoolSwitch()	{	return CoolSwitch;	}
	unsigned short GetCoolStatus()	{	return CoolStatus;	}
	unsigned short GetCoolTemp()	{	return CoolTemp;	}
	unsigned int   GetCoolErr()		{	return CoolError;	}
	unsigned int   GetCoolSet()		{	return (nCCDSetting[5]);	}
	void TransSetting(short para[6]) ;


	inline void SetCCDStatus(int n)		{ nStatus = n ;		} 
	inline void SetCoolStatus(int n)	{ CoolStatus = n ;	}

	unsigned int InitialCCD(void);
	unsigned int Scanning(void);
	unsigned int Cooler(bool bSwitch);
	unsigned int GetTempValue(int *tempature);
	inline void Stop(void)	{ if ( nStatus >= 10) ShutDown();	}

	static DWORD WINAPI Init_Thread(void *pObj) ;
	static DWORD WINAPI Scan_Thread(void* pObj) ;
};

