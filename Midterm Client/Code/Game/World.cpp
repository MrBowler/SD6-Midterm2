#include "World.hpp"
#include <queue>
#include "../Engine/Time.hpp"
#include "../Engine/DeveloperConsole.hpp"
#include "../Engine/NewMacroDef.hpp"

//-----------------------------------------------------------------------------------------------
World::World( float worldWidth, float worldHeight )
	: m_size( worldWidth, worldHeight )
	, m_playerTexture( nullptr )
	, m_isConnectedToServer( false )
	, m_nextPacketNumber( 0 )
{

}


//-----------------------------------------------------------------------------------------------
void World::Initialize()
{
	InitializeTime();
	m_client.ConnectToServer( IP_ADDRESS, PORT_NUMBER );
	m_playerTexture = Texture::CreateOrGetTexture( PLAYER_TEXTURE_FILE_PATH );
	m_secondsSinceLastInitSend = GetCurrentTimeSeconds();

	m_mainPlayer = new Player;
	m_players.push_back( m_mainPlayer );
}


//-----------------------------------------------------------------------------------------------
void World::Destruct()
{
	m_client.DisconnectFromServer();
}


//-----------------------------------------------------------------------------------------------
void World::ChangeIPAddress( const std::string& ipAddrString )
{
	unsigned short currentServerPortNumber = m_client.GetServerPortNumber();
	m_client.DisconnectFromServer();
	m_client.ConnectToServer( ipAddrString, currentServerPortNumber );

	m_isConnectedToServer = false;
}


//-----------------------------------------------------------------------------------------------
void World::ChangePortNumber( unsigned short portNumber )
{
	std::string currentServerIPAddress = m_client.GetServerIPAddress();
	m_client.DisconnectFromServer();
	m_client.ConnectToServer( currentServerIPAddress, portNumber );

	m_isConnectedToServer = false;
}


//-----------------------------------------------------------------------------------------------
void World::Update( float deltaSeconds, const Keyboard& keyboard, const Mouse& mouse )
{
	UpdateFromInput( keyboard, mouse, deltaSeconds );
	ReceivePackets();
	SendUpdate();
}


//-----------------------------------------------------------------------------------------------
void World::RenderObjects3D()
{
	
}


//-----------------------------------------------------------------------------------------------
void World::RenderObjects2D()
{
	RenderPlayers();
}


//-----------------------------------------------------------------------------------------------
void World::SendPacket( const CS6Packet& packet, bool requireAck )
{
	m_client.SendPacketToServer( (const char*) &packet, sizeof( packet ) );
	++m_nextPacketNumber;

	if( requireAck )
	{
		m_sentPackets.push_back( packet );
	}
}


//-----------------------------------------------------------------------------------------------
void World::SendJoinGamePacket()
{
	CS6Packet joinPacket;
	joinPacket.packetNumber = m_nextPacketNumber;
	joinPacket.packetType = TYPE_Acknowledge;
	joinPacket.timestamp = GetCurrentTimeSeconds();
	joinPacket.data.acknowledged.packetType = TYPE_Acknowledge;

	SendPacket( joinPacket, true );
}


//-----------------------------------------------------------------------------------------------
void World::ResetGame( const CS6Packet& resetPacket )
{
	m_isConnectedToServer = true;

	m_mainPlayer->m_color.r = resetPacket.data.reset.playerColorAndID[0];
	m_mainPlayer->m_color.g = resetPacket.data.reset.playerColorAndID[1];
	m_mainPlayer->m_color.b = resetPacket.data.reset.playerColorAndID[2];
	m_mainPlayer->m_currentPosition.x = resetPacket.data.reset.playerXPosition;
	m_mainPlayer->m_currentPosition.y = resetPacket.data.reset.playerYPosition;
	m_mainPlayer->m_gotoPosition = m_mainPlayer->m_currentPosition;
	m_mainPlayer->m_currentVelocity = Vector2( 0.f, 0.f );
	m_mainPlayer->m_orientationDegrees = 0.f;

	CS6Packet ackPacket;
	ackPacket.packetNumber = m_nextPacketNumber;
	ackPacket.packetType = TYPE_Acknowledge;
	ackPacket.playerColorAndID[0] = m_mainPlayer->m_color.r;
	ackPacket.playerColorAndID[1] = m_mainPlayer->m_color.g;
	ackPacket.playerColorAndID[2] = m_mainPlayer->m_color.b;
	ackPacket.timestamp = GetCurrentTimeSeconds();
	ackPacket.data.acknowledged.packetNumber = resetPacket.packetNumber;
	ackPacket.data.acknowledged.packetType = TYPE_Reset;

	SendPacket( ackPacket, false );
}


