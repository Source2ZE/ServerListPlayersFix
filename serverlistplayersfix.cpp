/**
 * =============================================================================
 * ServerListPlayersFix
 * Copyright (C) 2024 Poggu
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include "serverlistplayersfix.h"
#include <iserver.h>
#include <steam/steam_gameserver.h>
#include "utils/module.h"
#include "schemasystem/schemasystem.h"
#include <funchook.h>
#include "cs2_sdk/entity/cbaseplayercontroller.h"

class GameSessionConfiguration_t { };

SH_DECL_HOOK3_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool, bool, bool);
SH_DECL_HOOK0_void(IServerGameDLL, GameServerSteamAPIActivated, SH_NOATTRIB, 0);
SH_DECL_HOOK0_void(IServerGameDLL, GameServerSteamAPIDeactivated, SH_NOATTRIB, 0);

#ifdef _WIN32
#define ROOTBIN "/bin/win64/"
#define GAMEBIN "/csgo/bin/win64/"
#else
#define ROOTBIN "/bin/linuxsteamrt64/"
#define GAMEBIN "/csgo/bin/linuxsteamrt64/"
#endif


ServerListPlayersFix g_ServerListPlayersFix;
ICvar *icvar = NULL;
CSteamGameServerAPIContext g_steamAPI;
IServerGameDLL* server = NULL;
IVEngineServer* engine = NULL;
IServerGameClients* gameclients = NULL;
CSchemaSystem* g_pSchemaSystem2 = nullptr;
CGameEntitySystem* g_pEntitySystem = nullptr;

CGlobalVars* GetGameGlobals()
{
	INetworkGameServer* server = g_pNetworkServerService->GetIGameServer();

	if (!server)
		return nullptr;

	return g_pNetworkServerService->GetIGameServer()->GetGlobals();
}

CGameEntitySystem* GameEntitySystem()
{
#ifdef WIN32
	static int offset = 88;
#else
	static int offset = 80;
#endif
	return *reinterpret_cast<CGameEntitySystem**>((uintptr_t)(g_pGameResourceServiceServer)+offset);
}


PLUGIN_EXPOSE(ServerListPlayersFix, g_ServerListPlayersFix);
bool ServerListPlayersFix::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetServerFactory, g_pSource2Server, ISource2Server, SOURCE2SERVER_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetServerFactory, server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	GET_V_IFACE_ANY(GetServerFactory, gameclients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pSchemaSystem2, CSchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetEngineFactory, g_pNetworkServerService, INetworkServerService, NETWORKSERVERSERVICE_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pGameResourceServiceServer, IGameResourceService, GAMERESOURCESERVICESERVER_INTERFACE_VERSION);

	g_SMAPI->AddListener( this, this );

	SH_ADD_HOOK(IServerGameDLL, GameFrame, server, SH_MEMBER(this, &ServerListPlayersFix::Hook_GameFrame), true);
	SH_ADD_HOOK(IServerGameDLL, GameServerSteamAPIActivated, g_pSource2Server, SH_MEMBER(this, &ServerListPlayersFix::Hook_GameServerSteamAPIActivated), false);
	SH_ADD_HOOK(IServerGameDLL, GameServerSteamAPIDeactivated, g_pSource2Server, SH_MEMBER(this, &ServerListPlayersFix::Hook_GameServerSteamAPIDeactivated), false);

	g_pCVar = icvar;
	ConVar_Register( FCVAR_RELEASE | FCVAR_CLIENT_CAN_EXECUTE | FCVAR_GAMEDLL );

	return true;
}

bool ServerListPlayersFix::Unload(char *error, size_t maxlen)
{
	SH_REMOVE_HOOK(IServerGameDLL, GameFrame, server, SH_MEMBER(this, &ServerListPlayersFix::Hook_GameFrame), true);
	SH_REMOVE_HOOK(IServerGameDLL, GameServerSteamAPIActivated, g_pSource2Server, SH_MEMBER(this, &ServerListPlayersFix::Hook_GameServerSteamAPIActivated), false);
	SH_REMOVE_HOOK(IServerGameDLL, GameServerSteamAPIDeactivated, g_pSource2Server, SH_MEMBER(this, &ServerListPlayersFix::Hook_GameServerSteamAPIDeactivated), false);

	return true;
}

void ServerListPlayersFix::UpdatePlayers()
{
	auto gpGlobals = GetGameGlobals();
	g_pEntitySystem = GameEntitySystem();

	if(!gpGlobals)
		return;

	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		auto steamId = engine->GetClientSteamID(CPlayerSlot(i));
		if (steamId)
		{
			auto controller = (CBasePlayerController*)g_pEntitySystem->GetBaseEntity(CEntityIndex(i+1));
			if(controller)
				g_steamAPI.SteamGameServer()->BUpdateUserData(*steamId, controller->GetPlayerName(), gameclients->GetPlayerScore(CPlayerSlot(i)));
		}
	}
}

void ServerListPlayersFix::Hook_GameFrame(bool simulating, bool bFirstTick, bool bLastTick)
{
	static double g_flNextUpdate = 0.0;

	double curtime = Plat_FloatTime();
	if (curtime > g_flNextUpdate)
	{
		UpdatePlayers();
		
		g_flNextUpdate = curtime + 5.0;
	}

}

void ServerListPlayersFix::AllPluginsLoaded()
{
}

void ServerListPlayersFix::Hook_GameServerSteamAPIActivated()
{
	g_steamAPI.Init();
}

void ServerListPlayersFix::Hook_GameServerSteamAPIDeactivated()
{
}

void ServerListPlayersFix::OnLevelInit(char const* pMapName,
	char const* pMapEntities,
	char const* pOldLevel,
	char const* pLandmarkName,
	bool loadGame,
	bool background)
{
}

void ServerListPlayersFix::OnLevelShutdown()
{
}

bool ServerListPlayersFix::Pause(char *error, size_t maxlen)
{
	return true;
}

bool ServerListPlayersFix::Unpause(char *error, size_t maxlen)
{
	return true;
}

const char *ServerListPlayersFix::GetLicense()
{
	return "GPLv3";
}

const char *ServerListPlayersFix::GetVersion()
{
	return "1.0.1";
}

const char *ServerListPlayersFix::GetDate()
{
	return __DATE__;
}

const char *ServerListPlayersFix::GetLogTag()
{
	return "ServerListPlayersFix";
}

const char *ServerListPlayersFix::GetAuthor()
{
	return "Poggu";
}

const char *ServerListPlayersFix::GetDescription()
{
	return "Populates user information in the steam api";
}

const char *ServerListPlayersFix::GetName()
{
	return "ServerListPlayersFix";
}

const char *ServerListPlayersFix::GetURL()
{
	return "https://poggu.me";
}
