
// PinyinTestDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "PinyinTest.h"
#include "PinyinTestDlg.h"
#include "afxdialogex.h"
#include "../sqlite3/sqlite3.h"

#include <string.h>
#include <sphelper.h>
#include <sapi.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "sapi.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

ISpVoice *pVoice = NULL;

void MSSSpeak(LPCTSTR content)
{
	//ISpVoice *pVoice = NULL;
	//CoInitialize(NULL);
	//CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_INPROC_SERVER, IID_ISpVoice, (void**)&pVoice);
	pVoice->SetPriority(SPVPRI_ALERT);
	pVoice->SetRate(0);
	pVoice->SetVolume((USHORT)100);
	pVoice->Speak(content, SPF_ASYNC, NULL);
	//pVoice->Release();
	//::CoUninitialize();
}

sqlite3 *db;

static int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
	// 该函数作为sqlite3_exec的第三个参数
	// 每一条查询结果都执行此函数
	// 第一个参数为sqlite3_exec的第四个参数，可以用来修改外部数据
	// 第二个参数是查询结果的列数
	// 第三个参数是value，argv[i]为第i列的数据
	// 第四个参数是列名，azColName[i]为第i列的名称
	int i;
	for (i = 0; i < argc; i++)
	{
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}

static int selectWordFromDatabase(void *data, int argc, char **argv, char **azColName)
{
	// 因为sqlite3默认为utf8编码，拿到的数据要转换为Unicode编码
	CString ref = CA2W(argv[0], CP_UTF8);
	swprintf((WCHAR *)data, L"%s、", ref);

	return 0;
}

void createTestData()
{
	int rc;
	char *zErrMsg = 0;

	rc = sqlite3_exec(db, "create table py2ref(pinyin varchar(10), reference varchar(30));", 0, 0, &zErrMsg);
	rc = sqlite3_exec(db, CW2A(_T("insert into py2ref values('张', '张飞的张');"), CP_UTF8), 0, 0, &zErrMsg);
	rc = sqlite3_exec(db, CW2A(_T("insert into py2ref values('长', '增长的长');"), CP_UTF8), 0, 0, &zErrMsg);
	rc = sqlite3_exec(db, CW2A(_T("insert into py2ref values('涨', '涨潮的涨');"), CP_UTF8), 0, 0, &zErrMsg);
	rc = sqlite3_exec(db, CW2A(_T("insert into py2ref values('章', '文章的章');"), CP_UTF8), 0, 0, &zErrMsg);
	rc = sqlite3_exec(db, CW2A(_T("insert into py2ref values('帐', '帐篷的帐');"), CP_UTF8), 0, 0, &zErrMsg);
	if (rc != SQLITE_OK)
	{
		//fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
}

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CPinyinTestDlg 对话框



CPinyinTestDlg::CPinyinTestDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PINYINTEST_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CPinyinTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CPinyinTestDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_COPYDATA()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

_declspec(dllimport) void SetHook(HWND hwnd);

// CPinyinTestDlg 消息处理程序

BOOL CPinyinTestDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	// 安装钩子
	SetHook(m_hWnd);
	// 为pVoice创建com对象
	CoInitialize(NULL);
	CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_INPROC_SERVER, IID_ISpVoice, (void**)&pVoice);
	//	打开数据库
	int rc = sqlite3_open("wcp.db", &db);
	if (rc)
	{
		MSSSpeak(L"数据库打开错误");
	}
	//createTestData();		//建立一些数据用来测试

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CPinyinTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CPinyinTestDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CPinyinTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



BOOL CPinyinTestDlg::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	//SetDlgItemText(IDC_EDIT2, (LPWSTR)pCopyDataStruct->lpData);
	// 以上方法弃用，lpData在非hook加载进程中会有问题
	// 尚不清楚原因，故根据此消息使用没问题的剪贴板
	if ((ULONG_PTR)pCopyDataStruct->dwData == 8080)		// 8080为该消息来自拼音钩子的标识
	{
		if (OpenClipboard())
		{
			if (IsClipboardFormatAvailable(CF_UNICODETEXT))
			{
				HANDLE hClip;
				LPWSTR pBuf;

				hClip = GetClipboardData(CF_UNICODETEXT);
				pBuf = (LPWSTR)GlobalLock(hClip);		//获得候选列表
				EmptyClipboard();						//建议拿到候选列表后清空剪贴板
				GlobalUnlock(hClip);

				//SetDlgItemText(IDC_EDIT2, pBuf);

				WCHAR
					speakContent[256] = _T("\0"),
					candidateStr[64],
					sql[512];
				int
					start = 0, end = 0,
					j = 1,
					length = 0;

				while (0 != wcscmp(pBuf + start, L"\0"))
				{
					while (!iswblank(pBuf[end]))
					{
						end++;
					}
					if (start == end)
					{
						break;
					}
					// 拿到查询条件，即候选词
					for (int i = 0; start + i < end; i++)
					{
						swprintf_s(candidateStr + i, 64 - i, L"%c", pBuf[start + i]);
					}
					// 将候选词写入speakContent
					swprintf_s(speakContent + length, 256 - length, L"%d、%s、", j, candidateStr);
					j++;
					length = lstrlenW(speakContent);
					start = end + 1;
					end++;

					int rc;
					char *zErrMsg = 0;
					swprintf_s(sql, 512, L"select word from t_word where id in (select word_id from t_character_word where char_id in (select id from t_character where character='%s')) limit 1;", candidateStr);		// 构建sql语句

					// 查询数据库，并将提示信息写入speakContent
					rc = sqlite3_exec(db, CW2A(sql, CP_UTF8), selectWordFromDatabase, speakContent + length, &zErrMsg);		// sqlite3默认使用utf8编码，所以第二个参数需要将Unicode转为utf8
					if (rc != SQLITE_OK)
					{
						swprintf_s(speakContent + length, 256 - length, L"查询失败、");

						sqlite3_free(zErrMsg);
					}

					length = lstrlenW(speakContent);
				}
				SetDlgItemText(IDC_EDIT2, speakContent);
				MSSSpeak(speakContent);
			}
			CloseClipboard();
		}
	}

	return CDialogEx::OnCopyData(pWnd, pCopyDataStruct);
}

_declspec(dllimport) void Unhook();
void CPinyinTestDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: 在此处添加消息处理程序代码
	// 卸载钩子
	Unhook();
	// 清理pVoice
	::CoUninitialize();
	// 关闭数据库
	sqlite3_close(db);
}
