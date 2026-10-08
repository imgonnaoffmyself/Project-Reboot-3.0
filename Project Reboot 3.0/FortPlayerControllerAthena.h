#pragma once

#include "FortPlayerController.h"
#include "FortPlayerStateAthena.h"
#include "FortPlayerPawn.h"
#include "SoftObjectPtr.h"
#include "FortKismetLibrary.h"
#include "AthenaMarkerComponent.h"
#include "FortVolume.h"
#include "AthenaPlayerMatchReport.h"

static void ApplyHID(AFortPlayerPawn* Pawn, UObject* HeroDefinition, bool bUseServerChoosePart = false)
{
	using UFortHeroSpecialization = UObject;

	// Fix for 1.7.2/1.8 broken female HIDs where arms/legs are misaligned (wrong skeleton / missing parts).
	// For 1.7.2 only HID_001_F is broken, for 1.8 HID_001-004_F are broken. Copy CharacterParts from a working female HID
	// to avoid the misaligned skeleton while keeping female appearance (not cop-out to male).
	{
		auto HeroPath = HeroDefinition->GetPathName();
		bool bIsBrokenFemale = false;
		if (Fortnite_Version == 1.72)
		{
			bIsBrokenFemale = HeroPath.contains("HID_001_Athena_Commando_F");
		}
		else if (Fortnite_Version == 1.8)
		{
			bIsBrokenFemale = HeroPath.contains("HID_001_Athena_Commando_F") || HeroPath.contains("HID_002_Athena_Commando_F") || HeroPath.contains("HID_003_Athena_Commando_F") || HeroPath.contains("HID_004_Athena_Commando_F");
		}
		if (bIsBrokenFemale)
		{
			// Find a working female HID to copy parts from (avoid the broken ones)
			static UFortItemDefinition* WorkingFemaleHID = nullptr;
			if (!WorkingFemaleHID)
			{
				// Try known working female HIDs in order of preference
				const wchar_t* Candidates[] = {
					L"/Game/Athena/Heroes/HID_005_Athena_Commando_F.HID_005_Athena_Commando_F",
					L"/Game/Athena/Heroes/HID_006_Athena_Commando_F.HID_006_Athena_Commando_F",
					L"/Game/Athena/Heroes/HID_037_Athena_Commando_F.HID_037_Athena_Commando_F",
					L"/Game/Athena/Heroes/HID_002_Athena_Commando_F.HID_002_Athena_Commando_F", // for 1.72 this is working
					L"/Game/Athena/Heroes/HID_003_Athena_Commando_F.HID_003_Athena_Commando_F",
				};
				for (auto CandPath : Candidates)
				{
					auto Cand = FindObject<UFortItemDefinition>(CandPath);
					if (Cand)
					{
						auto CandPathStr = Cand->GetPathName();
						bool bCandBroken = false;
						if (Fortnite_Version == 1.72) bCandBroken = CandPathStr.contains("HID_001_Athena_Commando_F");
						else if (Fortnite_Version == 1.8) bCandBroken = CandPathStr.contains("HID_001_Athena_Commando_F") || CandPathStr.contains("HID_002_Athena_Commando_F") || CandPathStr.contains("HID_003_Athena_Commando_F") || CandPathStr.contains("HID_004_Athena_Commando_F");
						if (!bCandBroken)
						{
							WorkingFemaleHID = Cand;
							LOG_INFO(LogDev, "Female fix: using working HID {} for broken {}", CandPathStr, HeroPath);
							break;
						}
					}
				}
				// Fallback: find any Athena female not in broken list via scan
				if (!WorkingFemaleHID)
				{
					auto AllHeroTypes = GetAllObjectsOfClass(FindObject<UClass>(L"/Script/FortniteGame.FortHeroType"));
					for (int i = 0; i < AllHeroTypes.size(); ++i)
					{
						auto HT = (UFortItemDefinition*)AllHeroTypes.at(i);
						auto HP = HT->GetPathName();
						if (!HP.starts_with("/Game/Athena/Heroes/") || !HP.contains("_F")) continue;
						bool bBroken = false;
						if (Fortnite_Version == 1.72) bBroken = HP.contains("HID_001_Athena_Commando_F");
						else if (Fortnite_Version == 1.8) bBroken = HP.contains("HID_001_Athena_Commando_F") || HP.contains("HID_002_Athena_Commando_F") || HP.contains("HID_003_Athena_Commando_F") || HP.contains("HID_004_Athena_Commando_F");
						if (!bBroken)
						{
							WorkingFemaleHID = HT;
							LOG_INFO(LogDev, "Female fix: scanned working HID {} for broken {}", HP, HeroPath);
							break;
						}
					}
				}
			}
			if (WorkingFemaleHID)
			{
				// Use the working HID's Specializations/CharacterParts instead of the broken one's
				HeroDefinition = WorkingFemaleHID;
				LOG_INFO(LogDev, "Female fix: remapped broken HID {} to working HID {}", HeroPath, WorkingFemaleHID->GetPathName());
			}
		}
	}

	static auto SpecializationsOffset = HeroDefinition->GetOffset("Specializations");
	auto& Specializations = HeroDefinition->Get<TArray<TSoftObjectPtr<UFortHeroSpecialization>>>(SpecializationsOffset);

	auto PlayerState = Pawn->GetPlayerState();

	for (int i = 0; i < Specializations.Num(); i++)
	{
		auto& SpecializationSoft = Specializations.at(i);

		static auto FortHeroSpecializationClass = FindObject<UClass>(L"/Script/FortniteGame.FortHeroSpecialization");
		auto Specialization = SpecializationSoft.Get(FortHeroSpecializationClass, true);

		if (Specialization)
		{
			static auto Specialization_CharacterPartsOffset = Specialization->GetOffset("CharacterParts");
			auto& CharacterParts = Specialization->Get<TArray<TSoftObjectPtr<UObject>>>(Specialization_CharacterPartsOffset);

			static auto CustomCharacterPartClass = FindObject<UClass>(L"/Script/FortniteGame.CustomCharacterPart");

			if (bUseServerChoosePart)
			{
				for (int z = 0; z < CharacterParts.Num(); z++)
				{
					Pawn->ServerChoosePart((EFortCustomPartType)z, CharacterParts.at(z).Get(CustomCharacterPartClass, true));
				}

				continue; // hm?
			}

			bool aa;

			TArray<UObject*> CharacterPartsaa;

			for (int z = 0; z < CharacterParts.Num(); z++)
			{
				auto& CharacterPartSoft = CharacterParts.at(z, GetSoftObjectSize());
				auto CharacterPart = CharacterPartSoft.Get(CustomCharacterPartClass, true);

				CharacterPartsaa.Add(CharacterPart);

				continue;
			}

			UFortKismetLibrary::ApplyCharacterCosmetics(GetWorld(), CharacterPartsaa, PlayerState, &aa);
			CharacterPartsaa.Free();
		}
	}
}

