
// CommTestDlg.cpp : ���� ����
//

#include "stdafx.h"
#include "CommTest.h"
#include "CommTestDlg.h"
#include "afxdialogex.h"
#include "Serial.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif




// type ������ �Ҷ� ���Ǵ� �Լ�

CString int_to_CString(int x)
{
	CString str;
	str.Format(_T("%d"), x);
	return str;
}

int char_to_int(char c)
{
	int x = (int)c;
	return x - '0';
}

char int_to_char(int x)
{
	CString str;
	str.Format(_T("%d"), x);
	return str[0];
}

// CCommTestDlg ��ȭ ����



CCommTestDlg::CCommTestDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CCommTestDlg::IDD, pParent)
	, m_nIdxPort(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_bSerialConnected = false;
	mIdx = 0;
}

void CCommTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_CBIndex(pDX, IDC_CBB_PORT, m_nIdxPort);
	DDX_Control(pDX, IDC_REGIST, m_List);
	DDX_Control(pDX, IDC_EDIT_NAME, mName);
	DDX_Control(pDX, IDC_BTN_REGIST, mRegist);
	DDX_Control(pDX, IDC_BTN_REMOVE, mRemove);
	//  DDX_Control(pDX, IDC_CBB_PORT, mConnect);
	DDX_Control(pDX, IDC_BTN_CONNECT, mConnect);
	DDX_Control(pDX, IDC_EDIT_NOTICE, mNotice);
}

BEGIN_MESSAGE_MAP(CCommTestDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_CONNECT, &CCommTestDlg::OnBnClickedBtnConnect)
	ON_BN_CLICKED(IDC_BTN_REGIST, &CCommTestDlg::OnBnClickedBtnRegist)
	ON_BN_CLICKED(IDC_BTN_REMOVE, &CCommTestDlg::OnBnClickedBtnRemove)
    ON_MESSAGE(WM_MYMESSAGE, &CCommTestDlg::OnReceive)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_REGIST, &CCommTestDlg::OnLvnItemchangedRegist)
	ON_BN_CLICKED(IDC_BTN_FEATURE, &CCommTestDlg::OnBnClickedBtnFeature)
	ON_BN_CLICKED(IDC_BTN_NOTICE, &CCommTestDlg::OnBnClickedBtnNotice)
END_MESSAGE_MAP()


// CCommTestDlg �޽��� ó����

BOOL CCommTestDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// �� ��ȭ ������ �������� �����մϴ�. ���� ���α׷��� �� â�� ��ȭ ���ڰ� �ƴ� ��쿡��
	//  �����ӿ�ũ�� �� �۾��� �ڵ����� �����մϴ�.
	SetIcon(m_hIcon, TRUE);			// ū �������� �����մϴ�.
	SetIcon(m_hIcon, FALSE);		// ���� �������� �����մϴ�.

	// TODO: ���⿡ �߰� �ʱ�ȭ �۾��� �߰��մϴ�.
    m_bSerialConnected=false;

	((CComboBox*)GetDlgItem(IDC_CBB_PORT))->SetCurSel(0);

	CRect rect;
	m_List.GetClientRect(&rect);

	m_List.InsertColumn(0, "idx", LVCFMT_LEFT, 80);
	m_List.InsertColumn(1, "�̸�", LVCFMT_LEFT, rect.Width()-80);

	//m_List.InsertItem(0, "1234", 0);
	//m_List.SetItemText(0, 1, "hong");
	//m_List.InsertItem(0, "5678", 0);
	//m_List.SetItemText(0, 1, "kim");

	mRegist.EnableWindow(FALSE);
	mRemove.EnableWindow(FALSE);
 
	mFirst = false;
	DWORD dwThreadID;
	m_hThreadDisp = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadDisp, this, 0, &dwThreadID);
	mState = NORMAL;



	return TRUE;  // ��Ŀ���� ��Ʈ�ѿ� �������� ������ TRUE�� ��ȯ�մϴ�.
}

// ��ȭ ���ڿ� �ּ�ȭ ���߸� �߰��� ��� �������� �׸�����
//  �Ʒ� �ڵ尡 �ʿ��մϴ�. ����/�� ���� ����ϴ� MFC ���� ���α׷��� ��쿡��
//  �����ӿ�ũ���� �� �۾��� �ڵ����� �����մϴ�.

void CCommTestDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // �׸��⸦ ���� ����̽� ���ؽ�Ʈ�Դϴ�.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Ŭ���̾�Ʈ �簢������ �������� ����� ����ϴ�.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// �������� �׸��ϴ�.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// ����ڰ� �ּ�ȭ�� â�� ���� ���ȿ� Ŀ���� ǥ�õǵ��� �ý��ۿ���
//  �� �Լ��� ȣ���մϴ�.
HCURSOR CCommTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CCommTestDlg::OnBnClickedBtnConnect()
{
	UpdateData(TRUE);
	int nPort = ((CComboBox*)GetDlgItem(IDC_CBB_PORT))->GetCurSel()+1;
	CString strTemp;
	strTemp.Format(nPort >= 10 ? _T("\\\\.\\COM%d") : _T("COM%d"), nPort);

	if (m_ComuPort.OpenPort(strTemp, 9600, nPort-1, AfxGetApp()->m_pMainWnd->m_hWnd))
	{
		AfxMessageBox(_T("����"));
		m_bSerialConnected = true;
		mRegist.EnableWindow(TRUE);
		mRemove.EnableWindow(TRUE);
		mConnect.EnableWindow(FALSE);
	}
    else
    {
		AfxMessageBox("Connection Error[Port ���� ��Ȯ��]");
		return;
    }
	unsigned char buff[32] = { 0xF7,0x0C,0x01,0x53,0x01,0x12,0x11,0x01,0x00,0x0A,0x00,0xEE };
	CalcCRC(buff);
	m_ComuPort.WriteComm((LPCTSTR)buff, buff[1]);
	mState = INIT_REQ;
}


CCommTestDlg::~CCommTestDlg()
{
	m_ComuPort.ClosePort();
	m_ComuPort.m_bConnected = false;
}


void CCommTestDlg::OnBnClickedBtnRegist()
{
	CString name;
	unsigned char REGIST_START[32] = { 0xF7,0x00,0x01,0x53,0x02,0x21,0x11,0x00,0x01,};

	mName.GetWindowTextA(name);
	int len = name.GetLength();
	if (name == "")
	{
		AfxMessageBox(_T("�̸��� �Է��� �ּ���"));
		return;
	}

	LPCWSTR str;
	len = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, name, -1, NULL, NULL);
	str = (LPCWSTR)new TCHAR[len];
	MultiByteToWideChar(CP_ACP, 0, name, -1, (LPWSTR)(LPCWSTR)str, len);
	len--;
	memcpy((char*)REGIST_START+9, (char*)str, len*2);
	//delete[] str;
	int total_len = 10 + len*2 + 1;
	REGIST_START[1] = total_len;
	REGIST_START[total_len - 1] = 0xEE;
	CalcCRC(REGIST_START);

	m_ComuPort.WriteComm((LPCTSTR)REGIST_START, REGIST_START[1]);

	mNameSave = name;
	mState = REGIST_REQ;
}


void CCommTestDlg::OnBnClickedBtnRemove()
{
	CString name;
	mName.GetWindowTextA(name);
	int len = name.GetLength();
	if (name == "")
	{
		AfxMessageBox(_T("����Ʈ���� ������ �̸��� ������ �ּ���"));
		return;
	}

	unsigned char buff[32] = { 0xF7,0x0B,0x01,0x53,0x02,0x31,0x11,0x00,0x00,0x00,0xEE };
	buff[8] = mIdx;
	CalcCRC(buff);
	m_ComuPort.WriteComm((LPCTSTR)buff, buff[1]);

	mState = REMOVE_REQ;
}

int CCommTestDlg::readn(unsigned char* buf, int len)
{
	int	remains = len;
	bool ret = 0;

	while (remains > 0)
	{
		ret = this->m_ComuPort.m_QueueRead.GetByte(&buf[len - remains]);

		if (ret == FALSE)
		{
			Sleep(10);	//10ms
			continue;
		}
		remains--;
	}
	return(len);
}

