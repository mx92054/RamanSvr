#include "StdAfx.h"
#include "Newton.h"
#include "CsvFileHelper.h"
#include "Instruments.h"


CNewton::CNewton(void)
{
	nStatus = 0 ;
	nError = 0 ;
	bValidData = false ;

	nCCDSetting[0] = 2 ;
	nCCDSetting[1] = 0 ;
	nCCDSetting[2] = 100 ;
	nCCDSetting[3] = 1 ;
	nCCDSetting[4] = 200 ;
	nCCDSetting[5] = 160 ;

	CoolSwitch = 0 ;
	CoolStatus = 0 ;
	CoolTemp = 0 ;
	CoolError = 0 ;
	for(int i = 0 ; i < PIXEL ; i++)
		pixel_data[i] = rand() ;

	hThd_Init = 0 ;
	hThd_Scan = 0 ;

	SYSTEMTIME sysTime;
	ZeroMemory(&sysTime, sizeof(sysTime));
	GetLocalTime(&sysTime);
	pixel_data[PIXEL] = sysTime.wHour;
	pixel_data[PIXEL + 1] = sysTime.wMinute;
	pixel_data[PIXEL + 2] = sysTime.wSecond;
}


CNewton::~CNewton(void)
{
	DWORD dwExitCode = 0 ;
	GetExitCodeThread(hThd_Init,&dwExitCode) ;
	if ( dwExitCode == STILL_ACTIVE )
	{
		TerminateThread(hThd_Init,0) ;
	}
	
	GetExitCodeThread(hThd_Scan,&dwExitCode) ;
	if ( dwExitCode == STILL_ACTIVE )
	{
		TerminateThread(hThd_Init,0) ;
	}
}

//-----------------------------------------------------------------------------------------
unsigned int CNewton::InitialCCD(void)
{
	nStatus = 0 ;
	CoolStatus = 0 ;

	DWORD dwExitCode = 0 ;
	GetExitCodeThread(hThd_Init,&dwExitCode) ;
	if ( dwExitCode == STILL_ACTIVE )
	{
		//MessageBox(NULL,"CCD initialize still alive!","Alarm",MB_OK) ;
		return 1;
	}
	
	DWORD dwThd_Init ;
	hThd_Init = CreateThread(NULL, 0, Init_Thread, this, 0, &dwThd_Init);

	return 0;
}

//-----------------------------------------------------------------------------------------
unsigned int CNewton::Scanning(void)
{
	DWORD dwExitCode = 0 ;
	GetExitCodeThread(hThd_Scan,&dwExitCode) ;
	if ( dwExitCode == STILL_ACTIVE )
	{
		//MessageBox(NULL,"CCD scaning still alive!","Alarm",MB_OK) ;
		return 1;
	}

	DWORD dwThd_Scan ;
	hThd_Scan = CreateThread(NULL, 0, Scan_Thread, this, 0, &dwThd_Scan);

	return 0;
}

//-----------------------------------------------------------------------------------------
unsigned int CNewton::Cooler(bool bSwitch)
{
	CoolSwitch = (unsigned short)bSwitch;

	if (nStatus < 10)
		return 1;

	if (bSwitch)
	{
		CoolError = SetTemperature(nCCDSetting[5] - 200);
		if (CoolError != DRV_SUCCESS)
			return CoolError ;

		CoolError = CoolerON();
		if (CoolError != DRV_SUCCESS)
			return CoolError ;
		CoolStatus = 1;
	}
	else
	{
		CoolError = CoolerOFF();
		if (CoolError != DRV_SUCCESS)
			return CoolError ;
		CoolStatus = 3;
	}

	return 0;
}

//-----------------------------------------------------------------------------------------
unsigned int CNewton::GetTempValue(int *tempature)
{
	*tempature = 0 ;

	if (CoolStatus == 0 )
		return 1;

	int temp;
	CoolError = GetTemperature(&temp);
	if (CoolError == DRV_TEMP_STABILIZED)
	{
		CoolStatus = 2;
	}
	if (CoolError != DRV_NOT_INITIALIZED && nError != DRV_ERROR_ACK)
	{
		*tempature = temp;
		CoolTemp = temp + 200;
	}

	return 0;
}

void CNewton::TransSetting(short para[5]) 
{
	for(int i=0 ; i < 6 ; i++)
		nCCDSetting[i] = para[i] ;
}


