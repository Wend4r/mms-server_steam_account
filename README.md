## Server Steam Account

 MetaMod:Source plugin for Source 2 that authorizes a game server account by `+sv_setsteamaccount` command line parameter (it can also be used in autoexec configs).

### Prerequisites
 * [hl2sdk](https://github.com/alliedmodders/hl2sdk) of the game you plan on writing plugin for (the current plugin build scripts allows only for 1 sdk and 1 platform at a time!);
 * [metamod-source](https://github.com/alliedmodders/metamod-source);
 * [steamworks-sdk](https://partner.steamgames.com/dashboard);
 * [python3](https://www.python.org/)
 * [ambuild](https://github.com/alliedmodders/ambuild), make sure ``ambuild`` command is available via the ``PATH`` environment variable;

### Setting up
 * ``mkdir build`` & ``cd build`` in the root of the plugin folder.
 * Open the [MSVC developer console](https://learn.microsoft.com/en-us/cpp/build/building-on-the-command-line) with the correct platform (x86 or x86_64) that you plan on targetting.
 * Run ``python3 ../configure.py --plugin-name={PLUGIN_NAME} --plugin-alias={PLUGIN_ALIAS} -s {SDKNAME} --targets={TARGET} --mms_path={MMS_PATH} --steamwork_path={STEAMWORKS_PATH} --hl2sdk-root {HL2SDKROOT} `` where:
   * ``{PLUGIN_NAME}`` should be the plugin name which is used for the resulting binary name and folder naming scheme (this doesn't affect the plugin name you'd see in the plugin list if you don't modify the base plugin functions);
   * ``{PLUGIN_ALIAS}`` should be used to set the plugin alias that is used as a short hand version to load, unload, list info etc via the metamod-source menu (example being ``meta unload server_steam_account``, where ``server_steam_account`` is the alias);
   * ``{SDKNAME}`` should be the hl2sdk game name that you are building for;
   * ``{TARGET}`` should be the target platform you are targeting (``x86`` or ``x86_64``);
   * ``{MMS_PATH}`` should point to the root of the metamod-source folder;
   * ``{STEAMWORKS_PATH}`` should point to the root of the SteamWorks project folder;
   * ``{HL2SDKROOT}`` should point to the root of the hl2sdk's folders, note that it should not point to the actual ``hl2sdk-GAME`` folder but a root parent of it;
   * Alternatively ``{MMS_PATH}`` & ``{HL2SDKROOT}`` could be put as a ``PATH`` environment variables, like ``MMSOURCE112=D:\mmsource-1.12`` & ``HL2SDKCS2=D:\hl2sdks\hl2sdk-cs2`` (note the metamod version and that here hl2sdk environment variable should point directly to the game's hl2sdk folder and not to the root of it!)
   * Example: ``python3 ../configure.py --plugin-name=server_steam_account_mm --plugin-alias=server_steam_account -s cs2 --targets=x86_64 --mms_path=D:\mmsource-1.12 --hl2sdk-root=D:\hl2sdks``
 * If the process of configuring was successful, you should be able to run ``ambuild`` in the ``\build`` folder to compile the plugin.
 * Once the plugin is compiled the files would be packaged and placed in ``\build\package`` folder.
 * To run the plugin on the server, place the files preserving the layout provided in ``\package``. Be aware that plugins get loaded either by corresponding ``.vdf`` files (automatic step) in the metamod folder, or by listing them in ``addons/metamod/metaplugins.ini`` file (manual step).

## For more information on compiling and reading the plugin's source code, see:

	http://wiki.alliedmods.net/Category:Metamod:Source_Development