static bool ApplyCID(AFortPlayerPawn* Pawn, UObject* CID, bool bUseServerChoosePart = false)
{
	if (!CID)
		return false;

	auto PlayerController = Cast<AFortPlayerController>(Pawn->GetController());

	if (!PlayerController)
		return false;

	if (bUseServerChoosePart)
	{
		if (Pawn)
		{

		}
	}

	/* auto PCCosmeticLoadout = PlayerController->GetCosmeticLoadout();

	if (!PCCosmeticLoadout)
	{
		LOG_INFO(LogCosmetics, "PCCosmeticLoadout is not set! Will not be able to apply skin.");
		return false;
	}

	auto PawnCosmeticLoadout = PlayerController->GetCosmeticLoadout();

	if (!PawnCosmeticLoadout)
	{
		LOG_INFO(LogCosmetics, "PawnCosmeticLoadout is not set! Will not be able to apply skin.");
		return false;
	}

	PCCosmeticLoadout->GetCharacter() = CID;
	PawnCosmeticLoadout->GetCharacter() = CID;
	PlayerController->ApplyCosmeticLoadout(); // would cause recursive

	return true; */

	static auto HeroDefinitionOffset = CID->GetOffset("HeroDefinition");
	if (HeroDefinitionOffset == -1)
		return false;

	auto HeroDefinition = CID->Get(HeroDefinitionOffset);
	if (!HeroDefinition)
		return false;

	ApplyHID(Pawn, HeroDefinition, bUseServerChoosePart);

	// static auto HeroTypeOffset = PlayerState->GetOffset("HeroType");
	// PlayerState->Get(HeroTypeOffset) = HeroDefinition;

	return true;
}

