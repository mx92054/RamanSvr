// Instruments.cpp: implementation of the CInstruments class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Instruments.h"
//#include "CsvFileHelper.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//--------------------------------------------------------------------
// Function:  Get a register value 
// return true when success 
// return false when failure
//--------------------------------------------------------------------
bool CInstrument::GetSingleReg(u_short Adr,u_char* Val)
{
	bool bRes = false ;

	if ( IsValidAddressOfGet(Adr,1) )
	{
		if ( Adr < 30000 )
		{
			if ( Adr < 24000 )
			{
				Val[0] = (m_InputReg[Adr - 20000] & 0xFF00) >> 8 ;
				Val[1] = (m_InputReg[Adr - 20000] & 0x00FF) ;
			}
			else
				if ( Adr >= 24000 && Adr < 27000 )
				{
					Val[0] = (m_History[Adr - 24000] & 0xFF00) >> 8 ;
					Val[1] = (m_History[Adr - 24000] & 0x00FF) ;
				}
				else
				{
					Val[0] = (m_ScanData[Adr - 27000] & 0xFF00) >> 8 ;
					Val[1] = (m_ScanData[Adr - 27000] & 0x00FF) ;
				}
		}
		else
		{
			Val[0] = (m_HoldReg[Adr - 30000] & 0xFF00) >> 8 ;
			Val[1] = (m_HoldReg[Adr - 30000] & 0x00FF) ;
		}
		bRes = true ;
	}
	return bRes ;		
}

bool CInstrument::GetSingleReg(u_short Adr,unsigned short* Val)
{
	bool bRes = false ;

	if ( IsValidAddressOfGet(Adr,1) )
	{
		if ( Adr < 30000 )
		{
			if ( Adr < 20400 )
			{
				*Val = m_InputReg[Adr - 20000] ;
			}
			else
				if ( Adr >= 24000 && Adr < 27000 )
				{
					*Val = m_History[Adr - 24000] ;
				}
				else
				{
					*Val = m_ScanData[Adr - 27000] ;
				}
		}
		else
			*Val = m_HoldReg[Adr - 30000] ;

		bRes = true ;
	}
	return bRes ;		
}

//--------------------------------------------------------------------
// Function:  Set a register value 
// return true when success 
// return false when failure
//--------------------------------------------------------------------
bool CInstrument::SetSingleReg(u_short Adr,u_char* Val)
{
	if ( IsValidAddressOfSet(Adr,1) )
	{
		LockData() ;
		m_HoldReg[Adr - 30000] = Val[0] << 8 | Val[1] ;
		UnlockData() ;

		return true ;
	}

	return false ;		
}

//--------------------------------------------------------------------
// Function:  Get multiple register value 
// return true when success 
// return false when failure
//--------------------------------------------------------------------
bool CInstrument::GetMultReg(u_short Adr,byte Len,u_char* aVal)
{
	bool bRes = false ;

	if ( IsValidAddressOfGet(Adr,Len) )
	{
		LockData() ;
		if ( Adr < 30000 )
		{
			if ( Adr < 24000 )
			{
				for(int i = 0 ; i < Len ; i++)
				{
					aVal[2*i]	  = (m_InputReg[Adr - 20000 + i] & 0xFF00) >> 8 ;
					aVal[2*i + 1] = (m_InputReg[Adr - 20000 + i] & 0x00FF) ;
				}
			}
			else
				if ( Adr >= 24000 && Adr < 27000 )
				{
					for(int i = 0 ; i < Len ; i++)
					{
						aVal[2*i]	  = (m_History[Adr - 24000 + i] & 0xFF00) >> 8 ;
						aVal[2*i + 1] = (m_History[Adr - 24000 + i] & 0x00FF) ;
					}
				}
				else
				{
					for(int i = 0 ; i < Len ; i++)
					{
						aVal[2*i]	  = (m_ScanData[Adr - 27000 + i] & 0xFF00) >> 8 ;
						aVal[2*i + 1] = (m_ScanData[Adr - 27000 + i] & 0x00FF) ;
					}
				}
		}
		else
			for(int i = 0 ; i < Len ; i++)
			{
				aVal[2*i]	 = (m_HoldReg[Adr - 30000 + i] & 0xFF00) >> 8 ;
				aVal[2*i + 1] = (m_HoldReg[Adr - 30000 + i] & 0x00FF) ;
			}
		UnlockData() ;

		bRes = true ;
	}

	return bRes ;		
}

