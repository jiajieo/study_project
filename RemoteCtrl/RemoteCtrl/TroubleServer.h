#pragma once
#include <MSWSock.h>
#include "TroubleThread.h"
#include "CTroubleQueue.h"
#include <map>



enum TroubleOperator {
	ENone,
	EAccept,
	ERecv,
	ESend,
	EError
};

class TroubleServer;
class TroubleClient;
typedef std::shared_ptr<TroubleClient> PCLIENT;

class TroubleOverlapped {
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator;//操作 参见TroubleOperator
	std::vector<char> m_buffer;//缓冲区
	ThreadWorker m_worker;//处理函数
	TroubleServer* m_server;//服务器对象
};
template<TroubleOperator>class AcceptOverlapped;
typedef AcceptOverlapped<EAccept> ACCEPTOVERLAPPED;

class TroubleClient {
public:
	TroubleClient();

	~TroubleClient() {
		closesocket(m_sock);
	}

	void SetOverlapped(PCLIENT& ptr);

	operator SOCKET() {
		return m_sock;
	}
	operator PVOID() {
		return &m_buffer[0];
	}
	operator LPOVERLAPPED();

	operator LPDWORD() {
		return &m_recvived;
	}
	sockaddr_in* GetLocalAddr() { return &m_laddr; }
	sockaddr_in* GetRemoteAddr() { return &m_raddr; }
private:
	SOCKET m_sock;
	DWORD m_recvived;
	std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
	std::vector<char> m_buffer;
	sockaddr_in m_laddr;
	sockaddr_in m_raddr;
	bool m_isbusy;
};

template<TroubleOperator>
class AcceptOverlapped :public TroubleOverlapped, ThreadFuncBase {
public:
	AcceptOverlapped();
	int AcceptWorker();
	PCLIENT m_client;
};


template<TroubleOperator>
class RecvOverlapped :public TroubleOverlapped, ThreadFuncBase {
public:
	RecvOverlapped() :m_operator(ERecv), m_worker(this, &RecvOverlapped::RecvWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024 * 256);
	}
	int RecvWorker() {
		//TODO:
	}
};
typedef RecvOverlapped<ERecv> RECVOVERLAPPED;

template<TroubleOperator>
class SendOverlapped :public TroubleOverlapped, ThreadFuncBase {
public:
	SendOverlapped() :m_operator(ESend), m_worker(this, &SendOverlapped::SendWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024 * 256);
	}
	int SendWorker() {
		//TODO:
	}
};
typedef SendOverlapped<ESend> SENDOVERLAPPED;

template<TroubleOperator>
class ErrorOverlapped :public TroubleOverlapped, ThreadFuncBase {
public:
	ErrorOverlapped() :m_operator(EError), m_worker(this, &ErrorOverlapped::ErrorWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024);
	}
	int ErrorWorker() {
		//TODO:
	}
};
typedef ErrorOverlapped<EError> ERROROVERLAPPED;





class TroubleServer :
	public ThreadFuncBase
{
public:
	TroubleServer(const std::string& ip = "0.0.0.0", short port = 9527) :m_pool(10) {
		m_hIOCP = INVALID_HANDLE_VALUE;
		m_sock = INVALID_SOCKET;
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(port);
		m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
	}
	~TroubleServer() {}
	bool StartService() {
		CreateSocket();
		if (bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == -1) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			return false;
		}
		if (listen(m_sock, 3) == -1) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			return false;
		}
		m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
		if (m_hIOCP == NULL) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return false;
		}
		CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0);
		m_pool.Invoke();
		m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&TroubleServer::threadIocp));
		if (!NewAccept())return false;
		//m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&TroubleServer::threadIocp));
		//m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&TroubleServer::threadIocp));
		return true;
	}
	bool NewAccept() {
		PCLIENT pClient(new TroubleClient());
		pClient->SetOverlapped(pClient);
		m_client.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient));
		if (!AcceptEx(m_sock,
			*pClient,
			*pClient,
			0,
			sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			*pClient, *pClient))
		{
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return false;
		}
		return true;
	}
private:
	void CreateSocket() {
		m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		int opt = 1;
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
	}
	int threadIocp() {
		DWORD tranferred = 0;
		ULONG_PTR CompletionKey = 0;
		OVERLAPPED* lpOverlapped = NULL;
		if (GetQueuedCompletionStatus(m_hIOCP, &tranferred, &CompletionKey, &lpOverlapped, INFINITE)) {
			if (tranferred > 0 && (CompletionKey != 0)) {
				TroubleOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, TroubleOverlapped, m_overlapped);
				switch (pOverlapped->m_operator) {
				case EAccept:
				{
					ACCEPTOVERLAPPED* pOver = (ACCEPTOVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
				}
				break;
				case ERecv:
				{
					RECVOVERLAPPED* pOver = (RECVOVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
				}
				break;
				case ESend:
				{
					SENDOVERLAPPED* pOver = (SENDOVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
				}
				break;
				case EError:
				{
					ERROROVERLAPPED* pOver = (ERROROVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
				}
				break;
				}
			}
			else {
				return -1;
			}
		}
		return 0;
	}
private:
	TroubleThreadPool m_pool;
	HANDLE m_hIOCP;
	SOCKET m_sock;
	sockaddr_in m_addr;
	std::map<SOCKET, std::shared_ptr<TroubleClient>> m_client;
};

