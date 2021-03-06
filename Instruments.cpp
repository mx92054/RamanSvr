// Instruments.cpp: implementation of the CInstruments class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Instruments.h"
#include "ATMCD32D.H"
#include "CsvFileHelper.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

//--------------------------------------------------------------------
// Function:  initialize
// return None
//--------------------------------------------------------------------
CInstrument::CInstrument()
{
	for (int i = 0; i < 400; i++)
	{
		m_InputReg[i] = 0;
		m_HoldReg[i] = 0;
	}
	for (int j = 0; j < PIXEL_FLAG; j++)
	{
		m_History[j] = 0;
		m_ScanData[j] = 0;
	}

	InitializeCriticalSection(&m_cs);

	//andor camera
	m_nCurParaNo = 0;
	m_nCurProg = 0;
	nInitCount = 0;
}

//--------------------------------------------------------------------
// Function:  Destory
// return None
//--------------------------------------------------------------------
CInstrument::~CInstrument()
{
	DeleteCriticalSection(&m_cs);
}

//--------------------------------------------------------------------
// Function:  Get a register value
// return true when success
// return false when failure
//--------------------------------------------------------------------
bool CInstrument::GetSingleReg(u_short Adr, u_char *Val)
{
	bool bRes = false;

	if (IsValidAddressOfGet(Adr, 1))
	{
		if (Adr < 30000)
		{
			if (Adr < 24000)
			{
				Val[0] = (m_InputReg[Adr - 20000] & 0xFF00) >> 8;
				Val[1] = (m_InputReg[Adr - 20000] & 0x00FF);
			}
			else if (Adr >= 24000 && Adr < 27000)
			{
				Val[0] = (m_History[Adr - 24000] & 0xFF00) >> 8;
				Val[1] = (m_History[Adr - 24000] & 0x00FF);
			}
			else
			{
				Val[0] = (m_ScanData[Adr - 27000] & 0xFF00) >> 8;
				Val[1] = (m_ScanData[Adr - 27000] & 0x00FF);
			}
		}
		else
		{
			Val[0] = (m_HoldReg[Adr - 30000] & 0xFF00) >> 8;
			Val[1] = (m_HoldReg[Adr - 30000] & 0x00FF);
		}
		bRes = true;
	}
	return bRes;
}

bool CInstrument::GetSingleReg(u_short Adr, unsigned short *Val)
{
	bool bRes = false;

	if (IsValidAddressOfGet(Adr, 1))
	{
		if (Adr < 30000)
		{
			if (Adr < 20400)
			{
				*Val = m_InputReg[Adr - 20000];
			}
			else if (Adr >= 24000 && Adr < 27000)
			{
				*Val = m_History[Adr - 24000];
			}
			else
			{
				*Val = m_ScanData[Adr - 27000];
			}
		}
		else
			*Val = m_HoldReg[Adr - 30000];

		bRes = true;
	}
	return bRes;
}

	
u_short CInstrument::GetSingleReg(u_short Adr)
{
	if (IsValidAddressOfGet(Adr, 1))
	{
		if (Adr < 30000)
		{
			if (Adr < 20400)
			{
				return  m_InputReg[Adr - 20000];
			}
			else if (Adr >= 24000 && Adr < 27000)
			{
				return m_History[Adr - 24000];
			}
			else
			{
				return m_ScanData[Adr - 27000];
			}
		}
		else
			return m_HoldReg[Adr - 30000];
	}
	return 0;
}

//--------------------------------------------------------------------
// Function:  Set a register value
// return true when success
// return false when failure
//--------------------------------------------------------------------
bool CInstrument::SetSingleReg(u_short Adr, u_char *Val)
{
	if (IsValidAddressOfSet(Adr, 1))
	{
		LockData();
		m_HoldReg[Adr - 30000] = Val[0] << 8 | Val[1];
		UnlockData();
		return true;
	}
	return false;
}

//--------------------------------------------------------------------
// Function:  Set a register value
// return true when success
// return false when failure
//--------------------------------------------------------------------
bool CInstrument::SetSingleReg(u_short Adr, u_short Val) 
{
	int n = Adr - 30000 ;

	if (IsValidAddressOfSet(Adr, 1))
	{
		LockData();
		m_HoldReg[n] = Val;
		UnlockData();
		return true;
	}

	return false;
}

