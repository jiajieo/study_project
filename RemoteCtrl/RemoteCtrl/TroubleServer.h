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
	TroubleClient* m_client;//对应的客户端
	WSABUF m_wsabuffer;
	virtual ~TroubleOverlapped() {
		m_buffer.clear();
	}
};
template<TroubleOperator>class AcceptOverlapped;
typedef AcceptOverlapped<EAccept> ACCEPTOVERLAPPED;
template<TroubleOperator>class RecvOverlapped;
typedef RecvOverlapped<ERecv> RECVOVERLAPPED;
template<TroubleOperator>class SendOverlapped;
typedef SendOverlapped<ESend> SENDOVERLAPPED;

class TroubleClient:public ThreadFuncBase{
public:
	TroubleClient();

	~TroubleClient() {
		m_buffer.clear();
		closesocket(m_sock);
		m_recv.reset();
		m_send.reset();
		m_overlapped.reset();
		m_vecSend.Clear();
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
	LPWSABUF RecvWSABuffer();
	LPWSAOVERLAPPED RecvOverlapped();
	LPWSABUF SendWSABuffer();
	LPWSAOVERLAPPED SendOverlapped();
	DWORD& flags() { return m_flags; }
	sockaddr_in* GetLocalAddr() { return &m_laddr; }
	sockaddr_in* GetRemoteAddr() { return &m_raddr; }
	size_t GetBufferSize()const { return m_buffer.size(); }
	int Recv();
	int Send(void* buffer, size_t nSize);
	int SendData(std::vector<char>& data);
private:
	SOCKET m_sock;
	DWORD m_recvived;
	DWORD m_flags;
	std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
	std::shared_ptr<RECVOVERLAPPED> m_recv;
	std::shared_ptr<SENDOVERLAPPED> m_send;
	std::vector<char> m_buffer;
	size_t m_used;//已经使用的缓冲区大小
	sockaddr_in m_laddr;
	sockaddr_in m_raddr;
	bool m_isbusy;
	TroubleSendQueue<std::vector<char>> m_vecSend;//发送数据队列
};

template<TroubleOperator>
class AcceptOverlapped :public TroubleOverlapped, ThreadFuncBase {
public:
	AcceptOverlapped();
	int AcceptWorker();
};


template<TroubleOperator>
class RecvOverlapped :public TroubleOverlapped, ThreadFuncBase {
public:
	RecvOverlapped();
	int RecvWorker() {
		int ret = m_client->Recv();
		return ret;
	}
	
};

template<TroubleOperator>
class SendOverlapped :public TroubleOverlapped, ThreadFuncBase {
public:
	SendOverlapped();
	int SendWorker() {
		//TODO:
		/*
		* 1 Send可能不会立即完成
		*/
		return -1;
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
		return -1;
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
	~TroubleServer();
	bool StartService();
	bool NewAccept();
	void BindNewSocket(SOCKET s);
private:
	void CreateSocket();
	int threadIocp();
private:
	TroubleThreadPool m_pool;
	HANDLE m_hIOCP;
	SOCKET m_sock;
	sockaddr_in m_addr;
	std::map<SOCKET, std::shared_ptr<TroubleClient>> m_client;
};