struct FGhostModeRepData
{
	bool& IsInGhostMode()
	{
		static auto bInGhostModeOffset = FindOffsetStruct("/Script/FortniteGame.GhostModeRepData", "bInGhostMode");
		return *(bool*)(__int64(this) + bInGhostModeOffset);
	}

	UFortWorldItemDefinition*& GetGhostModeItemDef()
	{
		static auto GhostModeItemDefOffset = FindOffsetStruct("/Script/FortniteGame.GhostModeRepData", "GhostModeItemDef");
		return *(UFortWorldItemDefinition**)(__int64(this) + GhostModeItemDefOffset);
	}
};

class AFortPlayerControllerAthena : public AFortPlayerController
{
public:
	static inline void (*GetPlayerViewPointOriginal)(AFortPlayerControllerAthena* PlayerController, FVector& Location, FRotator& Rotation);
	static inline void (*ServerReadyToStartMatchOriginal)(AFortPlayerControllerAthena* PlayerController);
	static inline void (*ServerRequestSeatChangeOriginal)(AFortPlayerControllerAthena* PlayerController, int TargetSeatIndex);
	static inline void (*EnterAircraftOriginal)(UObject* PC, AActor* Aircraft);
	static inline void (*StartGhostModeOriginal)(UObject* Context, FFrame* Stack, void* Ret);
	static inline void (*EndGhostModeOriginal)(AFortPlayerControllerAthena* PlayerController);

	void SpectateOnDeath() // actually in zone
	{
		static auto SpectateOnDeathFn = FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerControllerZone.SpectateOnDeath") ?
			FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerControllerZone.SpectateOnDeath") :
			FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerControllerAthena.SpectateOnDeath");

		this->ProcessEvent(SpectateOnDeathFn);
	}

	class UAthenaResurrectionComponent*& GetResurrectionComponent()
	{
		static auto ResurrectionComponentOffset = GetOffset("ResurrectionComponent");
		return Get<class UAthenaResurrectionComponent*>(ResurrectionComponentOffset);
	}

	AFortPlayerStateAthena* GetPlayerStateAthena()
	{
		return (AFortPlayerStateAthena*)GetPlayerState();
	}

	FGhostModeRepData* GetGhostModeRepData()
	{
		static auto GhostModeRepDataOffset = GetOffset("GhostModeRepData", false);

		if (GhostModeRepDataOffset == -1)
			return nullptr;

		return GetPtr<FGhostModeRepData>(GhostModeRepDataOffset);
	}

	bool IsInGhostMode()
	{
		auto GhostModeRepData = GetGhostModeRepData();

		if (!GhostModeRepData)
			return false;

		return GhostModeRepData->IsInGhostMode();
	}

	UAthenaMarkerComponent* GetMarkerComponent()
	{
		static auto MarkerComponentOffset = GetOffset("MarkerComponent");
		return Get<UAthenaMarkerComponent*>(MarkerComponentOffset);
	}

	AFortVolume*& GetCreativePlotLinkedVolume()
	{
		static auto CreativePlotLinkedVolumeOffset = GetOffset("CreativePlotLinkedVolume");
		return Get<AFortVolume*>(CreativePlotLinkedVolumeOffset);
	}

	void ClientClearDeathNotification() // actually in zone
	{
		auto ClientClearDeathNotificationFn = FindFunction("ClientClearDeathNotification");

		if (ClientClearDeathNotificationFn)
			this->ProcessEvent(ClientClearDeathNotificationFn);
	}