//--------------------------------------------------------------------
// Function:  Get multiple register value
// return true when success
// return false when failure
//--------------------------------------------------------------------
bool CInstrument::GetMultReg(u_short Adr, byte Len, u_char *aVal)
{
	bool bRes = false;

	if (IsValidAddressOfGet(Adr, Len))
	{
		LockData();
		if (Adr < 30000)
		{
			if (Adr < 24000)
			{
				for (int i = 0; i < Len; i++)
				{
					aVal[2 * i] = (m_InputReg[Adr - 20000 + i] & 0xFF00) >> 8;
					aVal[2 * i + 1] = (m_InputReg[Adr - 20000 + i] & 0x00FF);
				}
			}
			else if (Adr >= 24000 && Adr < 27000)
			{
				for (int i = 0; i < Len; i++)
				{
					aVal[2 * i] = (m_History[Adr - 24000 + i] & 0xFF00) >> 8;
					aVal[2 * i + 1] = (m_History[Adr - 24000 + i] & 0x00FF);
				}
			}
			else
			{
				for (int i = 0; i < Len; i++)
				{
					aVal[2 * i] = (m_ScanData[Adr - 27000 + i] & 0xFF00) >> 8;
					aVal[2 * i + 1] = (m_ScanData[Adr - 27000 + i] & 0x00FF);
				}
			}
		}
		else
			for (int i = 0; i < Len; i++)
			{
				aVal[2 * i] = (m_HoldReg[Adr - 30000 + i] & 0xFF00) >> 8;
				aVal[2 * i + 1] = (m_HoldReg[Adr - 30000 + i] & 0x00FF);
			}
		UnlockData();

		bRes = true;
	}

	return bRes;
}

//--------------------------------------------------------------------
// Function:  Set multiple register value
// return true when success
// return false when failure
//--------------------------------------------------------------------
bool CInstrument::SetMultReg(u_short Adr, byte Len, u_char *aVal)
{
	u_short val;
	u_short add;

	if (IsValidAddressOfSet(Adr, Len))
	{
		LockData();
		for (int i = 0; i < Len; i++)
		{
			add = Adr - 30000 + i;
			val = aVal[2 * i] << 8 | aVal[2 * i + 1];

			m_HoldReg[add] = val;

			if (add == 18)
			{
				m_InputReg[24] = val;
			}
		}
		UnlockData();

		if (m_HoldReg[0] == 0 || m_HoldReg[2] == 0 || m_HoldReg[4] == 0) //ccd power off
		{
			m_InputReg[11] = 0;
			m_InputReg[13] = 0;
		}

		if (m_HoldReg[0] == 1 && m_HoldReg[2] == 1 && m_HoldReg[4] == 1 && m_InputReg[13] == 0) //ccd power on
		{
			InitialCCD();
		}

		if (m_HoldReg[5] == 1 && (m_InputReg[11] == 0 || m_InputReg[11] == 3)) // cool on
			Cooler(true);
		if (m_HoldReg[5] == 0 && (m_InputReg[11] == 1 || m_InputReg[11] == 2)) // cool off
			Cooler(false);

		if (m_HoldReg[6] == 1 && (m_InputReg[13] == 2 || m_InputReg[13] == 6)) //start sample once
		{
			m_HoldReg[6] = 0;
			Scanning();
		}

		if (Adr == 30019 && m_HoldReg[19] == 1) //chang system time
		{
			SYSTEMTIME tm;
			tm.wYear = m_HoldReg[7];
			tm.wMonth = m_HoldReg[8];
			tm.wDay = m_HoldReg[9];
			tm.wHour = m_HoldReg[10];
			tm.wMinute = m_HoldReg[11];
			tm.wSecond = m_HoldReg[12];
			tm.wMilliseconds = 0;
			SetSystemTime(&tm);
			LockData();
			m_HoldReg[19] = 0;
			UnlockData();
		}
		return true;
	}

	return false;
}