//--------------------------------------------------------------------
// Function:  Set multiple register value 
// return true when success 
// return false when failure
//--------------------------------------------------------------------
bool CInstrument::SetMultReg(u_short Adr,byte Len, u_char* aVal)
{
	u_short val ;
	u_short add ;

	if ( IsValidAddressOfSet(Adr,Len) )
	{
		LockData() ;
		for(int i = 0 ; i < Len ; i++)
		{
			add = Adr - 30000 + i ;
			val = aVal[2*i] << 8 | aVal[2*i + 1] ;

			if ( add == 17 && val == 1 )
				ReadHistoryData(m_HoldReg[13],m_HoldReg[14],m_HoldReg[15],m_HoldReg[16]) ;
			else
				m_HoldReg[add] = val ;

			if ( Adr < 30005 )
				m_hPower[add] =  m_HoldReg[add] ;

			if ( add == 18 )
			{
				m_InputReg[24] = val ;
			}
		}
		UnlockData() ;

		if ( m_HoldReg[0] == 0 || m_HoldReg[2] == 0 || m_HoldReg[4] == 0 )		//ccd power off
		{
			m_InputReg[11] = 0 ;
			m_InputReg[13] = 0 ;
		}

		if ( m_HoldReg[0] == 1 && m_HoldReg[2] == 1 && m_HoldReg[4] == 1 && m_InputReg[13] == 0 )  //ccd power on
		{
			InitialCCD() ;
		}

		if ( m_HoldReg[5] == 1 && ( m_InputReg[11] == 0 || m_InputReg[11] == 3 ) )			// cool on
			Cooler(true) ;
		if ( m_HoldReg[5] == 0 && ( m_InputReg[11] == 1 || m_InputReg[11] == 2 ) )			// cool off
			Cooler(false) ;

		if ( m_HoldReg[6] == 1 && ( m_InputReg[13] == 2 || m_InputReg[13] == 6 ) )			//start sample once
		{
			m_HoldReg[6] = 0 ;
			Scanning() ;
		}

		if ( Adr == 30019 && m_HoldReg[19] == 1 )											//chang system time
		{
			SYSTEMTIME tm ;
			tm.wYear = m_HoldReg[7] ;		tm.wMonth = m_HoldReg[8] ;		tm.wDay = m_HoldReg[9] ;
			tm.wHour = m_HoldReg[10] ;		tm.wMinute = m_HoldReg[11] ;	tm.wSecond = m_HoldReg[12] ;
			tm.wMilliseconds = 0 ;
			SetSystemTime(&tm) ;
			LockData() ;
			m_HoldReg[19] = 0 ;
			UnlockData() ;
		}

		return true ;
	}

	return false ;		
}

//--------------------------------------------------------------------
// Function:  Save figure data to file
// return true when success 
// return false when failure
//--------------------------------------------------------------------
bool CInstrument::SaveData()
{
	/*
	char buf[40] ;
	CStdioFile file ;
	if ( !file.Open(L"D:\\Developer\\RamanSvr\\Data\\Config.ini",CFile::modeWrite)) 
		return false ;

	file.SeekToBegin() ;
	for(int i = 0 ; i < 400 ; i++)
	{
		sprintf(buf,"Hold:%d  %d\n",i,m_HoldReg[i]) ;
		file.WriteString(buf) ;
	}

	for(int j = 0 ; j < 400 ; j++)
	{
		sprintf(buf,"Input:%d %d\n",j,m_InputReg[j]) ;
		file.WriteString(buf) ;
	}

	file.Close() ;
	*/
	return true ;
}

