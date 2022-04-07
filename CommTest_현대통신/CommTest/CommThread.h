#pragma once
#include <afxwin.h>

#define BUFF_SIZE 8192 // THREAD�� �θ� ���μ����� �����͸� �ְ� ���� ���� ũ��
#define WM_COMM_READ (WM_USER+2) // THREAD�� �θ� ���μ����� ���� �޽���

//#define ASCII_LF 0x0a
//#define ASCII_CR 0x0d
#define ASCII_XON 0x11
#define ASCII_X0FF 0x13

class CQueue
{
public : 
	BYTE buff[BUFF_SIZE]; // CQueue Buffer
	int m_iHead, m_iTail;  // CQueue Head Tail Position
	CQueue();
	void Clear(); // Queue�� ������ �����.
	int GetSize(); // Queue�� ��� �ִ� �������� ���̸� �����Ѵ�.
	BOOL PutByte(byte b); // Queue�� 1byte �ֱ�
	BOOL GetByte(byte* pb); // Queue�� 1byte ������

};



class CommThread
{

public : 
	HANDLE m_hComm; // ��� ��Ʈ ���� �ڵ�
	CString m_sPortName; // ��Ʈ �̸�
	BOOL m_bConnected;  // ��Ʈ�� ���ȴ��� Ȯ��
	OVERLAPPED m_osRead, m_osWrite; // ��Ʈ ���� IO�� ���� OverLapped structure

	HANDLE m_hThreadWatchComm; // ��Ʈ�� ������ �Լ�, �������� �ڵ��� ����

	WORD m_wPortID; // WM_COMM_READ�� �Բ� ������ �μ��� 
									// ���� ���� ��Ʈ�� ������ ��� � ��Ʈ���� 
									// �ĺ��ϴµ� ���

	CQueue m_QueueRead; // ������� �θ� ���μ������� ��� ����
	// Serial Port�� ����. ���ڷ� ��Ʈ���� String�� �ӵ�, ��Ʈ ��ȣ�� �ش�.
	BOOL OpenPort(CString sPortName, DWORD dwBaud, WORD wParam,HWND hwnd);
	// Serial Port�� �ݴ´�
	void ClosePort();
	// Serial Port�� �����͸� ����.
	DWORD WriteComm(LPCTSTR pBuff, int nToWrite);
	// �����尡 �޽����� ������ ��� �� �Լ��� �̿��ؼ� �����͸� �д´�.
	DWORD ReadComm(BYTE* pBuff, DWORD nToRead);

};

DWORD ThreadWatchComm(CommThread* pComm);



