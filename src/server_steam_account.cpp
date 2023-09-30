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
#include <stdint.h>

#include "server_steam_account.h"
#include "pattern/finder.h"

#include <sh_memory.h>
#include <tier0/icommandline.h>
#include <steam/steam_gameserver.h>

SH_DECL_HOOK0_void(IServerGameDLL, GameServerSteamAPIActivated, SH_NOATTRIB, 0);
SH_DECL_HOOK0_void(IServerGameDLL, GameServerSteamAPIDeactivated, SH_NOATTRIB, 0);

ServerSteamAccount g_aServerSteamAccount;
IVEngineServer *engine = NULL;
ICvar *icvar = NULL;
IServerGameDLL *server = NULL;

PatternFinder g_aEnginePattern;

PLUGIN_EXPOSE(ServerSteamAccount, g_aServerSteamAccount);
bool ServerSteamAccount::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	META_CONPRINTF("Starting \"%s\" plugin.\n", g_aServerSteamAccount.GetName());

	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetServerFactory, server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);

	if(!g_aEnginePattern.SetupLibrary(engine))
	{
		g_SMAPI->Format(error, maxlen, "Failed to get server library pattern");

		return false;
	}

	{
		static const char szSteamGameServerInitName[] = "SteamGameServer_Init";

	#if defined(_WINDOWS) && defined(X64BITS)
		static const char sSteamGameServerInitSignature[] = "\x40\x53\x55\x56\x48\x81\xEC\xE0\x06\x00\x00";

		void *pSteamGameServerInitFunction = g_aEnginePattern.Find(sSteamGameServerInitSignature, sizeof(sSteamGameServerInitSignature) - 1);

		if(pSteamGameServerInitFunction)
		{
			uint8_t *pLogOnCallAddress = (uint8_t *)pSteamGameServerInitFunction + 0x3F9;

			SourceHook::SetMemAccess(pLogOnCallAddress, 3, SH_MEM_READ | SH_MEM_WRITE | SH_MEM_EXEC);

			// NOP DWORD ptr [EAX]
			*(uint16_t *)(pLogOnCallAddress) = 0x1F0F;
			*(pLogOnCallAddress + 2) = 0x00;
		}
		else
		{
			g_SMAPI->Format(error, maxlen, "Failed to get \"%s\" signature", szSteamGameServerInitName);

			return false;
		}
	#elif defined(_LINUX) && defined(X64BITS)
		static const uint8_t sSteamGameServerInitSignature[] = "\x55\x48\x89\xE5\x41\x57\x41\x56\x41\x55\x41\x89\xF5\x41\x54\x49\x89\xFC\x53\x48\x81\xEC\x38\x24\x00\x00";

		void *pSteamGameServerInitFunction = g_aEnginePattern.Find(sSteamGameServerInitSignature, sizeof(sSteamGameServerInitSignature) - 1);

		if(pSteamGameServerInitFunction)
		{
			uint8_t *pLogOnCallAddress = (uint8_t *)pSteamGameServerInitFunction + 0x31C;

			SourceHook::SetMemAccess(pLogOnCallAddress, 3, SH_MEM_READ | SH_MEM_WRITE | SH_MEM_EXEC);

			// NOP DWORD ptr [EAX]
			*(uint16_t *)(pLogOnCallAddress) = 0x1F0F;
			*(pLogOnCallAddress + 2) = 0x00;
		}
		else
		{
			g_SMAPI->Format(error, maxlen, "Failed to get \"%s\" signature", szSteamGameServerInitName);

			return false;
		}
	#else
	#	error Unsupported platform
	#endif
	}

	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, GameServerSteamAPIActivated, server, this, &ServerSteamAccount::OnGameServerSteamAPIActivated, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, GameServerSteamAPIDeactivated, server, this, &ServerSteamAccount::OnGameServerSteamAPIDeactivated, true);

	ISteamGameServer *pGameServer = SteamGameServer();

	if(pGameServer)
	{
		bool bIsLoggedOn = pGameServer->BLoggedOn();

		if(bIsLoggedOn && pGameServer->GetSteamID().BAnonGameServerAccount())
		{
			this->ProcessCommandLineAccount();
			pGameServer->LogOff();
			this->AuthorizeGameServer(pGameServer);
		}
	}

	g_pCVar = icvar;
	ConVar_Register(FCVAR_RELEASE | FCVAR_GAMEDLL);

	return true;
}

