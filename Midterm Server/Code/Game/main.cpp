//-----------------------------------------------------------------------------------------------
#include "GameServer.hpp"


//-----------------------------------------------------------------------------------------------
bool g_isQuitting = false;
GameServer g_gameServer;


//-----------------------------------------------------------------------------------------------
int main( int, char* )
{
	g_gameServer.Initalize();

	while( !g_isQuitting )
	{
		g_gameServer.Update();
	}

	return 0;
}