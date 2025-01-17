#pragma once

// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

#define _WIN32_WINNT 0x601

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN

#define SERVER

#define MODNAME "TAMods-Server"
#define MODVERSION "0.8.4"
#define TASERVER_PROTOCOL_VERSION "5.0.0"
#define TAMODS_PROTOCOL_VERSION "0.1.0"

#define LOGFILE_NAME "TAMods-Server.log"
#define CONFIGFILE_NAME "serverconfig.lua"
#define CUSTOMFILE_NAME "servercustom.lua"

#define RELEASE