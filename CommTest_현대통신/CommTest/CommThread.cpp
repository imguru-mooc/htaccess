#include "stdafx.h"
#include "CommThread.h"
#include "resource.h"
// �޽����� ���� ���� �ڵ�, �� Ŭ������ ����ϴ� �������� HWND hCommwnd this-> m_hwnd �� ����
CWnd* pWnd = AfxGetMainWnd();
HWND hCommWnd;

// Queue �� �ʱ�ȭ

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

// Queue �� 1byte ����.

BOOL CQueue::PutByte(BYTE b)
{
	// Queue�� �� ã���� �˻�
	if (GetSize() == (BUFF_SIZE - 1)) return FALSE;
	// Queue�� �� ����Ʈ�� �ְ� Head Pointer�� ����
	buff[m_iHead++] = b;
	m_iHead %= BUFF_SIZE;
	return TRUE;
}

BOOL CQueue::GetByte(BYTE* pb)
{
	// Queue�� ������� �˻�
	if (GetSize() == 0) return FALSE;
	*pb = buff[m_iTail++];
	m_iTail %= BUFF_SIZE;
	return TRUE;
}

BOOL CommThread::OpenPort(CString sPortName, DWORD dwBaud, WORD wPortID,HWND hwnd)
{
	hCommWnd = hwnd;
	// Local ����
	COMMTIMEOUTS timeouts; // Timeout Value�� �����ϱ� ���� ����ü
	DCB dcb; // Serial Port�� Ư���� �����ϱ� ���� ����ü
	DWORD dwThreadID; // Thread ID�� ����

	// ���� ����
	m_bConnected = FALSE;
	m_wPortID = wPortID; // COM1 -> 0 , COM2->1,,,,

	// overlapped structure ���� �ʱ�ȭ

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



	// ��Ʈ ����

	m_sPortName = sPortName;
	m_hComm = CreateFile(m_sPortName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);

	if (m_hComm == (HANDLE)-1) return FALSE; // ��Ʈ ���� ����

	// EV_RXCHAR event ����

	SetCommMask(m_hComm, EV_RXCHAR);

	// Serail Port ��ġ�� InQueue, OutQueue ũ�� ����

	SetupComm(m_hComm, 4096, 4096);

	// ��Ʈ InQueue , OutQueue �ʱ�ȭ
	PurgeComm(m_hComm, PURGE_TXABORT | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_RXCLEAR);

	timeouts.ReadIntervalTimeout = 0xFFFFFFFF;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutMultiplier = 2 * CBR_9600 / dwBaud;
	SetCommTimeouts(m_hComm, &timeouts);

	// dcb  ����
	dcb.DCBlength = sizeof(DCB);
	GetCommState(m_hComm, &dcb);
//	dcb.BaudRate = dwBaud;
//	dcb.ByteSize = 8;
//	dcb.Parity = 0;
//	dcb.StopBits = 1;
	
	dcb.fInX = dcb.fOutX = 0; // Xon , Xoff ��� 
							  // Xon , Xoff ������� ���� ���� CriticalSection�� ���� �Ͽ� ��ȣ�� �־�� �Ѵ�.

	//dcb.XonChar = ASCII_XON;
	//dcb.XoffChar = ASCII_X0FF;
	//dcb.XonLim = 2048;
	//dcb.XoffLim = 1024;

	if (!SetCommState(m_hComm, &dcb))
	{
		UINT err = GetLastError();
		return FALSE;
	}

	// ��Ʈ ���� ������ ����

	m_bConnected = TRUE; // ������ �����Ǿ����� ����
	m_hThreadWatchComm = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadWatchComm, this, 0, &dwThreadID);
	EscapeCommFunction(m_hComm, SETDTR);

	if (!m_hThreadWatchComm)
	{
		// �����ϸ� ��Ʈ�� �ݴ´�.
		ClosePort();
		return FALSE;
	}

	return TRUE;
}

