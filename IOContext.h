#pragma once
#include <WinSock2.h>
#include <Windows.h>
#include <MSWSock.h>
#include <list>
#include "../PublicLibrary/Macro.h"
#include "../PublicLibrary/UnknownEx.h"
#include "IocpDefine.h"
#include "../PublicLibrary/CircularBuffer.h"
class IOContext
{
public:
	//size 36
	WSAOVERLAPPED m_overLapped;
	SOCKET m_socket;
	WSABUF m_wsa_buf;
	IO_TYPE m_io_type;

	CCircularBuffer m_buffer;
public:
	IOContext()
	{
		reset();
	}
	~IOContext()
	{

	}
	void reset();
	void write(const char* buffer, int len)
	{
		m_buffer.Write(buffer,len);
	}
	int get_count()
	{
		return m_buffer.GetCount();
	}
};

inline void IOContext::reset()
{
	ZeroMemory(&m_overLapped, sizeof(m_overLapped));
	m_buffer.SetEmpty();
	m_socket = INVALID_SOCKET;
	m_wsa_buf.buf = m_buffer.GetBuffer();
	m_wsa_buf.len = m_buffer.GetSize();
	m_io_type = IO_TYPE::UNKNOW;
}
