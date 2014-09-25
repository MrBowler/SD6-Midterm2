#ifndef include_GameServer
#define include_GameServer
#pragma once

//-----------------------------------------------------------------------------------------------
#include <map>
#include <set>
#include <string>
#include <time.h>
#include <vector>
#include <sstream>
#include <iostream>
#include "Player.hpp"
#include "Color3b.hpp"
#include "CS6Packet.hpp"
#include "UDPServer.hpp"
#include "ClientInfo.hpp"
#include "../Engine/Time.hpp"


//-----------------------------------------------------------------------------------------------
const unsigned short PORT_NUMBER = 5000;
const int MAP_SIZE_WIDTH = 500;
const int MAP_SIZE_HEIGHT = 500;
const double SECONDS_BEFORE_SEND_UPDATE = 0.1;
const double SECONDS_BEFORE_RESEND_RELIABLE_PACKETS = 0.25;


//-----------------------------------------------------------------------------------------------
class GameServer
{
public:
	GameServer() {}
	void Initalize();
	void Update();

private:
	void SendPacketToClient( const CS6Packet& pkt, const ClientInfo& info, bool requireAck );
	void SendPacketToAllClients( const CS6Packet& pkt, bool requireAck );
	std::string ConvertNumberToString( int number );
	Color3b GetPlayerColorForID( unsigned int playerID );
	Vector2 GetPlayerPosition( const Color3b& playerID );
	Vector2 GetRandomPosition();
	Player* GetItPlayer();
	void ProcessAckPackets( const CS6Packet& ackPacket, const ClientInfo& info );
	void AddPlayer( const ClientInfo& info );
	void ResetGame( const CS6Packet& victoryPacket, const ClientInfo& info );
	void UpdatePlayer( const CS6Packet& updatePacket, const ClientInfo& info );
	void SendUpdatesToClients();
	void GetPackets();
	void ResendAckPackets();

	UDPServer											m_server;
	unsigned int										m_nextPacketNumber;
	double												m_lastUpdateTime;
	std::map< ClientInfo, Player* >						m_players;
	std::map< ClientInfo, std::vector< CS6Packet > >	m_sendPacketsPerClient;
};


#endif // include_GameServer