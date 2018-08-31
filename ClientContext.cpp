#include "ClientContext.h"
#include <iostream>
using namespace std;
bool ClientContext::QueryInterface(const GUID& guid, void **ppvObject)
{
	QUERYINTERFACE(ClientContext);
	IF_TRUE(QUERYINTERFACE_PARENT(CCircularMemory));
	return false;
}
ClientContext::ClientContext()
{
	set_send_index(0);
	set_send_complete(true);
	m_sending_pool[0].reset();
	m_sending_pool[1].reset();
}
ClientContext::~ClientContext()
{
	
}
void ClientContext::on_recieve(const char* buffer, int len)
{
	string buf(buffer,len);
	cout << buf << endl;
	write(buffer,len);
}
void ClientContext::write(const char* buffer, int len)
{
	m_sending_pool_lock.Lock();
	int write_index = (m_send_index + 1) % 2;
	m_sending_pool[write_index].write(buffer, len);
	if (get_send_complete())
	{
		_post_send();
	}
	m_sending_pool_lock.Unlock();
	
}
void ClientContext::on_send_complete()
{
	m_sending_pool_lock.Lock();
	set_send_complete(true);
	_post_send();
	m_sending_pool_lock.Unlock();
}
void ClientContext::close()
{
	if (m_socket != INVALID_SOCKET)
	{
		shutdown(m_socket, SD_BOTH);
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		m_sending_pool[0].reset();
		m_sending_pool[1].reset();
		m_post_context.reset();
	}
}
void ClientContext::_post_send()
{
	m_send_index = (m_send_index + 1) % 2;
	int count = m_sending_pool[m_send_index].get_count();
	if (count > 0)
	{
		set_send_complete(false);
		m_sending_pool[m_send_index].m_io_type = SEND_POSTED;
		m_sending_pool[m_send_index].m_socket = m_socket;
		DWORD dwBytes = 0, dwFlags = 0;
		m_sending_pool[m_send_index].m_wsa_buf.buf = m_sending_pool[m_send_index].m_buffer.GetBuffer();
		m_sending_pool[m_send_index].m_wsa_buf.len = count;
		if (::WSASend(m_sending_pool[m_send_index].m_socket, &m_sending_pool[m_send_index].m_wsa_buf, 1, &dwBytes, dwFlags, &m_sending_pool[m_send_index].m_overLapped, NULL) != NO_ERROR)
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				//todo close the socket;
			}
		}
		m_sending_pool[m_send_index].m_buffer.SetEmpty();
	}
}