//--------------------------------------------------------------------
// Function:  Load figure data from file
// return true when success 
// return false when failure
//--------------------------------------------------------------------
int CInstrument::LoadData()
{
	/*
	CString buf,s ;
	CStdioFile file ;
	int	bRes = 0 ;
	if ( !file.Open("D:\\Developer\\RamanSvr\\Data\\Config.ini",CFile::modeRead)) 
		bRes = 1 ;
	else
	{
		file.SeekToBegin() ;
		for(int i = 0 ; i < 400 ; i++)
		{
			if ( file.ReadString(buf) )
			{
				int pos = buf.Find("  ") ;
				s = buf.Right(buf.GetLength()- pos - 1) ;
				m_HoldReg[i] = atoi(s) ;
			}
			else
			    bRes = i+1 ;
		}

		for(int j = 0 ; j < 400 ; j++)
		{
			if ( file.ReadString(buf) )
			{
				int pos = buf.Find("  ") ;
				s = buf.Right(buf.GetLength()- pos + 1) ;
				m_InputReg[j] = atoi(s) ;
			}
			else
				bRes = j+400 ;
		}
		m_InputReg[24] = m_HoldReg[18] ;
	}
	file.Close() ;
	*/
	return 0 ;
}

//--------------------------------------------------------------------
// Function:  Read Spectrum History data
// return true when success 
// return false when failure
//--------------------------------------------------------------------
bool	CInstrument::ReadHistoryData(u_short year,u_short month, u_short day,u_short index)
{
	CFileFind fFind ;
	char	buf[255] ;
	bool	bRes  = false ;
/*
	sprintf(buf,DATAPATYMD,year,month,day) ;
	int		bFind  = fFind.FindFile(buf) ;
	bool	bHaveFile = false ;
	u_short nIndex = 0 ;
	while (bFind)
	{
		bFind = fFind.FindNextFile() ;
		if (nIndex == index)
		{
			bHaveFile = true ;
			break ;
		}		
		nIndex ++ ;
	}

	if ( bHaveFile )
	{
		CString sFName = fFind.GetFileName() ;
		if ( sFName.GetLength() >= 11 )
		{
			int len ;
			CCsvFileHelper::ReadCsvFile(sFName,m_History,&len) ;

			CString sTime = sFName.Right(10).Left(6) ;
			m_History[PIXEL] = atoi(sTime.Left(2)) ;
			m_History[PIXEL+1] = atoi(sTime.Left(4).Right(2)) ;
			m_History[PIXEL+2] = atoi(sTime.Right(2)) ;
		}
	}
	else
	{
		m_History[PIXEL]   = 0x00AA ;
		m_History[PIXEL+1] = 0X00BB ;
		m_History[PIXEL+2] = 0X00CC ;
	}

	fFind.Close() ;*/

	return bRes ;
}

//--------------------------------------------------------------------
// Function:  Read Spectrum History data
// return true when success 
// return false when failure
//--------------------------------------------------------------------
void	CInstrument::RecvOneChar(unsigned char c) 
{
	if ( !bStartFrame )		//don't start to recv
	{
		if ( 0x01 == c )			//start char of a frame  station no:0x01
		{	
			nTimer = 0 ;
			nCurPos = 0 ;
			nFrameLen = 255 ;
			bStartFrame = true ;
			serialBuf[nCurPos++] = c ;
		}
	}
	else					//start to recv frame
	{
		serialBuf[nCurPos++] = c ;
		if ( nCurPos == 3 )
		{
			switch ( serialBuf[1] )
			{
			case 3:
			case 4:
				nFrameLen = serialBuf[2] + 5 ;
				break ;
			case 16:
				nFrameLen = 8 ;
				break ;
			default:
				bStartFrame = false ;
			}
		}

		if ( nCurPos >= nFrameLen )
		{
			char h,l,sDisp[300] ;
			for(int i=0 ; i < nCurPos ; i++)
			{	
				h = (serialBuf[i] & 0xF0) >> 4 ;
				l = serialBuf[i] & 0x0F ;
				sDisp[3*i]     = (h > 9)?(h + 0x37):(h + 0x30) ;
				sDisp[3*i + 1] = (l > 9)?(l + 0x37):(l + 0x30) ;
				sDisp[3*i + 2] = 0x20 ;
			}
			sDisp[3*nCurPos] = 0 ;

			HandleComm(serialBuf,nCurPos) ;

			bStartFrame = false ;
		}
	}
}

