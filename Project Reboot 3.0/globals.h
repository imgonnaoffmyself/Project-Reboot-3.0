#pragma once

#include <atomic>

#include "inc.h"

namespace Globals
{
	extern inline bool bCreative = false;
	extern inline bool bGoingToPlayEvent = false;
	extern inline bool bEnableAGIDs = true;
#ifdef INTERNAL_BUILD
	extern inline bool bNoMCP = false;
#else
	extern inline bool bNoMCP = true;
#endif
	extern inline bool bLogProcessEvent = false;
	// extern inline bool bLateGame = false;
	extern inline std::atomic<bool> bLateGame(false);

	extern inline bool bInfiniteMaterials = false;
	extern inline bool bInfiniteAmmo = false;
	extern inline bool bShouldUseReplicationGraph = false;

	extern inline bool bHitReadyToStartMatch = false;
	extern inline bool bInitializedPlaylist = false;
	extern inline bool bStartedListening = false;
	extern inline bool bStarted = false;
	extern inline bool bAutoRestart = false; // doesnt work fyi
	extern inline bool bFillVendingMachines = true;
	extern inline bool bPrivateIPsAreOperator = true;
	extern inline int AmountOfListens = 0; // TODO: Switch to this for LastNum
#ifdef INTERNAL_BUILD
	extern inline bool bDeveloperMode = true;
#else
	extern inline bool bDeveloperMode = false;
#endif
}

extern inline int NumToSubtractFromSquadId = 0; // I think 2?

// Playlist dropdown options
struct PlaylistOption {
	std::string displayName;
	std::string path;
};

// Function to get playlist options based on Fortnite version
inline std::vector<PlaylistOption> GetPlaylistOptions() {
	std::vector<PlaylistOption> options = {
		{"Solo", "/Game/Athena/Playlists/Playlist_DefaultSolo.Playlist_DefaultSolo"},
		{"Duos", "/Game/Athena/Playlists/Playlist_DefaultDuo.Playlist_DefaultDuo"},
		{"Squads", "/Game/Athena/Playlists/Playlist_DefaultSquad.Playlist_DefaultSquad"}
	};
	
	// Add 50v50 option only for Fortnite version 1.10
	if (Fortnite_Version == 1.10) {
		options.push_back({"50v50", "/Game/Athena/Playlists/Playlist_50v50.Playlist_50v50"});
	}
	
	return options;
}

extern inline std::vector<PlaylistOption> PlaylistOptions = GetPlaylistOptions();

extern inline int SelectedPlaylistIndex = 0; // DefaultSolo is default
extern inline std::string PlaylistName = PlaylistOptions[0].path;

// Other playlist options (commented for reference)
// "/Game/Athena/Playlists/gg/Playlist_Gg_Reverse.Playlist_Gg_Reverse";
// "/Game/Athena/Playlists/Playground/Playlist_Playground.Playlist_Playground";
// "/Game/Athena/Playlists/Carmine/Playlist_Carmine.Playlist_Carmine";
// "/Game/Athena/Playlists/Fill/Playlist_Fill_Solo.Playlist_Fill_Solo";
// "/Game/Athena/Playlists/Low/Playlist_Low_Solo.Playlist_Low_Solo";
// "/Game/Athena/Playlists/Bling/Playlist_Bling_Solo.Playlist_Bling_Solo";
// "/Game/Athena/Playlists/Creative/Playlist_PlaygroundV2.Playlist_PlaygroundV2";
// "/Game/Athena/Playlists/Ashton/Playlist_Ashton_Sm.Playlist_Ashton_Sm";
// "/Game/Athena/Playlists/BattleLab/Playlist_BattleLab.Playlist_BattleLab";
// "/MoleGame/Playlists/Playlist_MoleGame.Playlist_MoleGame"; // very experimental dont use