void CommThread::ClosePort()
{
	m_bConnected = FALSE;
	// ��� Event mask�� ���ش�.
	SetCommMask(m_hComm, 0);
	// ��� Queue���� �ʱ�ȭ �Ѵ�.
	PurgeComm(m_hComm, PURGE_TXABORT | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_RXCLEAR);
	// ���� �ڵ��� �ݴ´�.
	CloseHandle(m_hComm);
}

// ��Ʈ�� pBuff�� ������ nToWrite��ŭ ����.
// ������ ������ Byte���� �����Ѵ�.

DWORD CommThread::WriteComm(LPCTSTR pBuff, int nTOWrite)
{
	DWORD dwWritten, dwError, dwErrorFlags;
	COMSTAT comstat;

	if (!WriteFile(m_hComm, pBuff, nTOWrite, &dwWritten,&m_osWrite))
	{
		if (GetLastError() == ERROR_IO_PENDING)
		{
			//���� ���ڰ� ���� �����ų� ������ ���ڰ� ���� ���� ���
			// Overapped IO�� 
			// Ư���� ���� ERROR_IO_PENDING ���� �޽����� ���޵ȴ�.
			//timeout�� ������ �ð���ŭ ��ٷ��ش�.

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


// ��Ʈ�κ��� pBuff�� nToWrite��ŭ �д´�.
// ������ ������ Byte���� �����Ѵ�.

DWORD CommThread::ReadComm(BYTE* pBuff, DWORD nToRead)
{

	DWORD dwRead, dwError, dwErrorFlags;
	COMSTAT comstat;

	ClearCommError(m_hComm, &dwError, &comstat);
	// input Queue�� ���� �ִ� �������� ����
	dwRead = comstat.cbInQue;

	if (dwRead > 0)
	{
		if (!ReadFile(m_hComm, pBuff, nToRead, &dwRead, &m_osRead))
		{
			if (GetLastError() == ERROR_IO_PENDING)
			{
				// ��������� �����Ͱ� �ִ� ��� timeout�� ������ �ð���ŭ ��ٷ��ش�.
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

// ��Ʈ�� �����ϰ�, Serial Port���� �޽����� �߻��ϸ� �Է� �޽����ΰ� �˻��ϰ� ������
// ������ m_ReadData�� ������ �ڿ� MainWnd�� �޽����� ������ ������ ������
// �о� ����� WM_COMM_READ �޽����� ������.

DWORD ThreadWatchComm(CommThread* pComm)
{
	DWORD dwEvent;
	OVERLAPPED os;
	BOOL bOk = TRUE; // ���ϰ�
	
	BYTE buff[2048]; // InQueue�� �����͸� �о���� ���� Input Buffer
	DWORD dwRead; // ���� ����Ʈ��

	// Event , Overap Structure ���� 

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

	// ��Ʈ�� �����ϴ� ����

	while (pComm->m_bConnected) // �� ���μ������� �� ���� FALSE�� �ٲ��ָ� �����尡 ������.
	{
		dwEvent = 0;

		// ��Ʈ���� �޽����� �߻��� ������ ��ٸ���.

		WaitCommEvent(pComm->m_hComm , &dwEvent, NULL);


		if ((dwEvent & EV_RXCHAR) == EV_RXCHAR)
		{
			// ��Ʈ���� ���� �� �ִ� ��ŭ �д´�.
			::PostMessage(hCommWnd, WM_COMM_READ, pComm->m_wPortID, 0);
			do
			{
				dwRead = pComm->ReadComm(buff, 2048);

				//CommThead�� Read Queue�� �� �� �ִ��� �˻�
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
			
			TRACE(traceAppMsg, 0, "�����͸� �޾ҽ��ϴ�.\r\n");
			// �о���� �޽����� ������.
			::PostMessage(hCommWnd, WM_MYMESSAGE, pComm->m_wPortID,0);
			//TRACE(traceAppMsg, 0, "���: ��ȭ ���ڸ� ������ �������Ƿ� ���� ���α׷��� ����ġ �ʰ� ����˴ϴ�.\n");
		}
	}
	// ��Ʈ�� ClosePort�� ���� ������ m_bConnected�� FALSE�� �Ǿ� ����

	CloseHandle(os.hEvent);
	pComm->m_hThreadWatchComm = NULL;

	return TRUE;
}