//--------------------------------------------------------------------
// Function:  Compute CRC
//  
// 
//--------------------------------------------------------------------
	const unsigned char auchCRCHi[]=  
		{  
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,  
		0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,  
		0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,  
		0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,  
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,  
		0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,  
		0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,  
		0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,  
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,  
		0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,  
		0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,  
		0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,  
		0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,  
		0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,  
		0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,  
		0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,  
		0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,  
		0x40  
		};  
  
	const unsigned char auchCRCLo[] =  
		{  
		0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4,  
		0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,  
		0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD,  
		0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,  
		0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7,  
		0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,  
		0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE,  
		0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,  
		0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2,  
		0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,  
		0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB,  
		0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,  
		0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91,  
		0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,  
		0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88,  
		0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,  
		0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80,  
		0x40  
		};  

unsigned short CInstrument::GetCRC(unsigned char* CmmBuf,unsigned char Len)
{
	unsigned char uchCRCHi = 0xFF ;
	unsigned char uchCRCLo = 0xFF ;
	unsigned int  uindex ;

	for(int i = 0 ; i < Len ; i++)
	{
		uindex = uchCRCHi ^ CmmBuf[i] ;
		uchCRCHi = uchCRCLo ^ auchCRCHi[uindex] ;
		uchCRCLo = auchCRCLo[uindex] ;
	}
	return ( uchCRCHi << 8 | uchCRCLo) ;
}

//--------------------------------------------------------------------
// Function:  Compute CRC
//  
// 
//--------------------------------------------------------------------
void CInstrument::HandleComm(unsigned char* buf,unsigned char Len)
{
	unsigned short actualCRC = GetCRC(buf,Len-2) ;
	unsigned short recvCRC = ( buf[Len-2] << 8 | buf[Len-1] ) ;
	if ( actualCRC != recvCRC )
	{
		char sDisp[200] ;
		sprintf(sDisp,"CRC Error!: recv  %04X  <-> actual %04X",recvCRC,actualCRC) ;
		char *p = new char[strlen(sDisp)+1] ;
		strcpy(p,sDisp) ;
		::SendMessage(m_hWnd,WM_MODBUSSVR,0,(LPARAM)(LPCTSTR)p) ;
		return ;
	}
	switch (buf[1])
	{
	case 3:
		if ( buf[2] <= 20 )
		{
			for(int i = 0 ; i < buf[2]/2 ; i++)
			{
				m_iPower[i] = buf[3+2*i] << 8 | buf[4 + 2*i] ;
				m_InputReg[i] = m_iPower[i] ;
			}
		}
		break ;
	case 4:
		if ( buf[2] < 20 )
		{
			for(int i = 0 ; i < buf[2]/2 ; i++)
			{
				m_iPower[i] = buf[3+2*i] << 8 | buf[4 + 2*i] ;
				m_InputReg[i] = m_iPower[i] ;
			}
		}
		break ;

	default:
		break ;
	}
}

//--------------------------------------------------------------------
// Function:  production send frame
//  
// 
//--------------------------------------------------------------------
unsigned char* CInstrument::ProductFrame(char nCmd,u_short nAdr,u_short nNum,int* len) 
{
	if ( nNum > 10 )  nNum = 10 ;

	sendFrame[0] = 0x01 ;					//Server address
	sendFrame[1] = nCmd ;					//Command no.
	sendFrame[2] = (nAdr & 0xFF00) >> 8 ;	
	sendFrame[3] = nAdr & 0x00FF ;			//Start addr
	sendFrame[4] = (nNum & 0xFF00) >> 8 ;	
	sendFrame[5] = nNum & 0x00FF ;			//Start addr

	*len = 8 ;
	if ( nCmd == 0x10 )
	{
		sendFrame[6] = 2*nNum ;	
		for(int i=0 ; i < nNum ; i++)
		{
			sendFrame[7 + 2*i] = (m_hPower[i] & 0xFF00) >> 8 ;
			sendFrame[8 + 2*i] = m_hPower[i] & 0x00FF ;
		}
		*len = 9 + 2*nNum ; 
	}

	u_short crc = GetCRC(sendFrame,*len - 2) ;
	sendFrame[*len - 2] = (crc & 0xFF00) >> 8 ;	
	sendFrame[*len - 1] = crc & 0x00FF ;			//crc
	return sendFrame ;
}