//--------------------------------------------------------------------
// Function:  initialize andor camera
//
//
//------------------------------------------------------------------------
DWORD WINAPI CInstrument::InitialCCD_Thread(void *pParam)
{
	CInstrument *pIns = (CInstrument *)pParam;
	if (pIns->m_InputReg[13] > 0) //如果已经连接，则无需重新连接
		return 0;

	pIns->m_InputReg[13] = 1;

	char aBuffer[256];
	float speed;
	int index, iSpeed, iAD;
	int test, test2, nAD;

	int VSnumber; //垂直迁移速度对应的号码
	float VSpeed; //垂直迁移速度
	int HSnumber; //水平迁移速度对应的号码
	float HSpeed; //水平迁移速度
	int ADnumber;

	AndorCapabilities caps;
	char model[32];
	int gblXPixels; //光谱数据长度
	int gblYPixels; //光谱数据宽度

	//delay 2s
	test = GetTickCount();
	do
	{
		test2 = GetTickCount() - test;
	} while (test2 < 4000);

	GetCurrentDirectory(256, aBuffer);					// Look in current working directory
																							// for driver files
	pIns->m_InputReg[12] = Initialize(aBuffer); // Initialize driver in current directory
	if (pIns->m_InputReg[12] != DRV_SUCCESS)
		return 1;

	// Get camera capabilities
	caps.ulSize = sizeof(AndorCapabilities);
	pIns->m_InputReg[12] = GetCapabilities(&caps);
	if (pIns->m_InputReg[12] != DRV_SUCCESS)
		return 2;

	// Get Head Model
	pIns->m_InputReg[12] = GetHeadModel(model);
	if (pIns->m_InputReg[12] != DRV_SUCCESS)
		return 3;

	// Get detector information
	pIns->m_InputReg[12] = GetDetector(&gblXPixels, &gblYPixels);
	if (pIns->m_InputReg[12] != DRV_SUCCESS)
		return 4;

	// Set acquisition mode to required setting specified in xxxxWndw.c
	pIns->m_InputReg[12] = SetAcquisitionMode(pIns->m_HoldReg[100 + pIns->m_nCurParaNo * 6] + 1);
	if (pIns->m_InputReg[12] != DRV_SUCCESS)
		return 5;

	// Set read mode to required setting specified in xxxxWndw.c
	pIns->m_InputReg[12] = SetReadMode(pIns->m_HoldReg[101 + pIns->m_nCurParaNo * 6]);
	if (pIns->m_InputReg[12] != DRV_SUCCESS)
		return 6;

	// Set Vertical speed to max
	VSpeed = 0;
	VSnumber = 0;
	GetNumberVSSpeeds(&index);
	for (iSpeed = 0; iSpeed < index; iSpeed++)
	{
		GetVSSpeed(iSpeed, &speed);
		if (speed > VSpeed)
		{
			VSpeed = speed;
			VSnumber = iSpeed;
		}
	}
	pIns->m_InputReg[12] = SetVSSpeed(VSnumber);
	if (pIns->m_InputReg[12] != DRV_SUCCESS)
		return 7;

	// Set Horizontal Speed to max
	HSpeed = 0;
	HSnumber = 0;
	ADnumber = 0;
	pIns->m_InputReg[12] = GetNumberADChannels(&nAD);
	if (pIns->m_InputReg[12] != DRV_SUCCESS)
		return 8;

	for (iAD = 0; iAD < nAD; iAD++)
	{
		GetNumberHSSpeeds(iAD, 0, &index);
		for (iSpeed = 0; iSpeed < index; iSpeed++)
		{
			GetHSSpeed(iAD, 0, iSpeed, &speed);
			if (speed > HSpeed)
			{
				HSpeed = speed;
				HSnumber = iSpeed;
				ADnumber = iAD;
			}
		}
	}

	pIns->m_InputReg[12] = SetADChannel(ADnumber);
	if (pIns->m_InputReg[12] != DRV_SUCCESS)
		return 9;

	pIns->m_InputReg[12] = SetHSSpeed(0, HSnumber);
	if (pIns->m_InputReg[12] != DRV_SUCCESS)
		return 10;

	// Wait for 2 seconds to allow MCD to calibrate fully before allowing an
	// acquisition to begin
	test = GetTickCount();
	do
	{
		test2 = GetTickCount() - test;
	} while (test2 < 2000);

	pIns->m_InputReg[13] = 2;

	return 0;
}

