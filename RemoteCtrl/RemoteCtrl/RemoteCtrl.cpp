﻿// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <conio.h>
#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "TroubleTool.h"
#include "Command.h"
#include "CTroubleQueue.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
//#pragma comment(linker,"/subsystem:windows /entry:WinMainCRTStartup")
//#pragma comment(linker,"/subsystem:windows /entry:mainCRTStartup")
//#pragma comment(linker,"/subsystem:console /entry:mainCRTStartup")
//#pragma comment(linker,"/subsystem:console /entry:WinMainCRTStartup")
// 唯一的应用程序对象
//#define INVOKE_PATH _TCString(_T("C:\\Windows\\SysWOW64\\RemoteCtrl.exe"))
#define INVOKE_PATH _T("C:\\Users\\X1860\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe")

CWinApp theApp;
using namespace std;

//业务和通用
bool ChooseAutoInvoke(const CString& strPath) {
	TCHAR wcsSystem[MAX_PATH] = _T("");
	if (PathFileExists(strPath)) {
		return true;
	}
	CString strInfo = _T("该程序只允许用于合法的用途！\n");
	strInfo += _T("继续运行该程序，将使得这台机器处于被监控状态！\n");
	strInfo += _T("如果你不希望这样，请按“取消”按钮，退出程序。\n");
	strInfo += _T("按下“是”按钮，该程序将复制到你的机器上，并随系统启动而自动运行！\n");
	strInfo += _T("按下“否”按钮，程序只运行一次，不会在系统内留下任何东西！\n");
	int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
	if (ret == IDYES) {
		//WriteRegisterTable(strPath);
		if (!CTroubleTool::WriteStartupDir(strPath)) {
			MessageBox(NULL, _T("复制文件失败，是否权限不足？\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
	}
	else if (ret == IDCANCEL) {
		return false;
	}
	return true;
}

#define IOCP_LIST_EMPTY 0
#define IOCP_LIST_PUSH 1
#define IOCP_LIST_POP 2

enum {
	IocpListEmpty,
	IocpListPush,
	IocpListPop
};

typedef struct IocpParam {
	int nOperator;//操作
	std::string strData;//数据
	_beginthread_proc_type cbFunc;//回调
	IocpParam(int op, const char* sData, _beginthread_proc_type cb = NULL) {
		nOperator = op;
		strData = sData;
		cbFunc = cb;
	}
	IocpParam() {
		nOperator = -1;
	}
}IOCP_PARAM;

void threadmain(HANDLE hIOCP) {
	std::list<std::string> lstString;
	DWORD dwTransferred = 0;
	ULONG_PTR CompletionKey = 0;
	OVERLAPPED* pOverlapped = NULL;
	int count = 0, count0 = 0, total = 0;
	while (GetQueuedCompletionStatus(hIOCP, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE)) {
		if ((dwTransferred == 0) || (CompletionKey == NULL)) {
			printf("thread is prepare to exit!\r\n");
			break;
		}
		IOCP_PARAM* pParam = (IOCP_PARAM*)CompletionKey;
		if (pParam->nOperator == IocpListPush) {
			lstString.push_back(pParam->strData);
			printf("push size %d %p\r\n", lstString.size(), pOverlapped);
			count0++;
		}
		else if (pParam->nOperator == IocpListPop) {
			printf("%p size %d\r\n", pParam->cbFunc, lstString.size());
			std::string str;
			if (lstString.size() > 0) {
				str = lstString.front();
				lstString.pop_front();
			}
			if (pParam->cbFunc) {
				pParam->cbFunc(&str);
			}
			count++;
		}
		else if (pParam->nOperator == IocpListEmpty) {
			lstString.clear();
		}
		delete pParam;
		printf("total %d\r\n", ++total);
	}
	//lstString.clear();
	printf("thread exit count %d count0 %d\r\n", count, count0);
}

void threadQueueEntry(HANDLE hIOCP) {
	threadmain(hIOCP);
	_endthread();//代码到此为止，会导致本地对象无法调用析构，从而使得内存发生泄漏
}

void func(void* arg) {
	std::string* pstr = (std::string*)arg;
	if (pstr != NULL) {
		printf("pop from list:%s\r\n", pstr->c_str());
		//delete pstr;
	}
	else {
		printf("list is empty,no data!\r\n");
	}
}

void test() //性能测试
{//性能：CTroubleQueue push性能高 pop性能仅1/4
	//   list push性能比pop低
	CTroubleQueue<std::string> lstStrings;
	ULONGLONG tick0 = GetTickCount64(), tick = GetTickCount64(), total = GetTickCount64();
	while (GetTickCount64() - total <= 1000) {
		//if (GetTickCount64() - tick0 >= 5) 
		{
			lstStrings.PushBack("hello world");
			tick0 = GetTickCount64();
		}
		//Sleep(1);
	}
	size_t count = lstStrings.Size();
	printf("exit done!size %d\r\n",count);
	total = GetTickCount64();
	while (GetTickCount64() - total <= 1000) {//完成端口 把请求与实现 分离了
		//if (GetTickCount64() - tick >= 5) 
		{
			std::string str;
			lstStrings.PopFront(str);
			tick = GetTickCount64();
			//printf("pop from queue:%s\r\n", str.c_str());
		}
		//Sleep(1);

	}
	printf("exit done!size %d\r\n", count-lstStrings.Size());
	lstStrings.Clear();
	std::list<std::string> lstData;
	total = GetTickCount64();
	while (GetTickCount64() - total <= 1000) {
		lstData.push_back("hello world");
	}
	count = lstData.size();
	printf("lstData push done!size %d\r\n", lstData.size());
	total = GetTickCount64();
	while (GetTickCount64() - total <= 250) {
		if(lstData.size()>0)lstData.pop_front();
	}
	printf("lstData pop done!size %d\r\n", (count-lstData.size())*4);
}
/*
* 1.bug测试/功能测试
* 2.关键因素的测试（内存泄漏、运行的稳定性、条件性）
* 3.压力测试（可靠性测试）
* 4.性能测试
*/
int main()
{
	if (!CTroubleTool::Init())return 1;

	//printf("press any key to exit ...\r\n");
	for (int i = 0; i < 10; i++) {
		test();
	}


	/*
	if (CTroubleTool::IsAdmin()) {
		if (!CTroubleTool::Init())return 1;
		if (ChooseAutoInvoke(INVOKE_PATH)) {
			CCommand cmd;
			int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);
			switch (ret) {
			case -1:
				MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络状态！"), _T("网络初始化失败!"), MB_OK | MB_ICONERROR);
				break;
			case -2:
				MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
				break;
			}
		}
	}
	else {
		if (CTroubleTool::RunAsAdmin() == false) {
			CTroubleTool::ShowError();
			return 1;
		}
	}*/
	return 0;
}
