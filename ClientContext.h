#pragma once
#include <WinSock2.h>
#include <Windows.h>
#include <MSWSock.h>
#include <vector>
#include <list>
#include "../PublicLibrary/MemoryPool.h"
#include "../PublicLibrary/Macro.h"
#include "IOContext.h"
static const GUID GUID_OF(ClientContext) =
{ 0x578e040a, 0x514, 0x4d97, { 0xab, 0x50, 0xaa, 0x83, 0xa9, 0x95, 0x2e, 0x62 } };
class ClientContext :public CCircularMemory
{
public:
	MEMORY_INTERFACE;
	POOL_CREATE(ClientContext);
	DEFINE_VALUE(ID, int);
	DEFINE_STRING(client_IP);
	DEFINE_VALUE(client_port, int);
	SOCKET m_socket;
	IOContext m_post_context;
	IOContext m_sending_pool[2];
	DEFINE_VALUE(send_index, int);
	DEFINE_VALUE(send_complete, bool);
	CLock m_sending_pool_lock;
public:
	ClientContext();
	~ClientContext();
	IOContext& get_post_context(){ return m_post_context; }
	void on_recieve(const char* buffer,int len);
	void write(const char* buffer,int len);
	void on_send_complete();
	void close();
private:
	void _post_send();

};