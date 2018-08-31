#pragma once

enum IO_TYPE
{
	UNKNOW,				// 用于初始化，无意义
	ACCEPT_POSTED,		// 投递Accept操作
	SEND_POSTED,		// 投递Send操作
	RECV_POSTED,		// 投递Recv操作
};

#define MAX_BUFFER_SIZE 4060		//加上结构体里面的其他变量凑成4K
// 释放指针的宏
#define RELEASE(x)			{if(x != NULL) {delete x; x = NULL;}}
// 释放句柄的宏
#define RELEASE_HANDLE(x)	{if(x != NULL && x != INVALID_HANDLE_VALUE) { CloseHandle(x); x = INVALID_HANDLE_VALUE; }}
// 释放Socket的宏
#define RELEASE_SOCKET(x)	{if(x != INVALID_SOCKET) { closesocket(x); x = INVALID_SOCKET; }}