//--------------------------------------------------------------------
// Function:  initialize andor camera
//  
// 
//------------------------------------------------------------------------
DWORD WINAPI  CInstrument::InitialCCD_Thread(void* pParam) 
{
	CInstrument *pIns = (CInstrument*)pParam ;
	if ( pIns->m_InputReg[13] > 0 )		//如果已经连接，则无需重新连接
		return 0 ;

	pIns->m_InputReg[13] = 1 ;

	char	aBuffer[256] ;
	float	speed ;
	int		index,iSpeed,iAD ;
	int		test,test2,nAD ;

	int		VSnumber;		//垂直迁移速度对应的号码
	float	VSpeed;			//垂直迁移速度
	int		HSnumber;		//水平迁移速度对应的号码
	float   HSpeed;			//水平迁移速度
	int		ADnumber;

	//AndorCapabilities caps ;
	char	model[32] ;		
	int		gblXPixels ;			//光谱数据长度
	int		gblYPixels ;			//光谱数据宽度

	//delay 2s
	test=GetTickCount();
	do
	{
  		test2=GetTickCount()-test;
	}
	while ( test2 < 4000 );

 /*   GetCurrentDirectory(256,aBuffer);	// Look in current working directory
										// for driver files
    pIns->m_InputReg[12] = Initialize(aBuffer);		// Initialize driver in current directory
    if( pIns->m_InputReg[12] != DRV_SUCCESS)
		return 1;

    // Get camera capabilities
	caps.ulSize = sizeof(AndorCapabilities) ;
     pIns->m_InputReg[12] = GetCapabilities(&caps);
    if( pIns->m_InputReg[12] != DRV_SUCCESS)
		return 2;

    // Get Head Model
     pIns->m_InputReg[12] = GetHeadModel(model);
    if( pIns->m_InputReg[12] != DRV_SUCCESS)
		return 3;

    // Get detector information
     pIns->m_InputReg[12] = GetDetector(&gblXPixels,&gblYPixels);
    if( pIns->m_InputReg[12] != DRV_SUCCESS)
		return 4;

    // Set acquisition mode to required setting specified in xxxxWndw.c
	 pIns->m_InputReg[12] = SetAcquisitionMode(pIns->m_HoldReg[100 + pIns->m_nCurParaNo*6]+1);
    if( pIns->m_InputReg[12] != DRV_SUCCESS)
	   return 5;

	// Set read mode to required setting specified in xxxxWndw.c
	 pIns->m_InputReg[12] = SetReadMode(pIns->m_HoldReg[101 + pIns->m_nCurParaNo*6]);
	if( pIns->m_InputReg[12] != DRV_SUCCESS)
		return 6;
	
	// Set Vertical speed to max
    VSpeed = 0;
    VSnumber = 0;
    GetNumberVSSpeeds(&index);
    for(iSpeed=0; iSpeed<index; iSpeed++){
      GetVSSpeed(iSpeed, &speed);
      if(speed > VSpeed){
        VSpeed = speed;
        VSnumber = iSpeed;
      }
    }
     pIns->m_InputReg[12] = SetVSSpeed(VSnumber);
    if( pIns->m_InputReg[12] != DRV_SUCCESS)
		return 7;

    // Set Horizontal Speed to max
    HSpeed = 0;
    HSnumber = 0;
    ADnumber = 0;
     pIns->m_InputReg[12] = GetNumberADChannels(&nAD);
    if ( pIns->m_InputReg[12] != DRV_SUCCESS)
		return 8;

	for (iAD = 0; iAD < nAD; iAD++) {
		GetNumberHSSpeeds(iAD, 0, &index);
        for (iSpeed = 0; iSpeed < index; iSpeed++) {
          GetHSSpeed(iAD, 0, iSpeed, &speed);
          if(speed > HSpeed){
            HSpeed = speed;
            HSnumber = iSpeed;
            ADnumber = iAD;
          }
        }
      }

     pIns->m_InputReg[12] = SetADChannel(ADnumber);
    if( pIns->m_InputReg[12] != DRV_SUCCESS)
		return 9;

     pIns->m_InputReg[12] = SetHSSpeed(0,HSnumber);
    if( pIns->m_InputReg[12] != DRV_SUCCESS)
		return 10;

  // Wait for 2 seconds to allow MCD to calibrate fully before allowing an
  // acquisition to begin
	test=GetTickCount();
	do
	{
  		test2=GetTickCount()-test;
	}
	while ( test2 < 2000 );

	pIns->m_InputReg[13] = 2;*/

	return 0 ;
}