//-----------------------------------------------------------------------------------------
DWORD WINAPI CNewton::Init_Thread(void *pObj)
{
	CNewton *pccd = (CNewton*)pObj ;

	unsigned int& hr = pccd->nError ;
	int& status = pccd->nStatus;

	char aBuffer[256];
	float speed;
	int index, iSpeed, iAD;
	int  nAD;

	int VSnumber; //垂直迁移速度对应的号码
	float VSpeed; //垂直迁移速度
	int HSnumber; //水平迁移速度对应的号码
	float HSpeed; //水平迁移速度
	int ADnumber;

	AndorCapabilities caps;
	char model[32];
	int gblXPixels; //光谱数据长度
	int gblYPixels; //光谱数据宽度

	//delay 5s
	status = 1 ;
	Sleep(5000) ;

	GetCurrentDirectory(256, aBuffer);					// Look in current working directory
														// for driver files

	hr = Initialize(aBuffer); // Initialize driver in current directory
	if ( hr != DRV_SUCCESS )
		return 1 ;

	// Get camera capabilities
	status = 2 ;
	caps.ulSize = sizeof(AndorCapabilities);
	hr = GetCapabilities(&caps);
	if (hr != DRV_SUCCESS)
		return 2;

	// Get Head Model
	status = 3 ;
	hr = GetHeadModel(model);
	if (hr != DRV_SUCCESS)
		return 3;

	// Get detector information
	status = 4 ;
	hr = GetDetector(&gblXPixels, &gblYPixels);
	if (hr != DRV_SUCCESS)
		return 4;

	// Set acquisition mode to required setting specified in xxxxWndw.c
	status = 5 ;
	hr = SetAcquisitionMode(pccd->nCCDSetting[0]);
	if (hr != DRV_SUCCESS)
		return 5;

	// Set read mode to required setting specified in xxxxWndw.c
	status = 6 ;
	hr = SetReadMode(pccd->nCCDSetting[1]);
	if (hr != DRV_SUCCESS)
		return 6;

	// Set Vertical speed to max
	VSpeed = 0;
	VSnumber = 0;
	status = 7 ;
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
	hr = SetVSSpeed(VSnumber);
	if (hr != DRV_SUCCESS)
		return 7;

	// Set Horizontal Speed to max
	HSpeed = 0;
	HSnumber = 0;
	ADnumber = 0;
	status = 8 ;
	hr = GetNumberADChannels(&nAD);
	if (hr != DRV_SUCCESS)
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

	status = 9 ;
	hr = SetADChannel(ADnumber);
	if (hr != DRV_SUCCESS)
		return 9;

	hr = SetHSSpeed(0, HSnumber);
	if (hr != DRV_SUCCESS)
		return 9;

	// Wait for 2 seconds to allow MCD to calibrate fully before allowing an
	// acquisition to begin
	Sleep(1000) ;

	status = 10 ;
	return 0;
}


//-----------------------------------------------------------------------------------------
DWORD WINAPI CNewton::Scan_Thread(void *pObj)
{
	CNewton *pccd = (CNewton*)pObj ;

	unsigned int& hr = pccd->nError ;
	int& status = pccd->nStatus ;

	pccd->bValidData = false;
	//Currently ready ?
	if (status != 10 && status != 30)
		return 10;

	status = 11;
	//set parameter
	// Set acquisition mode to required setting specified in xxxxWndw.c
	hr = SetAcquisitionMode(pccd->nCCDSetting[0]);
	if (hr != DRV_SUCCESS)
		return status;

	// Set read mode to required setting specified in xxxxWndw.c
	status = 12;
	hr = SetReadMode(pccd->nCCDSetting[1]);
	if (hr != DRV_SUCCESS)
		return status;

	// Set Exposure Time
	status = 13;
	float fExpoTime = pccd->nCCDSetting[2] / 100.0f;
	hr = SetExposureTime(fExpoTime);
	if (hr != DRV_SUCCESS)
		return status;

	// Set accums cycle time
	status = 14;
	float fAccExpoCycle = pccd->nCCDSetting[4] / 100.0f;
	hr = SetAccumulationCycleTime(fAccExpoCycle);
	if (hr != DRV_SUCCESS)
		return status;

	float exposure, accumulate, kinetic;

	status = 15;
	GetAcquisitionTimings(&exposure, &accumulate, &kinetic);

	hr = SetNumberAccumulations(pccd->nCCDSetting[3]);
	if (hr != DRV_SUCCESS)
		return status;

	// It is necessary to get the actual times as the system will calculate the
	// nearest possible time. eg if you set exposure time to be 0, the system
	// will use the closest value (around 0.01s)
	GetAcquisitionTimings(&exposure, &accumulate, &kinetic);

	// Set trigger mode
	//	pIns->m_InputReg[12] =SetBaselineClamp(1);
	//	if(pIns->m_InputReg[12] !=DRV_SUCCESS)
	//	  return 5 ;

	status = 16;
	hr = SetTriggerMode(0);
	if (hr != DRV_SUCCESS)
		return status;

	//	pIns->m_InputReg[12] =SetPreAmpGain(2);
	//	if(pIns->m_InputReg[12] != DRV_SUCCESS)
	//	  return 7;

	status = 17;
	// Starting the acquisition also starts a timer which checks the card status
	// When the acquisition is complete the data is read from the card and
	// displayed in the paint area.
	hr = StartAcquisition();
	if (hr != DRV_SUCCESS)
	{
		AbortAcquisition();
		return status;
	}

	status = 18;	
	int nTimeout = 10 + pccd->nCCDSetting[4] / 10 ;	//timeout time setting
	int CCDStatus ;

	while (true ||  nTimeout--)
	{
		Sleep(100);
		hr = GetStatus(&CCDStatus);
		if (CCDStatus == DRV_ACQUIRING)
			continue;
		if (CCDStatus == DRV_IDLE)
		{
			hr = GetAcquiredData16(pccd->pixel_data, PIXEL);

			SYSTEMTIME sysTime;
			ZeroMemory(&sysTime, sizeof(sysTime));
			GetLocalTime(&sysTime);

			pccd->pixel_data[PIXEL] = sysTime.wHour;
			pccd->pixel_data[PIXEL + 1] = sysTime.wMinute;
			pccd->pixel_data[PIXEL + 2] = sysTime.wSecond;
			if (hr != DRV_SUCCESS)
			{
				status = 19;
				return status; 
			}
			else
				break;
		}
	}

	status = 20;
	if ( nTimeout > 0 )
	{
		if ( CCsvFileHelper::WriteCsvFile(pccd->pixel_data, PIXEL))
		{
			status = 30;
			pccd->bValidData = true;
			return status ;
		}
		status = 21;
		return status;
	}

	status = 22;
	return status;
}