unsigned int CInstrument::InitialCCD(void)
{

	if (m_HoldReg[0] == 0 || m_HoldReg[2] == 0 || m_HoldReg[4] == 0)
		return 1;

	DWORD dwThreadID;
	CreateThread(NULL, 0, InitialCCD_Thread, this, 0, &dwThreadID);

	return 0;
}

//--------------------------------------------------------------------
// Function:  initialize andor camera
//
//
//-------------------------------------------------------------------
unsigned int CInstrument::Cooler(bool bSwitch)
{
	m_HoldReg[5] = (unsigned short)bSwitch;

	if (m_InputReg[13] < 2)
		return 1;

	if (bSwitch)
	{
		m_InputReg[12] = SetTemperature(m_HoldReg[105 + m_nCurParaNo * 6] - 200);
		m_InputReg[14] = m_HoldReg[105 + m_nCurParaNo * 6];
		if (m_InputReg[12] != DRV_SUCCESS)
			return m_InputReg[12];

		m_InputReg[12] = CoolerON();
		if (m_InputReg[12] != DRV_SUCCESS)
			return m_InputReg[12];
		m_InputReg[11] = 1;
	}
	else
	{
		m_InputReg[12] = CoolerOFF();
		if (m_InputReg[12] != DRV_SUCCESS)
			return m_InputReg[12];
		m_InputReg[11] = 3;
	}

	return 0;
}
//--------------------------------------------------------------------
// Function:  initialize andor camera
//
//
//-------------------------------------------------------------------
unsigned int CInstrument::GetTempValue(int *tempature)
{
	if (m_InputReg[11] == 0)
		return 1;

	int temp;
	m_InputReg[12] = GetTemperature(&temp);
	if (m_InputReg[12] == DRV_TEMP_STABILIZED)
	{
		m_InputReg[11] = 2;
	}
	if (m_InputReg[12] != DRV_NOT_INITIALIZED && m_InputReg[12] != DRV_ERROR_ACK)
	{
		*tempature = temp;
		m_InputReg[10] = temp + 200;
	}
	return 0;
}