unsigned int CInstrument::InitialCCD(void) 
{

	if ( m_HoldReg[0] == 0 || m_HoldReg[2] == 0 || m_HoldReg[4] == 0 )
		return 1 ;

	DWORD dwThreadID ;
	CreateThread(NULL,0,InitialCCD_Thread,this,0,&dwThreadID) ;

	return 0 ;

}

//--------------------------------------------------------------------
// Function:  initialize andor camera
//  
// 
//-------------------------------------------------------------------
unsigned int CInstrument::Cooler(bool bSwitch)
{
/*	m_HoldReg[5] = (unsigned short)bSwitch ;

	if ( m_InputReg[13] < 2 )
		return 1 ;

	if ( bSwitch)
	{
		m_InputReg[12] = SetTemperature(m_HoldReg[105 + m_nCurParaNo*6]-200) ;
		m_InputReg[14] = m_HoldReg[105 + m_nCurParaNo*6] ;
		if ( m_InputReg[12] != DRV_SUCCESS )
			return m_InputReg[12] ;

		m_InputReg[12] = CoolerON() ;
		if ( m_InputReg[12] != DRV_SUCCESS )
			return m_InputReg[12] ;
		m_InputReg[11] = 1 ;
	}
	else
	{
		m_InputReg[12] = CoolerOFF() ;
		if ( m_InputReg[12] != DRV_SUCCESS )
			return m_InputReg[12] ;
		m_InputReg[11] = 3;
	}*/

	return 0 ;
}
//--------------------------------------------------------------------
// Function:  initialize andor camera
//  
// 
//-------------------------------------------------------------------
unsigned int	CInstrument::GetTempValue(int* tempature) 
{
/*	if ( m_InputReg[11] == 0  )
		return 1 ;

	int temp ;
	m_InputReg[12] = GetTemperature(&temp) ;
	if ( m_InputReg[12] == DRV_TEMP_STABILIZED )
	{
		m_InputReg[11] = 2;
	}
	if ( m_InputReg[12] != DRV_NOT_INITIALIZED && m_InputReg[12] != DRV_ERROR_ACK )
	{
		*tempature = temp ;
		m_InputReg[10] = temp + 200 ;
	}*/
	return 0 ; 
}


