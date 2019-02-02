#include "IOCP.h"

int main()
{
	IOCP server;
	if (server.Initialize())
	{
		server.StartServer();
	}
	return 0;
}