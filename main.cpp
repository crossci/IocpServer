#include <iostream>
#include "IocpServer.h"
#include "../PublicLibrary/ConsoleCtrlHandler.h"
using namespace std;

int main(int argc, char* argv)
{
	CPtrHelper<IocpServer> pIocpServer = IocpServer::CreateInstance();
	if (pIocpServer)
	{
		pIocpServer->start(30000,4);
	}
	ConsoleCtrlHandler::WaitConsoleCtrlHandler();
	pIocpServer = NULL;
}