//--------------------------------------------------------------------
// Function:  scaning sample
//  
// 
//-------------------------------------------------------------------
DWORD WINAPI CInstrument::ScanSample_Thread(void* pParam) 
{
/*	CInstrument* pIns = (CInstrument*)pParam ;

	//Currently ready ?
	if ( pIns->m_InputReg[13] != 2 && pIns->m_InputReg[13] != 6 )
		return 1 ;

	pIns->m_InputReg[13] = 3 ;
	//set parameter
	// Set acquisition mode to required setting specified in xxxxWndw.c
	pIns->m_InputReg[12] = SetAcquisitionMode(pIns->m_HoldReg[100 + pIns->m_nCurParaNo*6]+1);
	if( pIns->m_InputReg[12] != DRV_SUCCESS)
	   return 2;

	// Set read mode to required setting specified in xxxxWndw.c
	pIns->m_InputReg[12] = SetReadMode(pIns->m_HoldReg[101 + pIns->m_nCurParaNo*6]);
	if( pIns->m_InputReg[12] != DRV_SUCCESS)
		return 3;

	// Set Exposure Time
	float fExpoTime = pIns->m_HoldReg[102 + pIns->m_nCurParaNo*6]/100.0f ;
	pIns->m_InputReg[12] = SetExposureTime(fExpoTime);
	if( pIns->m_InputReg[12] != DRV_SUCCESS)
		return 3;

	// Set accums cycle time
	float fAccExpoCycle = pIns->m_HoldReg[104 + pIns->m_nCurParaNo*6]/100.0f ;
	pIns->m_InputReg[12] = SetAccumulationCycleTime(fAccExpoCycle);
	if( pIns->m_InputReg[12] != DRV_SUCCESS)
		return 3;

	float exposure,accumulate,kinetic ;

	GetAcquisitionTimings(&exposure, &accumulate, &kinetic);

	pIns->m_InputReg[12] = SetNumberAccumulations(pIns->m_HoldReg[103 + pIns->m_nCurParaNo*6]);
	if (pIns->m_InputReg[12]  != DRV_SUCCESS)
	  return 4 ;

	// It is necessary to get the actual times as the system will calculate the
	// nearest possible time. eg if you set exposure time to be 0, the system
	// will use the closest value (around 0.01s)
	GetAcquisitionTimings(&exposure, &accumulate, &kinetic);

	// Set trigger mode
//	pIns->m_InputReg[12] =SetBaselineClamp(1);
//	if(pIns->m_InputReg[12] !=DRV_SUCCESS)
//	  return 5 ;

	pIns->m_InputReg[12] = SetTriggerMode(0);
	if(pIns->m_InputReg[12] !=DRV_SUCCESS)
	  return 6;

//	pIns->m_InputReg[12] =SetPreAmpGain(2);
//	if(pIns->m_InputReg[12] != DRV_SUCCESS)
//	  return 7;

	pIns->m_InputReg[13] = 4 ;
	pIns->m_InputReg[15] = 0 ;
  // Starting the acquisition also starts a timer which checks the card status
  // When the acquisition is complete the data is read from the card and
  // displayed in the paint area.
	pIns->m_InputReg[12]=StartAcquisition();
	if(pIns->m_InputReg[12] != DRV_SUCCESS)
	{
		AbortAcquisition();
		return 8;
	}

	pIns->m_InputReg[13] = 5 ;
//	int nTimeout = 10 + pIns->m_HoldReg[102 + m_nCurParaNo*6] / 10 ;	//timeout time setting
	int nStatus ;
	int nCode ;

	while ( true )
	{
		Sleep(100) ;
		nCode = GetStatus(&nStatus) ;
		if ( nStatus == DRV_ACQUIRING )
			continue ;
		if ( nStatus == DRV_IDLE )
		{
			nCode = GetAcquiredData16(pIns->m_ScanData,PIXEL) ;

			pIns->m_ScanData[PIXEL]   = pIns->m_InputReg[19] ;
			pIns->m_ScanData[PIXEL+1] = pIns->m_InputReg[20] ;
			pIns->m_ScanData[PIXEL+2] = pIns->m_InputReg[21] ;
			if ( nCode != DRV_SUCCESS )
			{
				pIns->m_InputReg[13] = 2 ;
				return 9 ;
			}
			else
				break ;
		}
	}

	CCsvFileHelper::WriteCsvFile(pIns->m_ScanData,PIXEL) ;
	pIns->m_InputReg[13] = 6 ;
	pIns->m_InputReg[15] = 1 ;
	*/
	return 0 ;
}