	UAthenaPlayerMatchReport** GetMatchReport()
	{
		static auto MatchReportOffset = GetOffset("MatchReport", false);
		return MatchReportOffset == -1 ? nullptr : GetPtr<UAthenaPlayerMatchReport*>(MatchReportOffset);
	}

	void ClientSendTeamStatsForPlayer(FAthenaMatchTeamStats* TeamStats)
	{
		static auto ClientSendTeamStatsForPlayerFn = FindObject<UFunction>("/Script/FortniteGame.FortPlayerControllerAthena.ClientSendTeamStatsForPlayer");
		static auto ParamSize = ClientSendTeamStatsForPlayerFn->GetPropertiesSize();
		auto Params = malloc(ParamSize);

		memcpy_s(Params, ParamSize, TeamStats, TeamStats->GetStructSize());

		this->ProcessEvent(ClientSendTeamStatsForPlayerFn, Params);

		free(Params);
	}

	void RespawnPlayerAfterDeath(bool bEnterSkydiving)
	{
		static auto RespawnPlayerAfterDeathFn = FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerControllerAthena.RespawnPlayerAfterDeath");

		if (RespawnPlayerAfterDeathFn)
		{
			this->ProcessEvent(RespawnPlayerAfterDeathFn, &bEnterSkydiving);
		}
		else
		{
			// techinally we can remake this as all it really does on older versions is clear deathinfo (I think?)
		}
	}

	void ClientOnPawnRevived(AController* EventInstigator) // actually zone // idk what this actually does but i call it
	{
		static auto ClientOnPawnRevivedFn = FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerControllerZone.ClientOnPawnRevived");
		this->ProcessEvent(ClientOnPawnRevivedFn, &EventInstigator);
	}

	bool IsMarkedAlive()
	{
		static auto bMarkedAliveOffset = GetOffset("bMarkedAlive", false);

		if (bMarkedAliveOffset == -1) // nots ure if this is possible
			return true;

		static auto bMarkedAliveFieldMask = GetFieldMask(GetProperty("bMarkedAlive"));
		return ReadBitfieldValue(bMarkedAliveOffset, bMarkedAliveFieldMask);
	}

	static void StartGhostModeHook(UObject* Context, FFrame* Stack, void* Ret); // we could native hook this but eh
	static void EndGhostModeHook(AFortPlayerControllerAthena* PlayerController);
	static void ServerCreativeSetFlightSpeedIndexHook(UObject* Context, FFrame* Stack);
	static void EnterAircraftHook(UObject* PC, AActor* Aircraft);
	static void ServerRequestSeatChangeHook(AFortPlayerControllerAthena* PlayerController, int TargetSeatIndex); // actually in zone
	static void ServerRestartPlayerHook(AFortPlayerControllerAthena* Controller);
	static void ServerGiveCreativeItemHook(AFortPlayerControllerAthena* Controller, FFortItemEntry CreativeItem);
	static void ServerTeleportToPlaygroundLobbyIslandHook(AFortPlayerControllerAthena* Controller);
	static void ServerAcknowledgePossessionHook(APlayerController* Controller, APawn* Pawn);
	static void ServerPlaySquadQuickChatMessageHook(AFortPlayerControllerAthena* PlayerController, __int64 ChatEntry, __int64 SenderID);
	static void GetPlayerViewPointHook(AFortPlayerControllerAthena* PlayerController, FVector& Location, FRotator& Rotation);
	static void ServerReadyToStartMatchHook(AFortPlayerControllerAthena* PlayerController);
	static void UpdateTrackedAttributesHook(AFortPlayerControllerAthena* PlayerController);
	static void ServerClientIsReadyToRespawnHook(AFortPlayerControllerAthena* PlayerControllerAthena); // 1:1

	static UClass* StaticClass()
	{
		static auto Class = FindObject<UClass>(L"/Script/FortniteGame.FortPlayerControllerAthena");
		return Class;
	}
};