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
			// HID_005_Athena_Commando_F does NOT exist (HID_005_M is male). Previous STW GrenadeGun fallback path may not exist in 1.8 either, so also try direct CustomCharacterPart fix.
			static UFortItemDefinition* WorkingFemaleHID = nullptr;
			if (!WorkingFemaleHID)
			{
				// Try known working female HIDs in order of preference — STW Ramirez is the most reliable fallback since it's correctly rigged in all versions
				const wchar_t* Candidates[] = {
					L"/Game/Athena/Heroes/HID_002_Athena_Commando_F.HID_002_Athena_Commando_F", // for 1.72 this is working (only 001_F broken)
					L"/Game/Athena/Heroes/HID_003_Athena_Commando_F.HID_003_Athena_Commando_F", // also working in 1.72
					L"/Game/Athena/Heroes/HID_Commando_GrenadeGun_UC_T01.HID_Commando_GrenadeGun_UC_T01", // STW starter Ramirez — correctly rigged, game's default fallback
					L"/Game/Heroes/HID_Commando_GrenadeGun_UC_T01.HID_Commando_GrenadeGun_UC_T01",
					L"/Game/Athena/Heroes/HID_037_Athena_Commando_F.HID_037_Athena_Commando_F",
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
				// Fallback: find any Athena female not in broken list via scan, then STW heroes
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
					if (!WorkingFemaleHID)
					{
						// Last resort: STW Ramirez GrenadeGun — exists in all versions and is correctly rigged
						auto STW1 = FindObject<UFortItemDefinition>(L"/Game/Athena/Heroes/HID_Commando_GrenadeGun_UC_T01.HID_Commando_GrenadeGun_UC_T01");
						auto STW2 = FindObject<UFortItemDefinition>(L"/Game/Heroes/HID_Commando_GrenadeGun_UC_T01.HID_Commando_GrenadeGun_UC_T01");
						if (STW1) WorkingFemaleHID = STW1;
						else if (STW2) WorkingFemaleHID = STW2;
						if (WorkingFemaleHID) LOG_INFO(LogDev, "Female fix: using STW fallback {} for broken {}", WorkingFemaleHID->GetPathName(), HeroPath);
					}
					// Direct CustomCharacterPart fallback — when all Athena female HIDs are broken (1.8) and STW HID also not found, directly use known working body/head parts
					// This bypasses HID entirely and fixes the misaligned skeleton by using correctly rigged F_Med parts that exist in all versions
					if (!WorkingFemaleHID)
					{
						static auto CustomCharacterPartClass = FindObject<UClass>(L"/Script/FortniteGame.CustomCharacterPart");
						const wchar_t* BodyCandidates[] = {
							L"/Game/Characters/CharacterParts/Female/Medium/Bodies/F_Med_Soldier_01.F_Med_Soldier_01",
							L"/Game/Characters/Player/Female/Medium/Bodies/F_Med_Soldier_01.F_Med_Soldier_01",
							L"/Game/Characters/CharacterParts/Female/Medium/Bodies/F_Med_Soldier_02.F_Med_Soldier_02",
						};
						const wchar_t* HeadCandidates[] = {
							L"/Game/Characters/CharacterParts/Female/Medium/Heads/F_Med_Head1.F_Med_Head1",
							L"/Game/Characters/Player/Female/Medium/Heads/F_Med_Head1.F_Med_Head1",
						};
						UObject* WorkingBodyPart = nullptr;
						UObject* WorkingHeadPart = nullptr;
						for (auto p : BodyCandidates) { WorkingBodyPart = FindObject<UObject>(p); if (WorkingBodyPart) break; }
						for (auto p : HeadCandidates) { WorkingHeadPart = FindObject<UObject>(p); if (WorkingHeadPart) break; }
						if (WorkingBodyPart || WorkingHeadPart)
						{
							LOG_INFO(LogDev, "Female fix: direct CustomCharacterPart fix for broken {} using body {} head {} (HID fallback failed)", HeroPath, WorkingBodyPart ? WorkingBodyPart->GetPathName() : L"null", WorkingHeadPart ? WorkingHeadPart->GetPathName() : L"null");
							// Directly patch the broken HeroDefinition's Specializations — this fixes misaligned skeleton even when all Athena HIDs are broken (1.8)
							static auto SpecOffset_Direct = HeroDefinition->GetOffset("Specializations");
							if (SpecOffset_Direct != -1)
							{
								auto& BrokenSpecsDirect = HeroDefinition->Get<TArray<TSoftObjectPtr<UObject>>>(SpecOffset_Direct);
								for (int s = 0; s < BrokenSpecsDirect.Num(); ++s)
								{
									auto BrokenSpecDirect = BrokenSpecsDirect.at(s).Get(FindObject<UClass>(L"/Script/FortniteGame.FortHeroSpecialization"), true);
									if (!BrokenSpecDirect) continue;
									static auto CharPartsOffset_Direct = BrokenSpecDirect->GetOffset("CharacterParts");
									if (CharPartsOffset_Direct == -1) continue;
									auto& BrokenPartsDirect = BrokenSpecDirect->Get<TArray<TSoftObjectPtr<UObject>>>(CharPartsOffset_Direct);
									for (int pIdx = 0; pIdx < BrokenPartsDirect.Num(); ++pIdx)
									{
										auto PartSoft = BrokenPartsDirect.at(pIdx).Get(CustomCharacterPartClass, true);
										if (!PartSoft) continue;
										auto PartPath = PartSoft->GetPathName();
										// Replace Body (contains /Bodies/ and F_Med) with working body, Head with working head — fixes arms through stomach
										if (WorkingBodyPart && PartPath.contains(L"/Bodies/") && PartPath.contains(L"F_Med"))
										{
											BrokenPartsDirect.at(pIdx) = TSoftObjectPtr<UObject>(WorkingBodyPart);
											LOG_INFO(LogDev, "Female fix: replaced Body part {} with {}", PartPath, WorkingBodyPart->GetPathName());
										}
										else if (WorkingHeadPart && PartPath.contains(L"/Heads/") && PartPath.contains(L"F_Med"))
										{
											BrokenPartsDirect.at(pIdx) = TSoftObjectPtr<UObject>(WorkingHeadPart);
											LOG_INFO(LogDev, "Female fix: replaced Head part {} with {}", PartPath, WorkingHeadPart->GetPathName());
										}
									}
								}
							}
							// Direct fix applied — patched parts will be used in normal flow below, no HID copy needed
						}
						else
						{
							LOG_WARN(LogDev, "Female fix: direct part fallback failed for broken {} — no working F_Med parts found (tried BodyCandidates/head)", HeroPath);
						}
					}
				}
			}
			if (WorkingFemaleHID)
			{
				// Copy CharacterParts from working HID into broken HeroDefinition's Specializations instead of remapping entire HeroDefinition (keeps health/backpack)
				static auto HeroDefOffset_Working = WorkingFemaleHID->GetOffset("HeroDefinition");
				static auto HeroDefOffset_BrokenForCopy = HeroDefinition ? HeroDefinition->GetOffset("Specializations") : -1;
				UObject* WorkingHeroDef = HeroDefOffset_Working != -1 ? WorkingFemaleHID->Get<UObject*>(HeroDefOffset_Working) : nullptr;
				// If working HID is already a HeroDefinition itself (STW case where HID is the definition), handle directly
				if (!WorkingHeroDef && WorkingFemaleHID->GetOffset("Specializations") != -1)
					WorkingHeroDef = WorkingFemaleHID;
				if (WorkingHeroDef)
				{
					static auto SpecOffset_Broken = HeroDefinition->GetOffset("Specializations");
					static auto SpecOffset_Working = WorkingHeroDef->GetOffset("Specializations");
					if (SpecOffset_Broken != -1 && SpecOffset_Working != -1)
					{
						auto& BrokenSpecs = HeroDefinition->Get<TArray<TSoftObjectPtr<UObject>>>(SpecOffset_Broken);
						auto& WorkingSpecs = WorkingHeroDef->Get<TArray<TSoftObjectPtr<UObject>>>(SpecOffset_Working);
						if (BrokenSpecs.Num() > 0 && WorkingSpecs.Num() > 0)
						{
							for (int s = 0; s < BrokenSpecs.Num() && s < WorkingSpecs.Num(); ++s)
							{
								auto BrokenSpec = BrokenSpecs.at(s).Get(FindObject<UClass>(L"/Script/FortniteGame.FortHeroSpecialization"), true);
								auto WorkingSpec = WorkingSpecs.at(s).Get(FindObject<UClass>(L"/Script/FortniteGame.FortHeroSpecialization"), true);
								if (!BrokenSpec || !WorkingSpec) continue;
								static auto CharPartsOffset_Broken2 = BrokenSpec->GetOffset("CharacterParts");
								static auto CharPartsOffset_Working2 = WorkingSpec->GetOffset("CharacterParts");
								if (CharPartsOffset_Broken2 == -1 || CharPartsOffset_Working2 == -1) continue;
								auto& BrokenParts = BrokenSpec->Get<TArray<TSoftObjectPtr<UObject>>>(CharPartsOffset_Broken2);
								auto& WorkingParts = WorkingSpec->Get<TArray<TSoftObjectPtr<UObject>>>(CharPartsOffset_Working2);
								if (WorkingParts.Num() == 0) continue;
								LOG_INFO(LogDev, "Female fix: remapped broken HID {} to working HID {} (copied {} parts)", HeroPath, WorkingFemaleHID->GetPathName(), WorkingParts.Num());
								BrokenParts.Free();
								for (int p = 0; p < WorkingParts.Num(); ++p) BrokenParts.Add(WorkingParts.at(p));
							}
						}
						else
						{
							// Fallback to full remap if specialization copy fails
							HeroDefinition = WorkingHeroDef;
							LOG_INFO(LogDev, "Female fix: fallback remapped broken HID {} to working HID {}", HeroPath, WorkingFemaleHID->GetPathName());
						}
					}
				}
				else
				{
					HeroDefinition = WorkingFemaleHID;
					LOG_INFO(LogDev, "Female fix: remapped broken HID {} to working HID {}", HeroPath, WorkingFemaleHID->GetPathName());
				}
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