//-----------------------------------------------------------------------------------------------
void World::UpdatePlayer( const CS6Packet& updatePacket )
{
	for( unsigned int playerIndex = 0; playerIndex < m_players.size(); ++playerIndex )
	{
		Player* player = m_players[ playerIndex ];
		if( player->m_color.r == updatePacket.playerColorAndID[0]
		&& player->m_color.g == updatePacket.playerColorAndID[1]
		&& player->m_color.b == updatePacket.playerColorAndID[2] )
		{
			if( player == m_mainPlayer )
				return;

			player->m_lastUpdatePosition = player->m_currentPosition;
			player->m_currentPosition.x = updatePacket.data.updated.xPosition;
			player->m_currentPosition.y = updatePacket.data.updated.yPosition;
			player->m_currentVelocity.x = updatePacket.data.updated.xVelocity;
			player->m_currentVelocity.y = updatePacket.data.updated.yVelocity;
			player->m_orientationDegrees = updatePacket.data.updated.yawDegrees;
			player->m_secondsSinceLastUpdate = 0.f;

			return;
		}
	}

	Player* player = new Player();
	player->m_color.r = updatePacket.playerColorAndID[0];
	player->m_color.g = updatePacket.playerColorAndID[1];
	player->m_color.b = updatePacket.playerColorAndID[2];
	player->m_currentPosition.x = updatePacket.data.updated.xPosition;
	player->m_currentPosition.y = updatePacket.data.updated.yPosition;
	player->m_gotoPosition = player->m_currentPosition;
	player->m_currentVelocity.x = updatePacket.data.updated.xVelocity;
	player->m_currentVelocity.y = updatePacket.data.updated.yVelocity;
	player->m_orientationDegrees = updatePacket.data.updated.yawDegrees;
	player->m_secondsSinceLastUpdate = 0.f;

	m_players.push_back( player );
}


//-----------------------------------------------------------------------------------------------
void World::UpdateFromInput( const Keyboard& keyboard, const Mouse&, float deltaSeconds )
{
	if( g_developerConsole.m_drawConsole )
		return;

	Vector2 velocity;

	bool isEast = keyboard.IsKeyPressedDown( KEY_D );
	bool isNorth = keyboard.IsKeyPressedDown( KEY_W );
	bool isWest = keyboard.IsKeyPressedDown( KEY_A );
	bool isSouth = keyboard.IsKeyPressedDown( KEY_S );

	bool isNorthEast = isEast & isNorth;
	bool isNorthWest = isWest & isNorth;
	bool isSouthWest = isWest & isSouth;
	bool isSouthEast = isEast & isSouth;

	if( isNorthEast )
	{
		velocity = Vector2( 1.f, 1.f );
		m_mainPlayer->m_orientationDegrees = 45.f;
	}
	else if( isNorthWest )
	{
		velocity = Vector2( -1.f, 1.f );
		m_mainPlayer->m_orientationDegrees = 135.f;
	}
	else if( isSouthWest )
	{
		velocity = Vector2( -1.f, -1.f );
		m_mainPlayer->m_orientationDegrees = 225.f;
	}
	else if( isSouthEast )
	{
		velocity = Vector2( 1.f, -1.f );
		m_mainPlayer->m_orientationDegrees = 315.f;
	}
	else if( isEast )
	{
		velocity = Vector2( 1.f, 0.f );
		m_mainPlayer->m_orientationDegrees = 0.f;
	}
	else if( isNorth )
	{
		velocity = Vector2( 0.f, 1.f );
		m_mainPlayer->m_orientationDegrees = 90.f;
	}
	else if( isWest )
	{
		velocity = Vector2( -1.f, 0.f );
		m_mainPlayer->m_orientationDegrees = 180.f;
	}
	else if( isSouth )
	{
		velocity = Vector2( 0.f, -1.f );
		m_mainPlayer->m_orientationDegrees = 270.f;
	}

	velocity.Normalize();
	m_mainPlayer->m_currentVelocity = velocity * SPEED_PIXELS_PER_SECOND;

	m_mainPlayer->m_currentPosition = m_mainPlayer->m_currentPosition + m_mainPlayer->m_currentVelocity * deltaSeconds;
}