int CCommTestDlg::dispatch()
{
	BYTE buf[1024];
	int len, str_len;
	readn(buf, 1);
	if (buf[0] != 0xF7)
		return 0;

	readn(buf+1, 1);
	len = buf[1];
	readn(buf+2, len-2);

	
	char temp[32] = { 0, };
	CString msg;
	for (int i = 0; i < len; i++)
	{
		sprintf(temp, "%02x ", buf[i]);
		msg += temp;
	}
	// CalcCRC(buf);

	int type = buf[4];
	int srv = buf[5];
	if (type == 0x04) {
	    if (srv == 0x11) {
		    AfxMessageBox(msg);
	    } else if (srv == 0x12) {
			char temp[128];
			int curr = 0;
			for (int i = 0; i < buf[9]; i++) {
				sprintf(temp, "%04d", buf[10+curr]);
				m_List.InsertItem(i, temp, 0);

				memset(temp, 0, sizeof(temp));
				memcpy(temp, buf+13+curr, buf[12+curr]);
				curr += 3 + buf[12 + curr];

				wchar_t strUnicode[128] = { 0, };
				char strMultibyte[128] = { 0, };
				wcscpy_s(strUnicode, 128, (wchar_t*)temp);
				int len = WideCharToMultiByte(CP_ACP, 0, strUnicode, -1, NULL, 0, NULL, NULL);
				WideCharToMultiByte(CP_ACP, 0, strUnicode, -1, strMultibyte, len, NULL, NULL);

				m_List.SetItemText(i, 1, (char*)strMultibyte);
			}
		} else if (srv == 0x21) {
			Sleep(1000);	
			m_List.DeleteAllItems();
			unsigned char buff[32] = { 0xF7,0x0C,0x01,0x53,0x01,0x12,0x11,0x01,0x00,0x0A,0x00,0xEE };
			CalcCRC(buff);
			m_ComuPort.WriteComm((LPCTSTR)buff, buff[1]);
		} else if (srv == 0x23) {
			AfxMessageBox(msg);
		} else if (srv == 0x31) {
			Sleep(1000);
			m_List.DeleteAllItems();
			unsigned char buff[32] = { 0xF7,0x0C,0x01,0x53,0x01,0x12,0x11,0x01,0x00,0x0A,0x00,0xEE };
			CalcCRC(buff);
			m_ComuPort.WriteComm((LPCTSTR)buff, buff[1]);
		}
	}

	mFirst = false;
	return 0;
}

DWORD ThreadDisp(CCommTestDlg* pDlg)
{
	while (1)
	{
		if (pDlg->mFirst == true)
			pDlg->dispatch();
	}
	return TRUE;
}

afx_msg LRESULT CCommTestDlg::OnReceive(WPARAM wParam, LPARAM lParam)
{
	if (mFirst == false)
	{
		mFirst = true;
	}

	return 0;
}

void CCommTestDlg::OnLvnItemchangedRegist(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	
	POSITION pos;
	pos = m_List.GetFirstSelectedItemPosition();
	int i = m_List.GetNextSelectedItem(pos);

	CString name, idx;
	name = m_List.GetItemText(i, 1);
	idx = m_List.GetItemText(i, 0);
	mIdx = atoi(idx);
	mName.SetWindowTextA(name);
	*pResult = 0;
}

void CCommTestDlg::OnBnClickedBtnFeature()
{
	unsigned char FEATURE_LOOKUP[] = { 0xF7,0x0B,0x01,0x53,0x01,0x11,0x11,0x01,0x00,0x00,0xEE };
	CalcCRC(FEATURE_LOOKUP);
	m_ComuPort.WriteComm((LPCTSTR)FEATURE_LOOKUP, FEATURE_LOOKUP[1]);
}

int CCommTestDlg::CalcCRC(unsigned char* buf)
{
	int i;
	unsigned char crc;
	crc = buf[0];
	for (i = 1; i < buf[1]-2; ++i)
		crc ^= buf[i];
	buf[i] = crc;
	return 0;
}


void CCommTestDlg::OnBnClickedBtnNotice()
{
	CString num;
	unsigned char buff[32] = { 0xF7,0x00,0x01,0x53,0x02,0x23,0x11,0x00,0xFF, };

	mNotice.GetWindowTextA(num);

	if (num == "")
	{
		AfxMessageBox(_T("��ȣ�� �Է��� �ּ���"));
		return;
	}

	buff[9] = atoi(num);

	int total_len = 12;
	buff[1] = total_len;
	buff[total_len - 1] = 0xEE;
	CalcCRC(buff);

	m_ComuPort.WriteComm((LPCTSTR)buff, buff[1]);
}