//--------------------------------------------------------------------
// Function:  scaning sample
//
//
//-------------------------------------------------------------------
DWORD WINAPI CInstrument::ScanSample_Thread(void *pParam)
{
	CInstrument *pIns = (CInstrument *)pParam;

	//Currently ready ?
	if (pIns->m_InputReg[13] != 2 && pIns->m_InputReg[13] != 6)
		return 1;

	pIns->m_InputReg[13] = 3;
	//set parameter
	// Set acquisition mode to required setting specified in xxxxWndw.c
	pIns->m_InputReg[12] = SetAcquisitionMode(pIns->m_HoldReg[100 + pIns->m_nCurParaNo * 6] + 1);
	if (pIns->m_InputReg[12] != DRV_SUCCESS)
		return 2;

	// Set read mode to required setting specified in xxxxWndw.c
	pIns->m_InputReg[12] = SetReadMode(pIns->m_HoldReg[101 + pIns->m_nCurParaNo * 6]);
	if (pIns->m_InputReg[12] != DRV_SUCCESS)
		return 3;

	// Set Exposure Time
	float fExpoTime = pIns->m_HoldReg[102 + pIns->m_nCurParaNo * 6] / 100.0f;
	pIns->m_InputReg[12] = SetExposureTime(fExpoTime);
	if (pIns->m_InputReg[12] != DRV_SUCCESS)
		return 3;

	// Set accums cycle time
	float fAccExpoCycle = pIns->m_HoldReg[104 + pIns->m_nCurParaNo * 6] / 100.0f;
	pIns->m_InputReg[12] = SetAccumulationCycleTime(fAccExpoCycle);
	if (pIns->m_InputReg[12] != DRV_SUCCESS)
		return 3;

	float exposure, accumulate, kinetic;

	GetAcquisitionTimings(&exposure, &accumulate, &kinetic);

	pIns->m_InputReg[12] = SetNumberAccumulations(pIns->m_HoldReg[103 + pIns->m_nCurParaNo * 6]);
	if (pIns->m_InputReg[12] != DRV_SUCCESS)
		return 4;

	// It is necessary to get the actual times as the system will calculate the
	// nearest possible time. eg if you set exposure time to be 0, the system
	// will use the closest value (around 0.01s)
	GetAcquisitionTimings(&exposure, &accumulate, &kinetic);

	// Set trigger mode
	//	pIns->m_InputReg[12] =SetBaselineClamp(1);
	//	if(pIns->m_InputReg[12] !=DRV_SUCCESS)
	//	  return 5 ;

	pIns->m_InputReg[12] = SetTriggerMode(0);
	if (pIns->m_InputReg[12] != DRV_SUCCESS)
		return 6;

	//	pIns->m_InputReg[12] =SetPreAmpGain(2);
	//	if(pIns->m_InputReg[12] != DRV_SUCCESS)
	//	  return 7;

	pIns->m_InputReg[13] = 4;
	pIns->m_InputReg[15] = 0;
	// Starting the acquisition also starts a timer which checks the card status
	// When the acquisition is complete the data is read from the card and
	// displayed in the paint area.
	pIns->m_InputReg[12] = StartAcquisition();
	if (pIns->m_InputReg[12] != DRV_SUCCESS)
	{
		AbortAcquisition();
		return 8;
	}

	pIns->m_InputReg[13] = 5;
	//	int nTimeout = 10 + pIns->m_HoldReg[102 + m_nCurParaNo*6] / 10 ;	//timeout time setting
	int nStatus;
	int nCode;

	while (true)
	{
		Sleep(100);
		nCode = GetStatus(&nStatus);
		if (nStatus == DRV_ACQUIRING)
			continue;
		if (nStatus == DRV_IDLE)
		{
			nCode = GetAcquiredData16(pIns->m_ScanData, PIXEL);

			pIns->m_ScanData[PIXEL] = pIns->m_InputReg[19];
			pIns->m_ScanData[PIXEL + 1] = pIns->m_InputReg[20];
			pIns->m_ScanData[PIXEL + 2] = pIns->m_InputReg[21];
			if (nCode != DRV_SUCCESS)
			{
				pIns->m_InputReg[13] = 2;
				return 9;
			}
			else
				break;
		}
	}

	CCsvFileHelper::WriteCsvFile(pIns->m_ScanData, PIXEL);
	pIns->m_InputReg[13] = 6;
	pIns->m_InputReg[15] = 1;

	return 0;
}

