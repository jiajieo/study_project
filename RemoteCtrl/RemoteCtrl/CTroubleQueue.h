#pragma once
#include "pch.h"
#include <atomic>
#include <list>
#include "TroubleThread.h"

template<class T>
class CTroubleQueue
{//线程安全的队列（利用IOCP实现）
public:
	enum {
		EQNone,
		EQPush,
		EQPop,
		EQSize,
		EQClear
	};
	typedef struct IocpParam {
		size_t nOperator;//操作
		T Data;//数据
		HANDLE hEvent;//pop操作需要的
		IocpParam(int op, const T& data, HANDLE hEve = NULL) {
			nOperator = op;
			Data = data;
			hEvent = hEve;
		}
		IocpParam() {
			nOperator = EQNone;
		}
	}PPARAM;//Post Parameter 用于投递信息的结构体
public:
	CTroubleQueue() {
		m_lock = false;
		m_hCompeletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
		m_hThread = INVALID_HANDLE_VALUE;
		if (m_hCompeletionPort != NULL) {
			m_hThread = (HANDLE)_beginthread(
				&CTroubleQueue<T>::threadEntry,
				0, this
			);
		}
	}
	virtual  ~CTroubleQueue() {
		if (m_lock)return;
		m_lock = true;
		PostQueuedCompletionStatus(m_hCompeletionPort, 0, NULL, NULL);
		WaitForSingleObject(m_hThread, INFINITE);
		if (m_hCompeletionPort != NULL) {
			HANDLE hTemp = m_hCompeletionPort;
			m_hCompeletionPort = NULL;
			CloseHandle(hTemp);
		}
	}
	bool PushBack(const T& data) {
		IocpParam* pParam = new IocpParam(EQPush, data);
		if (m_lock) {
			delete pParam;
			return false;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false)delete pParam;
		//printf("push back done %d %08p\r\n", ret, (void*)pParam);
		return ret;
	}
	virtual bool PopFront(T& data) {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam Param(EQPop, data, hEvent);
		if (m_lock) {
			if (hEvent)CloseHandle(hEvent);
			return false;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return false;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		if (ret) {
			data = Param.Data;
		}
		return ret;
	}
	size_t Size() {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam Param(EQSize, T(), hEvent);
		if (m_lock) {
			if (hEvent)CloseHandle(hEvent);
			return -1;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return -1;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		if (ret) {
			return Param.nOperator;
		}
		return -1;
	}
	bool Clear() {
		if (m_lock)return false;
		IocpParam* pParam = new IocpParam(EQClear, T());
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false)delete pParam;
		//printf("clear %08p\r\n", (void*)pParam);
		return ret;
	}
protected:
	static void threadEntry(void* arg) {
		CTroubleQueue<T>* thiz = (CTroubleQueue<T>*)arg;
		thiz->threadMain();
		_endthread();
	}
	virtual void DealParam(PPARAM* pParam) {
		switch (pParam->nOperator) {
		case EQPush:
			m_lstData.push_back(pParam->Data);
			delete pParam;
			//printf("delete %08p\r\n", (void*)pParam);
			break;
		case EQPop:
			if (m_lstData.size() > 0) {
				pParam->Data = m_lstData.front();
				m_lstData.pop_front();
			}
			if (pParam->hEvent != NULL)SetEvent(pParam->hEvent);
			break;
		case EQSize:
			pParam->nOperator = m_lstData.size();
			if (pParam->hEvent != NULL)SetEvent(pParam->hEvent);
			break;
		case EQClear:
			m_lstData.clear();
			delete pParam;
			//printf("delete %08p\r\n", (void*)pParam);
			break;
		default:
			OutputDebugStringA("unknown operator!\r\n");
			break;
		}
	}
	virtual void threadMain() {
		DWORD dwTransferred = 0;
		PPARAM* pParam = NULL;
		ULONG_PTR CompletionKey = 0;
		OVERLAPPED* pOverlapped = NULL;
		while (GetQueuedCompletionStatus(
			m_hCompeletionPort,
			&dwTransferred,
			&CompletionKey,
			&pOverlapped, INFINITE))
		{
			if ((dwTransferred == 0) || (CompletionKey == NULL)) {
				printf("thread is prepare to exit!\r\n");
				break;
			}
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}
		while (GetQueuedCompletionStatus(
			m_hCompeletionPort,
			&dwTransferred,
			&CompletionKey,
			&pOverlapped, 0)) {
			if ((dwTransferred == 0) || (CompletionKey == NULL)) {
				printf("thread is prepare to exit!\r\n");
				continue;
			}
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}
		HANDLE hTemp = m_hCompeletionPort;
		m_hCompeletionPort = NULL;
		CloseHandle(hTemp);
	}
protected:
	std::list<T> m_lstData;
	HANDLE m_hCompeletionPort;
	HANDLE m_hThread;
	std::atomic<bool> m_lock;//队列正在析构
};

template<class T>
class TroubleSendQueue :public CTroubleQueue<T>, public ThreadFuncBase
{
public:
	typedef int (ThreadFuncBase::* EDYCALLBACK)(T& data);
	TroubleSendQueue(ThreadFuncBase* obj, EDYCALLBACK callback)
		:CTroubleQueue<T>(), m_base(obj), m_callback(callback)
	{
		m_thread.Start();
		m_thread.UpdateWorker(::ThreadWorker(this, (FUNCTYPE)&TroubleSendQueue<T>::threadTick));
	}
	virtual ~TroubleSendQueue(){
		m_base = NULL;
		m_callback = NULL;
		m_thread.Stop();
	}
protected:
	virtual bool PopFront(T& data) {
		return false;
	}
	bool PopFront() {
		typename CTroubleQueue<T>::IocpParam* Param = new typename CTroubleQueue<T>::IocpParam(CTroubleQueue<T>::EQPop, T());
		if (CTroubleQueue<T>::m_lock) {
			delete Param;
			return false;
		}
		bool ret = PostQueuedCompletionStatus(CTroubleQueue<T>::m_hCompeletionPort, sizeof(*Param), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			delete Param;
			return false;
		}
		return ret;
	}
	int threadTick() {
		if (WaitForSingleObject(CTroubleQueue<T>::m_hThread, 0) != WAIT_TIMEOUT)
			return 0;
		if (CTroubleQueue<T>::m_lstData.size() > 0) {
			PopFront();
		}
		return 0;
	}
	virtual void DealParam(typename CTroubleQueue<T>::PPARAM* pParam) {
		switch (pParam->nOperator) {
		case CTroubleQueue<T>::EQPush:
			CTroubleQueue<T>::m_lstData.push_back(pParam->Data);
			delete pParam;
			//printf("delete %08p\r\n", (void*)pParam);
			break;
		case CTroubleQueue<T>::EQPop:
			if (CTroubleQueue<T>::m_lstData.size() > 0) {
				pParam->Data = CTroubleQueue<T>::m_lstData.front();
				if ((m_base->*m_callback)(pParam->Data) == 0)
					CTroubleQueue<T>::m_lstData.pop_front();
			}
			delete pParam;
			break;
		case CTroubleQueue<T>::EQSize:
			pParam->nOperator = CTroubleQueue<T>::m_lstData.size();
			if (pParam->hEvent != NULL)SetEvent(pParam->hEvent);
			break;
		case CTroubleQueue<T>::EQClear:
			CTroubleQueue<T>::m_lstData.clear();
			delete pParam;
			//printf("delete %08p\r\n", (void*)pParam);
			break;
		default:
			OutputDebugStringA("unknown operator!\r\n");
			break;
		}
	}
private:
	ThreadFuncBase* m_base;
	EDYCALLBACK m_callback;
	TroubleThread m_thread;
};

typedef TroubleSendQueue<std::vector<char>>::EDYCALLBACK SENDCALLBACK;