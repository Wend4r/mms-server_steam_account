/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * ======================================================
 * Metamod: Server Steam Account plugin
 * Written by Wend4r.
 * ======================================================
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from 
 * the use of this software.
 */

#include <stdio.h>
#include "server_steam_account.h"
#include "iserver.h"

#include "steam/isteamgameserver.h"

SH_DECL_HOOK0_void(ISteamGameServer, LogOnAnonymous, SH_NOATTRIB, 0);

ServerSteamAccount g_aServerSteamAccount;
IVEngineServer *engine = NULL;
ICvar *icvar = NULL;

// Should only be called within the active game loop (i e map should be loaded and active)
// otherwise that'll be nullptr!
CGlobalVars *GetGameGlobals()
{
	INetworkGameServer *server = g_pNetworkServerService->GetIGameServer();

	if(!server)
		return nullptr;

	return g_pNetworkServerService->GetIGameServer()->GetGlobals();
}

PLUGIN_EXPOSE(ServerSteamAccount, g_aServerSteamAccount);
bool ServerSteamAccount::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);

	META_CONPRINTF("Starting \"%s\" plugin.\n", g_aServerSteamAccount.GetName());

	ISteamGameServer *pGameServer = SteamGameServer();

	bool bResult = pGameServer != NULL;

	if(bResult)
	{
		SH_ADD_HOOK_MEMFUNC(ISteamGameServer, LogOnAnonymous, pGameServer, this, &ServerSteamAccount::Hook_LogOnAnonymous, false);

		META_CONPRINTF( "All hooks started!\n" );

		g_pCVar = icvar;
		ConVar_Register( FCVAR_RELEASE | FCVAR_CLIENT_CAN_EXECUTE | FCVAR_GAMEDLL );
	}
	else
	{
		ismm->Format(error, maxlen, "Failed to get \"%s\" interface", STEAMGAMESERVER_INTERFACE_VERSION);
	}

	return bResult;
}

bool ServerSteamAccount::Unload(char *error, size_t maxlen)
{
	return true;
}

void ServerSteamAccount::AllPluginsLoaded()
{
	/* This is where we'd do stuff that relies on the mod or other plugins 
	 * being initialized (for example, cvars added and events registered).
	 */
}

bool ServerSteamAccount::Pause(char *error, size_t maxlen)
{
	return true;
}

bool ServerSteamAccount::Unpause(char *error, size_t maxlen)
{
	return true;
}

void ServerSteamAccount::Hook_LogOnAnonymous()
{
	META_CONPRINTF("%s", "ServerSteamAccount::Hook_LogOnAnonymous()\n");

	RETURN_META(MRES_HANDLED);
}

const char *ServerSteamAccount::GetLicense()
{
	return "Public Domain";
}

const char *ServerSteamAccount::GetVersion()
{
	return "1.0.0.0";
}

const char *ServerSteamAccount::GetDate()
{
	return __DATE__;
}

const char *ServerSteamAccount::GetLogTag()
{
	return "STEAM_ACCOUNT";
}

const char *ServerSteamAccount::GetAuthor()
{
	return "Wend4r";
}

const char *ServerSteamAccount::GetDescription()
{
	return "Plugin that authorize a game server";
}

const char *ServerSteamAccount::GetName()
{
	return "Server Steam Account";
}

const char *ServerSteamAccount::GetURL()
{
	return "Discord: wend4r, wend4r2";
}
