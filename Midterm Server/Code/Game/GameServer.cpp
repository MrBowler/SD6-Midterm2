#include "GameServer.hpp"


//-----------------------------------------------------------------------------------------------
void GameServer::Initalize()
{
	srand( (unsigned int) time( NULL ) );

	InitializeTime();
	m_server.StartServer( PORT_NUMBER );

	std::cout << "Server is up and running\n";
}


//-----------------------------------------------------------------------------------------------
void GameServer::Update()
{
	GetPackets();
}


//-----------------------------------------------------------------------------------------------
void GameServer::SendPacketToClient( const CS6Packet& pkt, const ClientInfo& info )
{
	struct sockaddr_in clientAddr;
	clientAddr.sin_family = AF_INET;
	clientAddr.sin_addr.s_addr = inet_addr( info.m_ipAddress );
	clientAddr.sin_port = info.m_portNumber;
	m_server.SendPacketToClient( (const char*) &pkt, sizeof( pkt ), clientAddr );
}


//-----------------------------------------------------------------------------------------------
void GameServer::SendPacketToAllClients( const CS6Packet& pkt )
{
	std::map< ClientInfo, Player* >::iterator playerIter;
	for( playerIter = m_players.begin(); playerIter != m_players.end(); ++playerIter )
	{
		SendPacketToClient( pkt, playerIter->first );
	}
}


//-----------------------------------------------------------------------------------------------
std::string GameServer::ConvertNumberToString( int number )
{
	return static_cast< std::ostringstream* >( &( std::ostringstream() << number ) )->str();
}


//-----------------------------------------------------------------------------------------------
Color3b GameServer::GetPlayerColorForID( unsigned int playerID )
{
	if( playerID == 0 )
		return Color3b( 255, 0, 0 );
	if( playerID == 1 )
		return Color3b( 0, 255, 0 );
	if( playerID == 2 )
		return Color3b( 0, 0, 255 );
	if( playerID == 3 )
		return Color3b( 255, 255, 0 );
	if( playerID == 4 )
		return Color3b( 255, 0, 255 );
	if( playerID == 5 )
		return Color3b( 0, 255, 255 );
	if( playerID == 6 )
		return Color3b( 255, 165, 0 );
	if( playerID == 7 )
		return Color3b( 128, 0, 128 );

	return Color3b( 255, 255, 255 );
}


//-----------------------------------------------------------------------------------------------
Vector2 GameServer::GetPlayerPosition( const Color3b& playerID )
{
	if( playerID == GetPlayerColorForID( 0 ) )
		return Vector2( 0.f, 0.f );
	if( playerID == GetPlayerColorForID( 1 ) )
		return Vector2( 0.f, MAP_SIZE_HEIGHT - 1.f );
	if( playerID == GetPlayerColorForID( 2 ) )
		return Vector2( MAP_SIZE_WIDTH - 1.f, MAP_SIZE_HEIGHT - 1.f );
	if( playerID == GetPlayerColorForID( 3 ) )
		return Vector2( MAP_SIZE_WIDTH - 1.f, 0.f );
	if( playerID == GetPlayerColorForID( 4 ) )
		return Vector2( 0.f, MAP_SIZE_HEIGHT * 0.5f );
	if( playerID == GetPlayerColorForID( 5 ) )
		return Vector2( MAP_SIZE_WIDTH - 1.f, MAP_SIZE_HEIGHT * 0.5f );
	if( playerID == GetPlayerColorForID( 6 ) )
		return Vector2( MAP_SIZE_WIDTH * 0.5f, 0.f );
	if( playerID == GetPlayerColorForID( 7 ) )
		return Vector2( MAP_SIZE_WIDTH * 0.5f, MAP_SIZE_HEIGHT - 1.f );

	return Vector2( MAP_SIZE_WIDTH * 0.5f, MAP_SIZE_HEIGHT * 0.5f );
}


//-----------------------------------------------------------------------------------------------
Vector2 GameServer::GetRandomPosition()
{
	Vector2 returnVec;
	returnVec.x = (float) ( rand() % MAP_SIZE_WIDTH );
	returnVec.y = (float) ( rand() % MAP_SIZE_HEIGHT );

	return returnVec;
}


//-----------------------------------------------------------------------------------------------
void GameServer::AddPlayer( const ClientInfo& info )
{
	Player* player;

	std::map< ClientInfo, Player* >::iterator playerIter = m_players.find( info );
	if( playerIter == m_players.end() )
	{
		player = new Player();
	}
	else
	{
		player = playerIter->second;
	}

	player->m_color = GetPlayerColorForID( m_players.size() );
	player->m_position = GetPlayerPosition( player->m_color );
	player->m_velocity = Vector2( 0.f, 0.f );
	player->m_orientationDegrees = 0.f;
	player->m_lastUpdateTime = GetCurrentTimeSeconds();

	m_players[ info ] = player;
}


//-----------------------------------------------------------------------------------------------
void GameServer::UpdatePlayer( const CS6Packet& updatePacket, const ClientInfo& info )
{
	std::map< ClientInfo, Player* >::iterator playerIter = m_players.find( info );
	if( playerIter != m_players.end() )
	{
		Player* player = playerIter->second;
		player->m_position.x = updatePacket.data.updated.xPosition;
		player->m_position.y = updatePacket.data.updated.yPosition;
		player->m_velocity.x = updatePacket.data.updated.xVelocity;
		player->m_velocity.y = updatePacket.data.updated.yVelocity;
		player->m_orientationDegrees = updatePacket.data.updated.yawDegrees;
	}
}


//-----------------------------------------------------------------------------------------------
void GameServer::SendUpdatesToClients()
{
	std::map< ClientInfo, Player* >::iterator playerIter;
	for( playerIter = m_players.begin(); playerIter != m_players.end(); ++playerIter )
	{
		Player* player = playerIter->second;
		CS6Packet updatePacket;
		updatePacket.packetType = TYPE_Update;
		updatePacket.playerColorAndID[0] = player->m_color.r;
		updatePacket.playerColorAndID[1] = player->m_color.g;
		updatePacket.playerColorAndID[2] = player->m_color.b;
		updatePacket.timestamp = GetCurrentTimeSeconds();
		updatePacket.data.updated.xPosition = player->m_position.x;
		updatePacket.data.updated.yPosition = player->m_position.y;
		updatePacket.data.updated.xVelocity = player->m_velocity.x;
		updatePacket.data.updated.yVelocity = player->m_velocity.y;
		updatePacket.data.updated.yawDegrees = player->m_orientationDegrees;

		SendPacketToAllClients( updatePacket );
	}
}


//-----------------------------------------------------------------------------------------------
void GameServer::GetPackets()
{
	CS6Packet pkt;
	struct sockaddr_in clientAddr;
	clientAddr.sin_family = AF_INET;
	int clientLen = sizeof( clientAddr );

	while( m_server.ReceivePacketFromClient( (char*) &pkt, sizeof( pkt ), clientAddr, clientLen ) )
	{
		ClientInfo info;
		info.m_ipAddress = inet_ntoa( clientAddr.sin_addr );
		info.m_portNumber = clientAddr.sin_port;
	}
}