//-----------------------------------------------------------------------------------------------
void World::SendUpdate()
{
	if( !m_isConnectedToServer && ( m_secondsSinceLastInitSend > SECONDS_BEFORE_RESEND_INIT_PACKET ) )
	{
		SendJoinGamePacket();
		m_secondsSinceLastInitSend = GetCurrentTimeSeconds();
		return;
	}
	
	CS6Packet updatePacket;
	updatePacket.packetType = TYPE_Update;
	updatePacket.playerColorAndID[0] = m_mainPlayer->m_color.r;
	updatePacket.playerColorAndID[1] = m_mainPlayer->m_color.g;
	updatePacket.playerColorAndID[2] = m_mainPlayer->m_color.b;
	updatePacket.timestamp = GetCurrentTimeSeconds();
	updatePacket.data.updated.xPosition = m_mainPlayer->m_currentPosition.x;
	updatePacket.data.updated.yPosition = m_mainPlayer->m_currentPosition.y;
	updatePacket.data.updated.xVelocity = m_mainPlayer->m_currentVelocity.x;
	updatePacket.data.updated.yVelocity = m_mainPlayer->m_currentVelocity.y;
	updatePacket.data.updated.yawDegrees = m_mainPlayer->m_orientationDegrees;

	SendPacket( updatePacket, false );
}


//-----------------------------------------------------------------------------------------------
void World::ReceivePackets()
{
	CS6Packet packet;
	std::set< CS6Packet > recvPackets;

	while( m_client.ReceivePacketFromServer( (char*) &packet, sizeof( packet ) ) )
	{
		recvPackets.insert( packet );
	}

	std::set< CS6Packet >::iterator setIter;
	for( setIter = recvPackets.begin(); setIter != recvPackets.end(); ++setIter )
	{
		CS6Packet orderedPacket = *setIter;

		if( orderedPacket.packetType == TYPE_Update )
		{
			UpdatePlayer( orderedPacket );
		}
		else if( orderedPacket.packetType == TYPE_Reset )
		{
			ResetGame( orderedPacket );
		}
	}
}


//-----------------------------------------------------------------------------------------------
void World::RenderPlayers()
{
	OpenGLRenderer::EnableTexture2D();
	OpenGLRenderer::BindTexture2D( m_playerTexture->m_openglTextureID );

	for( unsigned int playerIndex = 0; playerIndex < m_players.size(); ++playerIndex )
	{
		Player* player = m_players[ playerIndex ];
		float colorR = player->m_color.r * ONE_OVER_TWO_HUNDRED_TWENTY_FIVE;
		float colorG = player->m_color.g * ONE_OVER_TWO_HUNDRED_TWENTY_FIVE;
		float colorB = player->m_color.b * ONE_OVER_TWO_HUNDRED_TWENTY_FIVE;

		OpenGLRenderer::PushMatrix();

		OpenGLRenderer::SetColor3f( colorR, colorG, colorB );
		OpenGLRenderer::Translatef( player->m_currentPosition.x, player->m_currentPosition.y, 0.f );
		OpenGLRenderer::Rotatef( -player->m_orientationDegrees, 0.f, 0.f, 1.f );

		OpenGLRenderer::BeginRender( QUADS );
		{
			OpenGLRenderer::SetTexCoords2f( 0.f, 1.f );
			OpenGLRenderer::SetVertex2f( -ONE_HALF_POINT_SIZE_PIXELS, -ONE_HALF_POINT_SIZE_PIXELS );

			OpenGLRenderer::SetTexCoords2f( 1.f, 1.f );
			OpenGLRenderer::SetVertex2f( ONE_HALF_POINT_SIZE_PIXELS, -ONE_HALF_POINT_SIZE_PIXELS );

			OpenGLRenderer::SetTexCoords2f( 1.f, 0.f );
			OpenGLRenderer::SetVertex2f( ONE_HALF_POINT_SIZE_PIXELS, ONE_HALF_POINT_SIZE_PIXELS );

			OpenGLRenderer::SetTexCoords2f( 0.f, 0.f );
			OpenGLRenderer::SetVertex2f( -ONE_HALF_POINT_SIZE_PIXELS, ONE_HALF_POINT_SIZE_PIXELS );
		}
		OpenGLRenderer::EndRender();

		OpenGLRenderer::PopMatrix();
	}

	OpenGLRenderer::BindTexture2D( 0 );
	OpenGLRenderer::DisableTexture2D();
}