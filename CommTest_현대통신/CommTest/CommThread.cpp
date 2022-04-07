#include "stdafx.h"
#include "CommThread.h"
#include "resource.h"
// 메시지를 받을 윈도 핸들, 이 클래스를 사용하는 윈도에서 HWND hCommwnd this-> m_hwnd 로 설정
CWnd* pWnd = AfxGetMainWnd();
HWND hCommWnd;

// Queue 를 초기화

void CQueue::Clear()
{
	m_iHead = m_iTail = 0;
	memset(buff, 0, BUFF_SIZE);
}

CQueue::CQueue()
{
	Clear();
}

int CQueue::GetSize()
{
	return (m_iHead - m_iTail + BUFF_SIZE) % BUFF_SIZE;
}

// Queue 에 1byte 넣음.

BOOL CQueue::PutByte(BYTE b)
{
	// Queue가 꽉 찾는지 검사
	if (GetSize() == (BUFF_SIZE - 1)) return FALSE;
	// Queue에 한 바이트를 넣고 Head Pointer를 증가
	buff[m_iHead++] = b;
	m_iHead %= BUFF_SIZE;
	return TRUE;
}

BOOL CQueue::GetByte(BYTE* pb)
{
	// Queue가 비었는지 검사
	if (GetSize() == 0) return FALSE;
	*pb = buff[m_iTail++];
	m_iTail %= BUFF_SIZE;
	return TRUE;
}

BOOL CommThread::OpenPort(CString sPortName, DWORD dwBaud, WORD wPortID,HWND hwnd)
{
	hCommWnd = hwnd;
	// Local 변수
	COMMTIMEOUTS timeouts; // Timeout Value를 설정하기 위한 구조체
	DCB dcb; // Serial Port의 특성을 설정하기 위한 구조체
	DWORD dwThreadID; // Thread ID를 얻음

	// 변수 설정
	m_bConnected = FALSE;
	m_wPortID = wPortID; // COM1 -> 0 , COM2->1,,,,

	// overlapped structure 변수 초기화

	m_osRead.Offset = 0;
	m_osRead.OffsetHigh = 0;

	m_osWrite.Offset = 0;
	m_osWrite.OffsetHigh = 0;

	if (!(m_osRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL)))
	{
		CloseHandle(m_osRead.hEvent);
		return FALSE;
	}
	if (!(m_osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL)))
	{
		CloseHandle(m_osWrite.hEvent);
		return FALSE;
	}



	// 포트 열기

	m_sPortName = sPortName;
	m_hComm = CreateFile(m_sPortName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);

	if (m_hComm == (HANDLE)-1) return FALSE; // 포트 열기 실패

	// EV_RXCHAR event 설정

	SetCommMask(m_hComm, EV_RXCHAR);

	// Serail Port 장치의 InQueue, OutQueue 크기 설정

	SetupComm(m_hComm, 4096, 4096);

	// 포트 InQueue , OutQueue 초기화
	PurgeComm(m_hComm, PURGE_TXABORT | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_RXCLEAR);

	timeouts.ReadIntervalTimeout = 0xFFFFFFFF;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutMultiplier = 2 * CBR_9600 / dwBaud;
	SetCommTimeouts(m_hComm, &timeouts);

	// dcb  설정
	dcb.DCBlength = sizeof(DCB);
	GetCommState(m_hComm, &dcb);
//	dcb.BaudRate = dwBaud;
//	dcb.ByteSize = 8;
//	dcb.Parity = 0;
//	dcb.StopBits = 1;
	
	dcb.fInX = dcb.fOutX = 0; // Xon , Xoff 사용 
							  // Xon , Xoff 사용하지 않을 경우는 CriticalSection을 설정 하여 보호해 주어야 한다.

	//dcb.XonChar = ASCII_XON;
	//dcb.XoffChar = ASCII_X0FF;
	//dcb.XonLim = 2048;
	//dcb.XoffLim = 1024;

	if (!SetCommState(m_hComm, &dcb))
	{
		UINT err = GetLastError();
		return FALSE;
	}

	// 포트 감시 스레드 생성

	m_bConnected = TRUE; // 연결이 성립되었음을 세팅
	m_hThreadWatchComm = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadWatchComm, this, 0, &dwThreadID);
	EscapeCommFunction(m_hComm, SETDTR);

	if (!m_hThreadWatchComm)
	{
		// 실패하면 포트를 닫는다.
		ClosePort();
		return FALSE;
	}

	return TRUE;
}

void CommThread::ClosePort()
{
	m_bConnected = FALSE;
	// 모든 Event mask를 없앤다.
	SetCommMask(m_hComm, 0);
	// 통신 Queue들을 초기화 한다.
	PurgeComm(m_hComm, PURGE_TXABORT | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_RXCLEAR);
	// 파일 핸들을 닫는다.
	CloseHandle(m_hComm);
}

// 포트에 pBuff의 내용을 nToWrite만큼 쓴다.
// 실제로 쓰여진 Byte수를 리턴한다.

