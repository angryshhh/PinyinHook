// PinyinHookDll.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"

HHOOK g_hPinyin = NULL;
HINSTANCE g_hInst;

// 为g_hWnd开一个新节，保存安装进程的句柄，以避免多进程共享dll数据的问题
#pragma data_seg("MySec")
HWND g_hWnd = NULL;
#pragma data_seg()

//#pragma comment(linker, "/section:MySec,RWS")

// 拼音钩子
LRESULT CALLBACK PinyinProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	PMSG pmsg = (PMSG)lParam;
	if (nCode == HC_ACTION)
	{
		switch (pmsg->message)
		{
		case WM_IME_NOTIFY:
		{
			HIMC hIMC;
			HWND hWnd = pmsg->hwnd;
			DWORD dwSize;
			LPCANDIDATELIST canlist;
			hIMC = ImmGetContext(hWnd);
			dwSize = ImmGetCandidateList(hIMC, 0, NULL, 0);
			if (dwSize == 0)
			{
				return 1;
			}

			canlist = (LPCANDIDATELIST)operator new(dwSize);
			ImmGetCandidateList(hIMC, 0, canlist, dwSize);

			WCHAR list[1024] = L"";
			//wsprintf(list, L"%u", canlist->dwPageSize);	//先输入当前候选页个数，测试用
			for (DWORD i = 0; i < canlist->dwPageSize; i++)
			{
				WCHAR iStr[16] = L"";
				LPWSTR candidateStr = (LPWSTR)((BYTE *)canlist + canlist->dwOffset[i]);
				wcscat_s(list, 512, candidateStr);
			}

			if (OpenClipboard(NULL))
			{
				// 将候选列表放入剪贴板
				HANDLE hClip;		// 保存调用GlobalAlloc函数后分配的内存对象的句柄
				LPWSTR lpStr;
				EmptyClipboard();

				hClip = GlobalAlloc(GMEM_MOVEABLE, wcslen(list) * sizeof(WCHAR) * 2);//宽字符大小必须如此计算
				lpStr = (LPWSTR)GlobalLock(hClip);
				wcscpy_s(lpStr, wcslen(list) + 1, list);//必须+1
				GlobalUnlock(hClip);
				SetClipboardData(CF_UNICODETEXT, hClip);
				CloseClipboard();
			}

			// 组成copydata消息的数据
			// 但在加载hook进程之外的进程中，数据会出问题
			// 只会出现前三个候选，且第三个为乱码
			// 比如"啊 阿 吖 嗄 锕 "会出错为"啊 阿 "
			// 尚不清楚问题原因，但剪贴板无此问题
			// 故该消息只作为使用剪贴板的触发条件
			COPYDATASTRUCT cpd;
			cpd.dwData = 8080;	// 自己指定的8080，用来和其他应用的copydata消息区分
			//cpd.cbData = wcslen(list);
			//cpd.lpData = list;

			// 将数据通过消息发送， 同时也视为使用剪贴板的通知
			SendMessage(g_hWnd, WM_COPYDATA, NULL, (LPARAM)&cpd);

			//free(canlist);
			ImmReleaseContext(hWnd, hIMC);
		}
		break;
		default:
			break;
		}
	}

	return 1;
}

//安装拼音钩子过程的函数
void SetHook(HWND hwnd)
{
	g_hWnd = hwnd;

	g_hPinyin = SetWindowsHookEx(WH_GETMESSAGE, PinyinProc, g_hInst, 0);
}

//卸载拼音钩子
void Unhook()
{
	UnhookWindowsHookEx(g_hPinyin);
}