//--------------------------------------------------------------------
// Function:  control function
//  
// 
//-------------------------------------------------------------------
void CInstrument::OnTimer_1s()
{
	//*****************************************
	SYSTEMTIME tm ;
	GetLocalTime(&tm) ;
	LockData() ;
	m_InputReg[16] = tm.wYear ;
	m_InputReg[17] = tm.wMonth ;
	m_InputReg[18] = tm.wDay ;
	m_InputReg[19] = tm.wHour ;
	m_InputReg[20] = tm.wMinute ;
	m_InputReg[21] = tm.wSecond ;
	UnlockData() ;

	char buf[100] ;
	sprintf(buf,"%02d:%02d:%02d,%01d%01d%01d%01d%01d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
			tm.wHour,tm.wMinute,tm.wSecond,
			m_InputReg[0],m_InputReg[1],m_InputReg[2],m_InputReg[3],m_InputReg[4],
			m_InputReg[5],m_InputReg[6],m_InputReg[7],m_InputReg[8],m_InputReg[9],
			m_InputReg[10],m_InputReg[11],m_InputReg[12],m_InputReg[13],m_InputReg[14]) ;
	//log.AddLogRecord(tm.wYear,tm.wMonth,tm.wDay,buf) ;	

	//save parameter per Hour
	if ( tm.wMinute == 0 && tm.wSecond == 0 )
		SaveData() ;
	
	//step1   trun on power
	for(int i = 0 ; i <= m_HoldReg[200] ; i++)
	{
		if (  tm.wHour == m_HoldReg[201 + 5*i] &&
			  tm.wMinute == m_HoldReg[202 + 5*i] &&
			  tm.wSecond== m_HoldReg[203 + 5*i] &&
			  m_HoldReg[18] == 1 )
		{
			m_nScaning = 0 ;
			m_nCurPowerNO = m_HoldReg[205 + 5*i] ;				
			m_nCurParaNo = m_HoldReg[204 + 5*i] ;
			if (  m_InputReg[13] == 0 )
			{
				unsigned char buf[] = {0,1,0,1,0,1,0,1,0,1} ;
				SetMultReg(30000,5,buf) ;
				m_InputReg[23] = 1 ;
			
				return ;
			}
			else
			{
				m_InputReg[23] = 100 ;
				return ;
			}

		}
	}

	// counter work?
	if ( m_InputReg[23] != 0 )
		m_InputReg[23] ++ ;

	// delay 2s,and to initialize CCD
	if ( m_InputReg[23] == 2 )
	{
		InitialCCD() ; 
		m_InputReg[23] = 100 ;
		nInitCount = 0 ;
		return ;
	}

	// initialize do not success in 30s
	if ( m_InputReg[23] >= 130 && m_InputReg[23] < 1000 && m_InputReg[13] != 2 )
	{
		if ( nInitCount++ >= 3 )
		{
			Halt_byFault() ;
			return ;
		}
		m_InputReg[13] = 0 ;
		InitialCCD() ; 
		m_InputReg[23] = 100 ;
		return ;
	}

	//cooler
	if ( m_InputReg[23] > 100 && m_InputReg[23] < 1000 && m_InputReg[13] == 2 )
	{
		Cooler(true) ;
		m_InputReg[23] = 1000 ;
		return ;
	}
	
	//cool fault?
	if ( m_InputReg[23] > 4000 && m_InputReg[23] < 5000 && m_InputReg[10] < m_HoldReg[105 + m_nCurParaNo*6] )
	{
		Halt_byFault() ;
		return ;
	}

	//scaning
	if ( m_InputReg[23] > 1005 && 
		 m_InputReg[23] < 5000 && 
		 ( m_InputReg[10] <= m_HoldReg[105 + m_nCurParaNo*6] || m_InputReg[11] ==2 ) )
	{
		Scanning() ;
		m_InputReg[23] = 5000 ;
		return ;
	}

	if ( m_InputReg[23] > 5005 && m_InputReg[13] == 6 && m_nScaning < m_nCurPowerNO-1)
	{
		m_nScaning++ ;
		m_InputReg[11] = 2 ;
		m_InputReg[23] = 1000 ;
		return ;
	}

	//swich off cooler and laser
	if ( m_InputReg[23] > 5005 && m_InputReg[23] < 8000 && m_InputReg[13] == 6 )
	{
		LockData() ;
		m_HoldReg[3] = 0 ;
		m_hPower[3] = 0 ;
		UnlockData() ;

		Cooler(false) ;
		m_InputReg[23] = 8000 ;
		return ;
	}

	//switch off  ccd
	if ( m_InputReg[23] > 8005 && m_InputReg[10] > 190 )
	{
		//ShutDown() ;
		Halt_byFault() ;
		return ;
	}
}

//---------------------------------------------------------------------------------
void	CInstrument::Halt_byFault()
{
	LockData() ;
	m_HoldReg[0] = 0 ;	m_hPower[0] = 0 ;
	m_HoldReg[2] = 0 ;	m_hPower[2] = 0 ;
	m_HoldReg[3] = 0 ;	m_hPower[3] = 0 ;
	m_HoldReg[4] = 0 ;	m_hPower[4] = 0 ;
	m_nCurParaNo = 0 ;
	UnlockData() ;
	m_InputReg[23] = 0 ;
	m_InputReg[13] = 0 ;
	m_nCurParaNo = 0 ;
}