DWORD CommThread::WriteComm(LPCTSTR pBuff, int nTOWrite)
{
	DWORD dwWritten, dwError, dwErrorFlags;
	COMSTAT comstat;

	if (!WriteFile(m_hComm, pBuff, nTOWrite, &dwWritten,&m_osWrite))
	{
		if (GetLastError() == ERROR_IO_PENDING)
		{
			//읽을 문자가 남아 있지거나 전송할 문자가 남아 있으 경우
			// Overapped IO의 
			// 특성에 따라 ERROR_IO_PENDING 에러 메시지가 전달된다.
			//timeout에 정해준 시간만큼 기다려준다.

			while (!GetOverlappedResult(m_hComm, &m_osWrite, &dwWritten, TRUE))
			{
				dwError = GetLastError();
				if (dwError != ERROR_IO_INCOMPLETE)
				{
					ClearCommError(m_hComm, &dwErrorFlags, &comstat);
					break;
				}
			}
		}
		else
		{
			dwWritten = 0;
			ClearCommError(m_hComm, &dwErrorFlags, &comstat);
		}
	}
	return dwWritten;
}


// 포트로부터 pBuff에 nToWrite만큼 읽는다.
// 실제로 읽혀진 Byte수를 리턴한다.

DWORD CommThread::ReadComm(BYTE* pBuff, DWORD nToRead)
{

	DWORD dwRead, dwError, dwErrorFlags;
	COMSTAT comstat;

	ClearCommError(m_hComm, &dwError, &comstat);
	// input Queue에 들어와 있는 데이터의 길이
	dwRead = comstat.cbInQue;

	if (dwRead > 0)
	{
		if (!ReadFile(m_hComm, pBuff, nToRead, &dwRead, &m_osRead))
		{
			if (GetLastError() == ERROR_IO_PENDING)
			{
				// 입출력중인 데이터가 있는 경우 timeout에 정해준 시간만큼 기다려준다.
				while (!GetOverlappedResult(m_hComm, &m_osRead, &dwRead, TRUE))
				{
					dwError = GetLastError();
					if (dwError != ERROR_IO_INCOMPLETE)
					{
						ClearCommError(m_hComm, &dwErrorFlags, &comstat);
						break;
					}
				}
			}
			else
			{
				dwRead = 0;
				ClearCommError(m_hComm, &dwErrorFlags, &comstat);
			}
		}
	}
	return dwRead;
}

// 포트를 감시하고, Serial Port에서 메시지가 발생하면 입력 메시지인가 검사하고 내용이
// 있으면 m_ReadData에 저장한 뒤에 MainWnd에 메시지를 보내어 버퍼의 내용을
// 읽어 가라고 WM_COMM_READ 메시지를 보낸다.

DWORD ThreadWatchComm(CommThread* pComm)
{
	DWORD dwEvent;
	OVERLAPPED os;
	BOOL bOk = TRUE; // 리턴값
	
	BYTE buff[2048]; // InQueue의 데이터를 읽어오기 위한 Input Buffer
	DWORD dwRead; // 읽은 바이트수

	// Event , Overap Structure 설정 

	memset(&os, 0, sizeof(OVERLAPPED));
	if (!(os.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL)))
	bOk = FALSE;
	if (!SetCommMask(pComm->m_hComm, EV_RXCHAR))
		bOk = FALSE;
	if (!bOk)
	{
		AfxMessageBox("Error while creating ThreadWatchComm,  " + pComm->m_sPortName);
		return FALSE;
	}

	// 포트를 감시하는 루프

	while (pComm->m_bConnected) // 주 프로세스에서 이 값을 FALSE로 바꿔주면 스레드가 끝난다.
	{
		dwEvent = 0;

		// 포트에서 메시지가 발생할 때까지 기다린다.

		WaitCommEvent(pComm->m_hComm , &dwEvent, NULL);


		if ((dwEvent & EV_RXCHAR) == EV_RXCHAR)
		{
			// 포트에서 읽을 수 있는 만큼 읽는다.
			::PostMessage(hCommWnd, WM_COMM_READ, pComm->m_wPortID, 0);
			do
			{
				dwRead = pComm->ReadComm(buff, 2048);

				//CommThead의 Read Queue가 꽉 차 있는지 검사
				if (BUFF_SIZE - pComm->m_QueueRead.GetSize() > (int)dwRead)
				{
					for (WORD i = 0; i < dwRead; i++)
						pComm->m_QueueRead.PutByte(buff[i]);
				}
				else
					AfxMessageBox(_T("m_QueueRead FULL!"));
				
			} while (dwRead);

			//CEdit* p = (CEdit*)GetDlgItem(hCommWnd,IDC_EDIT2);

			//p->SetWindowTextW((LPCTSTR)buff);
			
			TRACE(traceAppMsg, 0, "데이터를 받았습니다.\r\n");
			// 읽어가도록 메시지를 보낸다.
			::PostMessage(hCommWnd, WM_MYMESSAGE, pComm->m_wPortID,0);
			//TRACE(traceAppMsg, 0, "경고: 대화 상자를 만들지 못했으므로 응용 프로그램이 예기치 않게 종료됩니다.\n");
		}
	}
	// 포트가 ClosePort에 의해 닫히면 m_bConnected가 FALSE가 되어 종료

	CloseHandle(os.hEvent);
	pComm->m_hThreadWatchComm = NULL;

	return TRUE;
}