bool ServerSteamAccount::Unload(char *error, size_t maxlen)
{
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, GameServerSteamAPIActivated, server, this, &ServerSteamAccount::OnGameServerSteamAPIActivated, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, GameServerSteamAPIActivated, server, this, &ServerSteamAccount::OnGameServerSteamAPIDeactivated, true);

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

void ServerSteamAccount::OnGameServerSteamAPIActivated()
{
	ISteamGameServer *pGameServer = this->m_pSteamGameServer = SteamGameServer();

	if(pGameServer)
	{
		this->AuthorizeGameServer(pGameServer);
	}
	else
	{
		g_SMAPI->LogMsg(g_PLAPI, "ERROR: Failed to get \"%s\" interface", STEAMGAMESERVER_INTERFACE_VERSION);
	}
}

void ServerSteamAccount::OnGameServerSteamAPIDeactivated()
{
	this->m_pSteamGameServer = nullptr;
}

void ServerSteamAccount::OnSetSteamAccountCommand(const CCommandContext &context, const CCommand &args)
{
	{
		ISteamGameServer *pGameServer = SteamGameServer();

		if(pGameServer && pGameServer->BLoggedOn())
		{
			META_CONPRINT("Warning: Game server already logged into steam.  You need to use the sv_setsteamaccount command earlier.\n");

			return;
		}
	}

	if(args.ArgC() != 2)
	{
		META_CONPRINT("Usage: sv_setsteamaccount <login_token>\n");

		return;
	}

	bool bDoAuthorize = false;

	ISteamGameServer *pGameServer = SteamGameServer();

	if(this->m_sAccount[0] != '\0')
	{
		if(strcmp((const char *)this->m_sAccount, args[1])) // Is different.
		{
			bDoAuthorize = pGameServer != nullptr;
		}
	}

	strncpy((char *)this->m_sAccount, args[1], sizeof(this->m_sAccount));

	if(bDoAuthorize)
	{
		pGameServer->LogOff();
		this->AuthorizeGameServer(pGameServer);
	}
}

bool ServerSteamAccount::ProcessCommandLineAccount()
{
	const char *pszSetSteamAccount = CommandLine()->ParmValue("+sv_setsteamaccount", "\0");

	bool bResult = pszSetSteamAccount[0] != '\0';

	if(bResult)
	{
		strncpy((char *)this->m_sAccount, pszSetSteamAccount, sizeof(this->m_sAccount));
	}

	return bResult;
}

bool ServerSteamAccount::AuthorizeGameServer(ISteamGameServer *pGameServer)
{
	bool bResult = this->m_sAccount[0] != '\0';

	if(bResult)
	{
		META_CONPRINTF("Logging into Steam gameserver account with logon token '%.8sxxxxxxxxxxxxxxxxxxxxxxxx'\n", (const char *)m_sAccount);
		pGameServer->LogOn((const char *)this->m_sAccount);
	}
	else
	{
		if(engine->IsDedicatedServer())
		{
			Warning( "****************************************************\n" );
			Warning( "*                                                  *\n" );
			Warning( "*  No Steam account token was specified.           *\n" );
			Warning( "*  Logging into anonymous game server account.     *\n" );
			Warning( "*  Connections will be restricted to LAN only.     *\n" );
			Warning( "*                                                  *\n" );
			Warning( "*  To create a game server account go to           *\n" );
			Warning( "*  http://steamcommunity.com/dev/managegameservers *\n" );
			Warning( "*                                                  *\n" );
			Warning( "****************************************************\n" );
		}
		else
		{
			Msg( "Logging into anonymous listen server account.\n" );
		}

		SteamGameServer()->LogOnAnonymous();
	}

	return bResult;
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