//--------------------------------------------------------------------
// Function:  control function
//
//
//-------------------------------------------------------------------
void CInstrument::OnTimer_1s()
{
	//*****************************************
	SYSTEMTIME tm;
	GetLocalTime(&tm);
	LockData();
	m_InputReg[16] = tm.wYear;
	m_InputReg[17] = tm.wMonth;
	m_InputReg[18] = tm.wDay;
	m_InputReg[19] = tm.wHour;
	m_InputReg[20] = tm.wMinute;
	m_InputReg[21] = tm.wSecond;
	UnlockData();

	char buf[100];
	sprintf(buf, "%02d:%02d:%02d,%01d%01d%01d%01d%01d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
					tm.wHour, tm.wMinute, tm.wSecond,
					m_InputReg[0], m_InputReg[1], m_InputReg[2], m_InputReg[3], m_InputReg[4],
					m_InputReg[5], m_InputReg[6], m_InputReg[7], m_InputReg[8], m_InputReg[9],
					m_InputReg[10], m_InputReg[11], m_InputReg[12], m_InputReg[13], m_InputReg[14]);
	log.AddLogRecord(tm.wYear, tm.wMonth, tm.wDay, buf);

	//save parameter per Hour


	//step1   trun on power
	for (int i = 0; i <= m_HoldReg[200]; i++)
	{
		if (tm.wHour == m_HoldReg[201 + 5 * i] &&
				tm.wMinute == m_HoldReg[202 + 5 * i] &&
				tm.wSecond == m_HoldReg[203 + 5 * i] &&
				m_HoldReg[18] == 1)
		{
			m_nScaning = 0;
			m_nCurPowerNO = m_HoldReg[205 + 5 * i];
			m_nCurParaNo = m_HoldReg[204 + 5 * i];
			if (m_InputReg[13] == 0)
			{
				unsigned char buf[] = {0, 1, 0, 1, 0, 1, 0, 1, 0, 1};
				SetMultReg(30000, 5, buf);
				m_InputReg[23] = 1;

				return;
			}
			else
			{
				m_InputReg[23] = 100;
				return;
			}
		}
	}

	// counter work?
	if (m_InputReg[23] != 0)
		m_InputReg[23]++;

	// delay 2s,and to initialize CCD
	if (m_InputReg[23] == 2)
	{
		InitialCCD();
		m_InputReg[23] = 100;
		nInitCount = 0;
		return;
	}

	// initialize do not success in 30s
	if (m_InputReg[23] >= 130 && m_InputReg[23] < 1000 && m_InputReg[13] != 2)
	{
		if (nInitCount++ >= 3)
		{
			Halt_byFault();
			return;
		}
		m_InputReg[13] = 0;
		InitialCCD();
		m_InputReg[23] = 100;
		return;
	}

	//cooler
	if (m_InputReg[23] > 100 && m_InputReg[23] < 1000 && m_InputReg[13] == 2)
	{
		Cooler(true);
		m_InputReg[23] = 1000;
		return;
	}

	//cool fault?
	if (m_InputReg[23] > 4000 && m_InputReg[23] < 5000 && m_InputReg[10] < m_HoldReg[105 + m_nCurParaNo * 6])
	{
		Halt_byFault();
		return;
	}

	//scaning
	if (m_InputReg[23] > 1005 &&
			m_InputReg[23] < 5000 &&
			(m_InputReg[10] <= m_HoldReg[105 + m_nCurParaNo * 6] || m_InputReg[11] == 2))
	{
		Scanning();
		m_InputReg[23] = 5000;
		return;
	}

	if (m_InputReg[23] > 5005 && m_InputReg[13] == 6 && m_nScaning < m_nCurPowerNO - 1)
	{
		m_nScaning++;
		m_InputReg[11] = 2;
		m_InputReg[23] = 1000;
		return;
	}

	//swich off cooler and laser
	if (m_InputReg[23] > 5005 && m_InputReg[23] < 8000 && m_InputReg[13] == 6)
	{
		LockData();
		m_HoldReg[3] = 0;
		UnlockData();

		Cooler(false);
		m_InputReg[23] = 8000;
		return;
	}

	//switch off  ccd
	if (m_InputReg[23] > 8005 && m_InputReg[10] > 190)
	{
		//ShutDown() ;
		Halt_byFault();
		return;
	}
}

//---------------------------------------------------------------------------------
void CInstrument::Halt_byFault()
{
	LockData();
	m_HoldReg[0] = 0;
	m_HoldReg[2] = 0;
	m_HoldReg[3] = 0;
	m_HoldReg[4] = 0;
	m_nCurParaNo = 0;
	UnlockData();
	m_InputReg[23] = 0;
	m_InputReg[13] = 0;
	m_nCurParaNo = 0;
}

//----------------------------------------------------------------------------------------------
inline bool CInstrument::IsValidAddressOfGet(u_short Adr, u_short Len)
{
	if ((Adr >= 20000 && ((Adr + Len) < 20400)) ||							 //status reg
			(Adr >= 24000 && ((Adr + Len) <= 24000 + PIXEL_FLAG)) || //real data
			(Adr >= 27000 && ((Adr + Len) <= 27000 + PIXEL_FLAG)) || //histrory data
			(Adr >= 30000) && ((Adr + Len) < 30401))								 //setting reg
		return true;
	else
		return false;
}

//----------------------------------------------------------------------------------------------
inline bool CInstrument::IsValidAddressOfSet(u_short Adr, u_short Len)
{
	if (Adr >= 30000 && (Adr + Len) < 30400)
		return true;
	else
		return false;
}


//----------------------------------------------------------------------------------------------
unsigned int CInstrument::Scanning(void)
{
	DWORD dwThreadID;
	CreateThread(NULL, 0, ScanSample_Thread, this, 0, &dwThreadID);
	return 0;
}





