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
#include "engine2/ivengineserver2.h"
#include <steam/steam_gameserver.h>
#include "utils/module.h"
#include "schemasystem/schemasystem.h"
#include "cs2_sdk/entity/cbaseplayercontroller.h"
#include "version_gen.h"

class GameSessionConfiguration_t { };

KHook::Virtual<IServerGameDLL, void, bool, bool, bool> gameFrameHook(&IServerGameDLL::GameFrame, &g_ServerListPlayersFix, nullptr, &ServerListPlayersFix::Hook_GameFrame);

#ifdef _WIN32
#define ROOTBIN "/bin/win64/"
#define GAMEBIN "/csgo/bin/win64/"
#else
#define ROOTBIN "/bin/linuxsteamrt64/"
#define GAMEBIN "/csgo/bin/linuxsteamrt64/"
#endif


ServerListPlayersFix g_ServerListPlayersFix;
ICvar *icvar = NULL;
IServerGameDLL* server = NULL;
IVEngineServer* engine = NULL;
IServerGameClients* gameclients = NULL;
CGameEntitySystem* g_pEntitySystem = nullptr;

PLUGIN_EXPOSE(ServerListPlayersFix, g_ServerListPlayersFix);
bool ServerListPlayersFix::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetServerFactory, g_pSource2Server, ISource2Server, SOURCE2SERVER_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetServerFactory, server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	GET_V_IFACE_ANY(GetServerFactory, gameclients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pSchemaSystem, ISchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetEngineFactory, g_pNetworkServerService, INetworkServerService, NETWORKSERVERSERVICE_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pGameResourceServiceServer, IGameResourceService, GAMERESOURCESERVICESERVER_INTERFACE_VERSION);

	g_SMAPI->AddListener( this, this );

	gameFrameHook.Add(server);

	g_pCVar = icvar;
	ConVar_Register( FCVAR_RELEASE | FCVAR_CLIENT_CAN_EXECUTE | FCVAR_GAMEDLL );

	return true;
}

bool ServerListPlayersFix::Unload(char *error, size_t maxlen)
{
	gameFrameHook.Remove(server);

	return true;
}

void ServerListPlayersFix::UpdatePlayers()
{
	auto gpGlobals = engine->GetServerGlobals();
	g_pEntitySystem = GameEntitySystem();

	if(!gpGlobals || !g_pEntitySystem || !SteamGameServer())
		return;

	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		auto steamId = engine->GetClientSteamID(CPlayerSlot(i));
		if (steamId)
		{
			auto controller = (CBasePlayerController*)g_pEntitySystem->GetEntityInstance(CEntityIndex(i+1));
			if(controller)
				SteamGameServer()->BUpdateUserData(*steamId, controller->GetPlayerName(), gameclients->GetPlayerScore(CPlayerSlot(i)));
		}
	}
}

KHook::Return<void> ServerListPlayersFix::Hook_GameFrame(IServerGameDLL* pThis, bool simulating, bool bFirstTick, bool bLastTick)
{
	static double g_flNextUpdate = 0.0;

	double curtime = Plat_FloatTime();
	if (curtime > g_flNextUpdate)
	{
		UpdatePlayers();
		
		g_flNextUpdate = curtime + 5.0;
	}

	return {KHook::Action::Ignore};
}

void ServerListPlayersFix::AllPluginsLoaded()
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
	return PLUGIN_LICENSE;
}

const char *ServerListPlayersFix::GetVersion()
{
	return PLUGIN_FULL_VERSION;
}

const char *ServerListPlayersFix::GetDate()
{
	return __DATE__;
}

const char *ServerListPlayersFix::GetLogTag()
{
	return PLUGIN_LOGTAG;
}

const char *ServerListPlayersFix::GetAuthor()
{
	return PLUGIN_AUTHOR;
}

const char *ServerListPlayersFix::GetDescription()
{
	return PLUGIN_DESCRIPTION;
}

const char *ServerListPlayersFix::GetName()
{
	return PLUGIN_DISPLAY_NAME;
}

const char *ServerListPlayersFix::GetURL()
{
	return PLUGIN_URL;
}
