#pragma once

enum IO_TYPE
{
	UNKNOW,				// ���ڳ�ʼ����������
	ACCEPT_POSTED,		// Ͷ��Accept����
	SEND_POSTED,		// Ͷ��Send����
	RECV_POSTED,		// Ͷ��Recv����
};

#define MAX_BUFFER_SIZE 4060		//���Ͻṹ����������������ճ�4K
// �ͷ�ָ��ĺ�
#define RELEASE(x)			{if(x != NULL) {delete x; x = NULL;}}
// �ͷž���ĺ�
#define RELEASE_HANDLE(x)	{if(x != NULL && x != INVALID_HANDLE_VALUE) { CloseHandle(x); x = INVALID_HANDLE_VALUE; }}
// �ͷ�Socket�ĺ�
#define RELEASE_SOCKET(x)	{if(x != INVALID_SOCKET) { closesocket(x); x = INVALID_SOCKET; }}