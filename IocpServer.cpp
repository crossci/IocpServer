#include "IocpServer.h"
#include "IocpDefine.h"
#include "IOContext.h"
#include "ClientContext.h"
#pragma comment(lib, "WS2_32.lib")

bool IocpServer::QueryInterface(const GUID& guid, void **ppvObject)
{
	QUERYINTERFACE(IocpServer);
	IF_TRUE(QUERYINTERFACE_PARENT(CUnknownEx));
	return false;
}
IocpServer::IocpServer()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}
IocpServer::~IocpServer()
{
	clear();
}
void IocpServer::start(int port, int work_num)
{
	set_server_port(port);
	init_iocp(work_num);
	init_listen_socket();
}
bool IocpServer::init_iocp(int work_num)
{
	m_completion_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (m_completion_port == NULL)
	{
		return false;
	}
	HANDLE thread_handle = INVALID_HANDLE_VALUE;
	for (int i = 0; i < work_num; i++)
	{
		thread_handle = CreateThread(0, 0, WorkerThreadProc, (void *)this, 0, 0);
	}
}
bool IocpServer::init_listen_socket()
{
	m_server_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_server_socket == INVALID_SOCKET)
	{
		return false;
	}
	if (!CreateIoCompletionPort((HANDLE)m_server_socket, m_completion_port, 0, 0))
	{
		return false;
	}
	sockaddr_in server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(get_server_port());

	if (SOCKET_ERROR == bind(m_server_socket, (sockaddr *)&server_addr, sizeof(server_addr)))
	{
		return false;
	}
	if (SOCKET_ERROR == listen(m_server_socket, SOMAXCONN))
	{
		return false;
	}
	GUID guidAcceptEx = WSAID_ACCEPTEX;
	GUID guidGetAcceptSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;
	DWORD dwBytes = 0;
	if (SOCKET_ERROR == WSAIoctl(
		m_server_socket,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidAcceptEx,
		sizeof(guidAcceptEx),
		&m_fn_acceptex,
		sizeof(m_fn_acceptex),
		&dwBytes,
		NULL,
		NULL))
	{
		return false;
	}

	if (SOCKET_ERROR == WSAIoctl(
		m_server_socket,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidGetAcceptSockAddrs,
		sizeof(guidGetAcceptSockAddrs),
		&m_fn_get_acceptex_addr,
		sizeof(m_fn_get_acceptex_addr),
		&dwBytes,
		NULL,
		NULL))
	{
		return false;
	}
	int pre_accept_size = sizeof(m_accept_pool) / sizeof(m_accept_pool[0]);
	for (size_t i = 0; i < pre_accept_size; i++)
	{
		if (false == post_accept(&m_accept_pool[i]))
		{
			return false;
		}
	}
	return true;
}
void IocpServer::close_client(CPtrHelper<ClientContext> cc)
{
	cc->close();
	remove_client(cc->get_ID());
}
void IocpServer::clear()
{
	// 释放工作者线程句柄指针
	HANDLE_VECTOR::iterator it = m_thread_handles.begin();
	for (; it != m_thread_handles.end(); ++it)
	{
		RELEASE_HANDLE(*it);
	}
	m_thread_handles.clear();

	// 关闭IOCP句柄
	RELEASE_HANDLE(m_completion_port);

	// 关闭监听Socket
	RELEASE_SOCKET(m_server_socket);
	WSACleanup();
}
bool IocpServer::post_accept(IOContext* ioc)
{
	DWORD dwBytes = 0;
	ioc->m_io_type = ACCEPT_POSTED;
	ioc->m_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == ioc->m_socket)
	{
		return false;
	}
	ioc->m_wsa_buf.buf = ioc->m_buffer.GetBuffer();
	// 将接收缓冲置为0,令AcceptEx直接返回,防止拒绝服务攻击
	if (false == m_fn_acceptex(m_server_socket, ioc->m_socket, ioc->m_wsa_buf.buf, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &dwBytes, &ioc->m_overLapped))
	{
		if (WSA_IO_PENDING != WSAGetLastError())
		{
			return false;
		}
	}
	return true;
}
bool IocpServer::do_accept(IOContext* ioc)
{
	SOCKADDR_IN *clientAddr = NULL;
	SOCKADDR_IN *localAddr = NULL;
	int clientAddrLen, localAddrLen;
	clientAddrLen = localAddrLen = sizeof(SOCKADDR_IN);

	// 1. 获取地址信息 （GetAcceptExSockAddrs函数不仅可以获取地址信息，还可以顺便取出第一组数据）
	m_fn_get_acceptex_addr(ioc->m_wsa_buf.buf, 0, localAddrLen, clientAddrLen, (LPSOCKADDR *)&localAddr, &localAddrLen, (LPSOCKADDR *)&clientAddr, &clientAddrLen);

	// 2. 为新连接建立一个SocketContext 
	CPtrHelper<ClientContext> pClientContext = ClientContext::CreateInstance();
	if (!pClientContext)
		return false;
	
	pClientContext->m_socket = ioc->m_socket;
	pClientContext->set_ID(ioc->m_socket);
	const char* RemoteIP = (const char*)inet_ntoa(clientAddr->sin_addr);
	unsigned short RemotePort = ntohs(clientAddr->sin_port);
	pClientContext->set_client_IP(inet_ntoa(clientAddr->sin_addr));
	pClientContext->set_client_port(clientAddr->sin_port);
	add_client(pClientContext);
	// 3. 将listenSocketContext的IOContext 重置后继续投递AcceptEx
	ioc->reset();
	if (false == post_accept(ioc))
	{
		//todo 打印错误log
	}

	// 4. 将新socket和完成端口绑定
	if (NULL == CreateIoCompletionPort((HANDLE)pClientContext->m_socket, m_completion_port, (DWORD)(pClientContext.operator->()), 0))
	{
		DWORD dwErr = WSAGetLastError();
		if (dwErr != ERROR_INVALID_PARAMETER)
		{
			close_client(pClientContext);
			return false;
		}
	}

	// 并设置tcp_keepalive
	//tcp_keepalive alive_in;
	//tcp_keepalive alive_out;
	//alive_in.onoff = TRUE;
	//alive_in.keepalivetime = 1000 * 60;  // 60s  多长时间（ ms ）没有数据就开始 send 心跳包
	//alive_in.keepaliveinterval = 1000 * 10; //10s  每隔多长时间（ ms ） send 一个心跳包
	//unsigned long ulBytesReturn = 0;
	//if (SOCKET_ERROR == WSAIoctl(newSockContext->connSocket, SIO_KEEPALIVE_VALS, &alive_in, sizeof(alive_in), &alive_out, sizeof(alive_out), &ulBytesReturn, NULL, NULL))
	//{
	//	TRACE(L"WSAIoctl failed: %d/n", WSAGetLastError());
	//}

	// 5. 建立recv操作所需的ioContext，在新连接的socket上投递recv请求

	IOContext& newIoContext = pClientContext->get_post_context();
	// 投递recv请求
	if (false == post_recieve(pClientContext, newIoContext))
	{
		close_client(pClientContext);
		return false;
	}
	return true;
}
bool IocpServer::post_recieve(CPtrHelper<ClientContext> cc, IOContext& ioc)
{
	DWORD dwFlags = 0, dwBytes = 0;
	ioc.reset();
	ioc.m_io_type = RECV_POSTED;
	ioc.m_socket = cc->m_socket;
	int nBytesRecv = WSARecv(ioc.m_socket, &ioc.m_wsa_buf, 1, &dwBytes, &dwFlags, &ioc.m_overLapped, NULL);
	// 如果返回值错误，并且错误的代码并非是Pending的话，那就说明这个重叠请求失败了
	if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		close_client(cc);
		return false;
	}
	return true;
}
bool IocpServer::on_recive(CPtrHelper<ClientContext> cc, IOContext* ioc, int len)
{
	cc->on_recieve(ioc->m_wsa_buf.buf, len);
	if (!post_recieve(cc,*ioc))
	{
		close_client(cc);
		return false;
	}
	return true;
}
void IocpServer::add_client(CPtrHelper<ClientContext> cc)
{
	int socket_ID = cc->get_ID();
	m_client_lock.Lock();
	CLIENT_MAP::iterator it = m_client_map.find(socket_ID);
	if (it != m_client_map.end())
	{
		m_client_map.erase(socket_ID);
	}
	m_client_map.insert(std::make_pair(socket_ID,cc));
	m_client_lock.Unlock();
}
void IocpServer::on_error(CPtrHelper<ClientContext> cc)
{
	if (cc)
	{
		cc->close();
		remove_client(cc->get_ID());
	}
}
CPtrHelper<ClientContext> IocpServer::find_client(int socket_ID)
{
	CPtrHelper<ClientContext> ret = NULL;
	m_client_lock.Lock();
	CLIENT_MAP::iterator it = m_client_map.find(socket_ID);
	if (it != m_client_map.end())
	{
		ret = it->second;
	}
	m_client_lock.Unlock();
	return ret;
}
void IocpServer::remove_client(int socket_ID)
{
	m_client_lock.Lock();
	CLIENT_MAP::iterator it = m_client_map.find(socket_ID);
	if (it != m_client_map.end())
	{
		m_client_map.erase(socket_ID);
	}
	m_client_lock.Unlock();
}
DWORD IocpServer::WorkerThreadProc(LPVOID lpParam)
{
	CPtrHelper<IocpServer> pIS = IocpServer::CreateInstance();
	if (!pIS)
		return 0;
	OVERLAPPED *ol = NULL;
	DWORD dwBytes = 0;
	IOContext* ioContext = NULL;
	ClientContext* client_context = NULL;
	while (1)
	{
		BOOL bRet = GetQueuedCompletionStatus(pIS->m_completion_port, &dwBytes, (PULONG_PTR)&client_context, &ol, INFINITE);

		// 读取传入的参数
		ioContext = CONTAINING_RECORD(ol, IOContext, m_overLapped);

		// 收到退出标志
		if (-1 == (DWORD)client_context)
		{
			break;
		}

		if (!bRet)
		{
			DWORD dwErr = GetLastError();

			// 可能是客户端异常退出了(64)
			if (ERROR_NETNAME_DELETED == dwErr)
			{
				pIS->on_error(client_context);
				continue;
			}
			else
			{
				pIS->on_error(client_context);
				continue;
			}
		}
		else
		{
			// 判断是否有客户端断开
			if ((0 == dwBytes) && (RECV_POSTED == ioContext->m_io_type || SEND_POSTED == ioContext->m_io_type))
			{
				pIS->on_error(client_context);
				continue;
			}
			else
			{
				switch (ioContext->m_io_type)
				{
				case ACCEPT_POSTED:
					pIS->do_accept(ioContext);
					break;
				case RECV_POSTED:
					pIS->on_recive(client_context, ioContext, dwBytes);
					break;
				case SEND_POSTED:
					client_context->on_send_complete();
					break;
				default:
					break;
				}
			}
		}
	}
	return 0;
}