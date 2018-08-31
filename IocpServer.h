#pragma once
#include <vector>
#include <map>
#include <WinSock2.h>
#include <Windows.h>
#include <MSWSock.h>
#include "../PublicLibrary/UnknownEx.h"
#include "..\PublicLibrary\Macro.h"
#include "ClientContext.h"
class IOContext;
static const GUID GUID_OF(IocpServer) =
{ 0x54235d42, 0x24e4, 0x4e7f, { 0x89, 0x95, 0x73, 0xae, 0xe9, 0x3c, 0x5f, 0x5a } };
class IocpServer : public CUnknownEx
{
public:
	__QueryInterface;
	STATIC_CREATE(IocpServer);

	DEFINE_STRING(server_IP);
	DEFINE_VALUE(server_port, int);
	DEFINE_VALUE(max_accpet_request, int);
	HANDLE m_completion_port;
	SOCKET m_server_socket;
	LPFN_ACCEPTEX m_fn_acceptex;
	LPFN_GETACCEPTEXSOCKADDRS m_fn_get_acceptex_addr;

	typedef std::vector<HANDLE> HANDLE_VECTOR;
	typedef std::map<int, CPtrHelper<ClientContext>> CLIENT_MAP;
	HANDLE_VECTOR m_thread_handles;
	CLIENT_MAP m_client_map;
	CLock m_client_lock;
	IOContext m_accept_pool[10];
public:
	IocpServer();
	~IocpServer();
	void start(int port,int work_num);
	bool init_iocp(int work_num);
	bool init_listen_socket();
	void close_client(CPtrHelper<ClientContext> cc);
	void clear();
	bool post_accept(IOContext* ioc);
	bool do_accept(IOContext* ioc);
	bool post_recieve(CPtrHelper<ClientContext> cc, IOContext& ioc);
	bool on_recive(CPtrHelper<ClientContext> cc, IOContext* ioc,int len);
	void add_client(CPtrHelper<ClientContext> cc);
	void on_error(CPtrHelper<ClientContext> cc);
	CPtrHelper<ClientContext> find_client(int socket_ID);
	void remove_client(int socket_ID);
	static DWORD WINAPI WorkerThreadProc(LPVOID lpParam);
};