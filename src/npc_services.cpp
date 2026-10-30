/*
 * tw-mod-npc-services
 *
 * Tortoise-WoW port of AzerothCore mod-npc-services.
 */

#include "npc_services.h"
#include "Config/Config.h"
#include "Chat.h"
#include "Player.h"
#include "SpellMgr.h"
#include "ScriptedGossip.h"
#include "Database/DatabaseEnv.h"
#include "Map.h"

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
    enum NpcServiceActions
    {
        ACTION_RESTORE_HP_MP    = 1,
        ACTION_RESET_INSTANCES  = 2,
        ACTION_RESET_COOLDOWNS  = 3,
        ACTION_LEAVE_COMBAT     = 4,
        ACTION_REMOVE_SICKNESS  = 5,
        ACTION_REPAIR_ITEMS     = 6,
        ACTION_RESET_TALENTS    = 7,
        ACTION_OPEN_BANK        = 8,
        ACTION_TELEPORT_MENU    = 9,
        ACTION_TELEPORT_DUNGEONS = 10,
        ACTION_TELEPORT_RAIDS   = 11,
        ACTION_BACK_MAIN        = 12,
        ACTION_TELEPORT_DUNGEON_BASE = 100,
        ACTION_TELEPORT_RAID_BASE    = 1000,
        ACTION_TELEPORT_DUNGEON_PAGE_BASE = 10000,
        ACTION_TELEPORT_RAID_PAGE_BASE    = 20000,
        ACTION_FACTION_CITY_MENU          = 21,
        ACTION_FACTION_CITY_BASE          = 30000,
        ACTION_WORLD_BUFFS_MENU           = 22,
        ACTION_WORLD_BUFF_BASE            = 40000,
    };

    static const size_t TELEPORT_PAGE_SIZE = 10;

    enum NpcServiceSpells
    {
        // Classic resurrection sickness spell.
        SPELL_RESURRECTION_SICKNESS = 15007,
    };

    struct TeleportDestination
    {
        uint32 mapId;
        std::string name;
        float x;
        float y;
        float z;
        float o;
    };

    bool NpcServicesEnabled  = true;
    bool NpcServicesAnnounceEnabled = true;

    bool NpcServicesRestoreHpMpEnabled      = true;
    bool NpcServicesResetInstancesEnabled   = true;
    bool NpcServicesResetCooldownsEnabled   = true;
    bool NpcServicesLeaveCombatEnabled      = true;
    bool NpcServicesRemoveSicknessEnabled   = true;
    bool NpcServicesRepairItemsEnabled      = true;
    bool NpcServicesResetTalentsEnabled     = true;
    bool NpcServicesOpenBankEnabled         = true;
    bool NpcServicesTeleportDungeonsEnabled = true;
    bool NpcServicesTeleportRaidsEnabled    = true;
    bool NpcServicesTeleportFactionCitiesEnabled = true;
    bool NpcServicesWorldBuffsEnabled       = true;

    uint32 NpcServicesVisualSpell = 26636; // default visual

    std::vector<TeleportDestination> sDungeonDestinations;
    std::vector<TeleportDestination> sRaidDestinations;

    struct FactionCity
    {
        Team team;
        std::string name;
        uint32 mapId;
        float x;
        float y;
        float z;
        float o;
    };

    static const std::vector<FactionCity> sFactionCities = {
        { ALLIANCE, "Stormwind",    0, -8844.0f,  626.0f,   94.0f,  0.7f },
        { ALLIANCE, "Ironforge",    0, -4918.0f, -935.0f,  502.0f,  0.0f },
        { ALLIANCE, "Darnassus",    1,  9962.0f, 2280.0f, 1342.0f,  0.0f },
        { HORDE,    "Orgrimmar",    1,  1437.0f, -4421.0f,  25.0f,  0.0f },
        { HORDE,    "Thunder Bluff", 1, -1274.0f,  116.0f,  131.0f,  0.0f },
        { HORDE,    "Undercity",    0,  1586.0f,  240.0f,  -52.0f,  0.0f },
    };

    struct WorldBuff
    {
        uint32 spellId;
        Team team;
        std::string name;
        std::string configKey;
        bool enabled;
    };

    static std::vector<WorldBuff> sWorldBuffs = {
        { 22888, ALLIANCE, "Rallying Cry of the Dragonslayer",   "NpcServices.WorldBuffs.RallyingCryOfTheDragonslayer.Enable", true },
        { 24425, TEAM_NONE, "Spirit of Zandalar",                "NpcServices.WorldBuffs.SpiritOfZandalar.Enable",             true },
        { 22818, TEAM_NONE, "Mol'dar's Moxie",                   "NpcServices.WorldBuffs.MoldarsMoxie.Enable",                 true },
        { 22817, TEAM_NONE, "Fengus' Ferocity",                  "NpcServices.WorldBuffs.FengusFerocity.Enable",               true },
        { 22820, TEAM_NONE, "Slip'kik's Savvy",                  "NpcServices.WorldBuffs.SlipkiksSavvy.Enable",                true },
        { 15366, TEAM_NONE, "Songflower Serenade",               "NpcServices.WorldBuffs.SongflowerSerenade.Enable",           true },
        { 23768, TEAM_NONE, "Sayge's Dark Fortune of Damage",    "NpcServices.WorldBuffs.SaygesDarkFortuneDamage.Enable",      true },
        { 23736, TEAM_NONE, "Sayge's Dark Fortune of Agility",   "NpcServices.WorldBuffs.SaygesDarkFortuneAgility.Enable",     true },
        { 23767, TEAM_NONE, "Sayge's Dark Fortune of Armor",     "NpcServices.WorldBuffs.SaygesDarkFortuneArmor.Enable",       true },
        { 23766, TEAM_NONE, "Sayge's Dark Fortune of Intelligence", "NpcServices.WorldBuffs.SaygesDarkFortuneIntelligence.Enable", true },
        { 23769, TEAM_NONE, "Sayge's Dark Fortune of Resistance", "NpcServices.WorldBuffs.SaygesDarkFortuneResistance.Enable",  true },
        { 23738, TEAM_NONE, "Sayge's Dark Fortune of Spirit",    "NpcServices.WorldBuffs.SaygesDarkFortuneSpirit.Enable",      true },
        { 23737, TEAM_NONE, "Sayge's Dark Fortune of Stamina",   "NpcServices.WorldBuffs.SaygesDarkFortuneStamina.Enable",     true },
        { 23735, TEAM_NONE, "Sayge's Dark Fortune of Strength",  "NpcServices.WorldBuffs.SaygesDarkFortuneStrength.Enable",    true },
        { 16609, HORDE,    "Warchief's Blessing",               "NpcServices.WorldBuffs.WarchiefsBlessing.Enable",            true },
    };

    class NpcServicesConfig : public WorldScript
    {
    public:
        NpcServicesConfig()
            : WorldScript("NpcServicesConfig", { WORLDHOOK_ON_BEFORE_WORLD_INITIALIZED })
        {
        }

        void OnBeforeWorldInitialized() override
        {
            NpcServicesEnabled        = sConfig.GetBoolDefault("NpcServices.Enable", true);
            NpcServicesAnnounceEnabled = sConfig.GetBoolDefault("NpcServices.Announce", true);

            NpcServicesRestoreHpMpEnabled    = sConfig.GetBoolDefault("NpcServices.RestoreHpMp.Enable", true);
            NpcServicesResetInstancesEnabled = sConfig.GetBoolDefault("NpcServices.ResetInstances.Enable", true);
            NpcServicesResetCooldownsEnabled = sConfig.GetBoolDefault("NpcServices.ResetCooldowns.Enable", true);
            NpcServicesLeaveCombatEnabled    = sConfig.GetBoolDefault("NpcServices.LeaveCombat.Enable", true);
            NpcServicesRemoveSicknessEnabled = sConfig.GetBoolDefault("NpcServices.RemoveSickness.Enable", true);
            NpcServicesRepairItemsEnabled    = sConfig.GetBoolDefault("NpcServices.RepairItems.Enable", true);
            NpcServicesResetTalentsEnabled   = sConfig.GetBoolDefault("NpcServices.ResetTalents.Enable", true);
            NpcServicesOpenBankEnabled              = sConfig.GetBoolDefault("NpcServices.OpenBank.Enable", true);
            NpcServicesTeleportDungeonsEnabled      = sConfig.GetBoolDefault("NpcServices.Teleport.Dungeons.Enable", true);
            NpcServicesTeleportRaidsEnabled         = sConfig.GetBoolDefault("NpcServices.Teleport.Raids.Enable", true);
            NpcServicesTeleportFactionCitiesEnabled = sConfig.GetBoolDefault("NpcServices.Teleport.FactionCities.Enable", true);
            NpcServicesWorldBuffsEnabled            = sConfig.GetBoolDefault("NpcServices.WorldBuffs.Enable", true);

            if (NpcServicesWorldBuffsEnabled)
            {
                for (auto& buff : sWorldBuffs)
                    buff.enabled = sConfig.GetBoolDefault(buff.configKey.c_str(), true);
            }

            NpcServicesVisualSpell = sConfig.GetIntDefault("NpcServices.VisualSpell", 26636);

            if (NpcServicesTeleportDungeonsEnabled || NpcServicesTeleportRaidsEnabled)
                LoadTeleportDestinations();
        }

        static void LoadTeleportDestinations()
        {
            sDungeonDestinations.clear();
            sRaidDestinations.clear();

            std::unique_ptr<QueryResult> result(WorldDatabase.PQuery(
                "SELECT `mt`.`entry`, `mt`.`map_name`, `mt`.`map_type`, "
                "`at`.`name`, `at`.`target_position_x`, `at`.`target_position_y`, `at`.`target_position_z`, `at`.`target_orientation` "
                "FROM `map_template` `mt` "
                "JOIN `areatrigger_teleport` `at` ON `at`.`target_map` = `mt`.`entry` "
                "WHERE `mt`.`map_type` IN (1, 2) "
                "ORDER BY `mt`.`entry`, `at`.`name`"));

            if (!result)
                return;

            std::unordered_map<uint32, TeleportDestination> bestForMap;

            do
            {
                Field* fields = result->Fetch();
                uint32 mapId        = fields[0].GetUInt32();
                std::string mapName = fields[1].GetCppString();
                uint32 mapType      = fields[2].GetUInt32();
                std::string atName  = fields[3].GetCppString();
                float x             = fields[4].GetFloat();
                float y             = fields[5].GetFloat();
                float z             = fields[6].GetFloat();
                float o             = fields[7].GetFloat();

                // Skip obvious test maps.
                if (mapId == 29)
                    continue;

                auto it = bestForMap.find(mapId);
                bool preferThis = false;
                if (it == bestForMap.end())
                {
                    preferThis = true;
                }
                else
                {
                    bool currentIsEntrance = atName.find("Entrance") != std::string::npos;
                    bool storedIsEntrance  = it->second.name.find("Entrance") != std::string::npos;
                    if (currentIsEntrance && !storedIsEntrance)
                        preferThis = true;
                }

                if (preferThis)
                {
                    TeleportDestination dest;
                    dest.mapId = mapId;
                    dest.name  = mapName.empty() ? atName : mapName;
                    dest.x     = x;
                    dest.y     = y;
                    dest.z     = z;
                    dest.o     = o;
                    bestForMap[mapId] = dest;
                }
            } while (result->NextRow());

            for (auto& pair : bestForMap)
            {
                if (pair.second.mapId == 0)
                    continue;

                if (MapEntry const* mapEntry = sMapStorage.LookupEntry<MapEntry>(pair.second.mapId))
                {
                    if (mapEntry->IsRaid())
                        sRaidDestinations.push_back(pair.second);
                    else if (mapEntry->IsDungeon())
                        sDungeonDestinations.push_back(pair.second);
                }
            }

            auto sortByName = [](const TeleportDestination& a, const TeleportDestination& b)
            {
                return a.name < b.name;
            };
            std::sort(sDungeonDestinations.begin(), sDungeonDestinations.end(), sortByName);
            std::sort(sRaidDestinations.begin(), sRaidDestinations.end(), sortByName);
        }
    };

    class NpcServicesAnnounce : public PlayerScript
    {
    public:
        NpcServicesAnnounce()
            : PlayerScript("NpcServicesAnnounce", { PLAYERHOOK_ON_LOGIN })
        {
        }

        void OnLogin(Player* player) override
        {
            if (NpcServicesEnabled && NpcServicesAnnounceEnabled)
                ChatHandler(player->GetSession()).SendSysMessage("This server is running the |cff4CFF00NpcServices|r module.");
        }
    };

    void CastServiceVisual(Player* player)
    {
        if (SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(NpcServicesVisualSpell))
            player->CastSpell(player, NpcServicesVisualSpell, true);
    }

    bool IsInCombatNotify(Player* player)
    {
        if (player->IsInCombat())
        {
            player->GetSession()->SendNotification("You are in combat!");
            return true;
        }
        return false;
    }

    void SendMainMenu(Player* player, Creature* creature)
    {
        if (NpcServicesRestoreHpMpEnabled)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, "|TInterface\\icons\\Spell_Nature_Regenerate:40:40:-18|t Restore HP and MP", GOSSIP_SENDER_MAIN, ACTION_RESTORE_HP_MP);
        if (NpcServicesResetInstancesEnabled)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, "|TInterface\\icons\\Achievement_BG_winAB_underXminutes:40:40:-18|t Reset Instances", GOSSIP_SENDER_MAIN, ACTION_RESET_INSTANCES);
        if (NpcServicesResetCooldownsEnabled)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, "|TInterface\\icons\\SPELL_HOLY_BORROWEDTIME:40:40:-18|t Reset Cooldowns", GOSSIP_SENDER_MAIN, ACTION_RESET_COOLDOWNS);
        if (NpcServicesLeaveCombatEnabled)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, "|TInterface\\icons\\Achievement_BG_AB_defendflags:40:40:-18|t Reset Combat", GOSSIP_SENDER_MAIN, ACTION_LEAVE_COMBAT);
        if (NpcServicesRemoveSicknessEnabled)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, "|TInterface\\icons\\Spell_Shadow_DeathScream:40:40:-18|t Remove Sickness", GOSSIP_SENDER_MAIN, ACTION_REMOVE_SICKNESS);
        if (NpcServicesRepairItemsEnabled)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, "|TInterface\\icons\\INV_Hammer_24:40:40:-18|t Repair Items", GOSSIP_SENDER_MAIN, ACTION_REPAIR_ITEMS);
        if (NpcServicesResetTalentsEnabled)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, "|TInterface\\icons\\Achievement_WorldEvent_Lunar:40:40:-18|t Reset Talents", GOSSIP_SENDER_MAIN, ACTION_RESET_TALENTS);
        if (NpcServicesOpenBankEnabled)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, "|TInterface/Icons/INV_Misc_Bag_07:40:40:-18|t Bank", GOSSIP_SENDER_MAIN, ACTION_OPEN_BANK);

        bool hasDungeons = NpcServicesTeleportDungeonsEnabled && !sDungeonDestinations.empty();
        bool hasRaids    = NpcServicesTeleportRaidsEnabled    && !sRaidDestinations.empty();
        bool hasCities   = NpcServicesTeleportFactionCitiesEnabled;

        if (hasDungeons || hasRaids || hasCities)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TAXI, "|TInterface/Icons/INV_Misc_Rune_06:40:40:-18|t Teleport", GOSSIP_SENDER_MAIN, ACTION_TELEPORT_MENU);

        if (NpcServicesWorldBuffsEnabled)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, "|TInterface/Icons/Spell_Nature_Regeneration:40:40:-18|t World Buffs", GOSSIP_SENDER_MAIN, ACTION_WORLD_BUFFS_MENU);

        player->PlayerTalkClass->SendGossipMenu(DEFAULT_GOSSIP_MESSAGE, creature->GetObjectGuid());
    }

    void SendTeleportMenu(Player* player, Creature* creature)
    {
        if (NpcServicesTeleportDungeonsEnabled && !sDungeonDestinations.empty())
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TAXI, "Dungeons", GOSSIP_SENDER_MAIN, ACTION_TELEPORT_DUNGEONS);
        if (NpcServicesTeleportRaidsEnabled && !sRaidDestinations.empty())
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TAXI, "Raids", GOSSIP_SENDER_MAIN, ACTION_TELEPORT_RAIDS);
        if (NpcServicesTeleportFactionCitiesEnabled)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TAXI, "Faction Cities", GOSSIP_SENDER_MAIN, ACTION_FACTION_CITY_MENU);

        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TALK, "Back", GOSSIP_SENDER_MAIN, ACTION_BACK_MAIN);
        player->PlayerTalkClass->SendGossipMenu(DEFAULT_GOSSIP_MESSAGE, creature->GetObjectGuid());
    }

    void SendDungeonListPage(Player* player, Creature* creature, size_t page)
    {
        size_t start = page * TELEPORT_PAGE_SIZE;
        size_t end = std::min(start + TELEPORT_PAGE_SIZE, sDungeonDestinations.size());

        for (size_t i = start; i < end; ++i)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TAXI, sDungeonDestinations[i].name.c_str(), GOSSIP_SENDER_MAIN, static_cast<uint32>(ACTION_TELEPORT_DUNGEON_BASE + i));

        if (page > 0)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TALK, "Previous", GOSSIP_SENDER_MAIN, static_cast<uint32>(ACTION_TELEPORT_DUNGEON_PAGE_BASE + (page - 1)));
        if (end < sDungeonDestinations.size())
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TALK, "Next", GOSSIP_SENDER_MAIN, static_cast<uint32>(ACTION_TELEPORT_DUNGEON_PAGE_BASE + (page + 1)));

        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TALK, "Back", GOSSIP_SENDER_MAIN, ACTION_TELEPORT_MENU);
        player->PlayerTalkClass->SendGossipMenu(DEFAULT_GOSSIP_MESSAGE, creature->GetObjectGuid());
    }

    void SendRaidListPage(Player* player, Creature* creature, size_t page)
    {
        size_t start = page * TELEPORT_PAGE_SIZE;
        size_t end = std::min(start + TELEPORT_PAGE_SIZE, sRaidDestinations.size());

        for (size_t i = start; i < end; ++i)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TAXI, sRaidDestinations[i].name.c_str(), GOSSIP_SENDER_MAIN, static_cast<uint32>(ACTION_TELEPORT_RAID_BASE + i));

        if (page > 0)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TALK, "Previous", GOSSIP_SENDER_MAIN, static_cast<uint32>(ACTION_TELEPORT_RAID_PAGE_BASE + (page - 1)));
        if (end < sRaidDestinations.size())
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TALK, "Next", GOSSIP_SENDER_MAIN, static_cast<uint32>(ACTION_TELEPORT_RAID_PAGE_BASE + (page + 1)));

        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TALK, "Back", GOSSIP_SENDER_MAIN, ACTION_TELEPORT_MENU);
        player->PlayerTalkClass->SendGossipMenu(DEFAULT_GOSSIP_MESSAGE, creature->GetObjectGuid());
    }

    void TeleportToDestination(Player* player, const TeleportDestination& dest)
    {
        if (IsInCombatNotify(player))
            return;

        player->TeleportTo(dest.mapId, dest.x, dest.y, dest.z, dest.o);
    }

    void SendFactionCityMenu(Player* player, Creature* creature)
    {
        Team playerTeam = player->GetTeam();
        size_t index = 0;
        for (const auto& city : sFactionCities)
        {
            if (city.team == playerTeam)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TAXI, city.name.c_str(), GOSSIP_SENDER_MAIN, static_cast<uint32>(ACTION_FACTION_CITY_BASE + index));
            ++index;
        }

        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TALK, "Back", GOSSIP_SENDER_MAIN, ACTION_BACK_MAIN);
        player->PlayerTalkClass->SendGossipMenu(DEFAULT_GOSSIP_MESSAGE, creature->GetObjectGuid());
    }

    void TeleportToFactionCity(Player* player, uint32 cityIndex)
    {
        if (cityIndex >= sFactionCities.size())
            return;

        const FactionCity& city = sFactionCities[cityIndex];
        if (player->GetTeam() != city.team)
        {
            player->GetSession()->SendNotification("You cannot teleport to an enemy city!");
            return;
        }

        if (IsInCombatNotify(player))
            return;

        player->TeleportTo(city.mapId, city.x, city.y, city.z, city.o);
    }

    void SendWorldBuffsMenu(Player* player, Creature* creature)
    {
        Team playerTeam = player->GetTeam();
        size_t index = 0;
        for (const auto& buff : sWorldBuffs)
        {
            if (buff.enabled && (buff.team == TEAM_NONE || buff.team == playerTeam))
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, buff.name.c_str(), GOSSIP_SENDER_MAIN, static_cast<uint32>(ACTION_WORLD_BUFF_BASE + index));
            ++index;
        }

        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TALK, "Back", GOSSIP_SENDER_MAIN, ACTION_BACK_MAIN);
        player->PlayerTalkClass->SendGossipMenu(DEFAULT_GOSSIP_MESSAGE, creature->GetObjectGuid());
    }

    void ApplyWorldBuff(Player* player, uint32 buffIndex)
    {
        if (buffIndex >= sWorldBuffs.size())
            return;

        const WorldBuff& buff = sWorldBuffs[buffIndex];
        if (!buff.enabled)
        {
            player->GetSession()->SendNotification("That world buff is currently disabled.");
            return;
        }

        if (buff.team != TEAM_NONE && player->GetTeam() != buff.team)
        {
            player->GetSession()->SendNotification("This buff is not available for your faction!");
            return;
        }

        if (IsInCombatNotify(player))
            return;

        if (SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(buff.spellId))
            player->CastSpell(player, buff.spellId, true);
    }
}

bool NpcServices::OnGossipHello(Player* player, Creature* creature)
{
    if (!NpcServicesEnabled)
        return false;

    SendMainMenu(player, creature);
    return true;
}

bool NpcServices::OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
{
    if (!NpcServicesEnabled)
        return false;

    player->PlayerTalkClass->ClearMenus();

    if (action == ACTION_TELEPORT_MENU)
    {
        SendTeleportMenu(player, creature);
        return true;
    }

    if (action == ACTION_TELEPORT_DUNGEONS)
    {
        if (NpcServicesTeleportDungeonsEnabled && !sDungeonDestinations.empty())
            SendDungeonListPage(player, creature, 0);
        return true;
    }

    if (action == ACTION_TELEPORT_RAIDS)
    {
        if (NpcServicesTeleportRaidsEnabled && !sRaidDestinations.empty())
            SendRaidListPage(player, creature, 0);
        return true;
    }

    if (action >= ACTION_TELEPORT_DUNGEON_PAGE_BASE && action < ACTION_TELEPORT_RAID_PAGE_BASE)
    {
        if (NpcServicesTeleportDungeonsEnabled && !sDungeonDestinations.empty())
        {
            size_t page = action - ACTION_TELEPORT_DUNGEON_PAGE_BASE;
            SendDungeonListPage(player, creature, page);
        }
        return true;
    }

    if (action >= ACTION_TELEPORT_RAID_PAGE_BASE && action < ACTION_FACTION_CITY_BASE)
    {
        if (NpcServicesTeleportRaidsEnabled && !sRaidDestinations.empty())
        {
            size_t page = action - ACTION_TELEPORT_RAID_PAGE_BASE;
            SendRaidListPage(player, creature, page);
        }
        return true;
    }


    if (action == ACTION_FACTION_CITY_MENU)
    {
        if (NpcServicesTeleportFactionCitiesEnabled)
            SendFactionCityMenu(player, creature);
        return true;
    }

    if (action >= ACTION_FACTION_CITY_BASE && action < ACTION_WORLD_BUFF_BASE)
    {
        if (NpcServicesTeleportFactionCitiesEnabled)
        {
            size_t index = action - ACTION_FACTION_CITY_BASE;
            player->PlayerTalkClass->CloseGossip();
            TeleportToFactionCity(player, index);
        }
        return true;
    }

    if (action == ACTION_WORLD_BUFFS_MENU)
    {
        if (NpcServicesWorldBuffsEnabled)
            SendWorldBuffsMenu(player, creature);
        return true;
    }

    if (action >= ACTION_WORLD_BUFF_BASE)
    {
        if (NpcServicesWorldBuffsEnabled)
        {
            size_t index = action - ACTION_WORLD_BUFF_BASE;
            player->PlayerTalkClass->CloseGossip();
            ApplyWorldBuff(player, index);
        }
        return true;
    }

    if (action == ACTION_BACK_MAIN)
    {
        SendMainMenu(player, creature);
        return true;
    }

    if (action >= ACTION_TELEPORT_DUNGEON_BASE && action < ACTION_TELEPORT_RAID_BASE)
    {
        if (NpcServicesTeleportDungeonsEnabled)
        {
            size_t index = action - ACTION_TELEPORT_DUNGEON_BASE;
            if (index < sDungeonDestinations.size())
            {
                player->PlayerTalkClass->CloseGossip();
                TeleportToDestination(player, sDungeonDestinations[index]);
            }
        }
        return true;
    }

    if (action >= ACTION_TELEPORT_RAID_BASE)
    {
        if (NpcServicesTeleportRaidsEnabled)
        {
            size_t index = action - ACTION_TELEPORT_RAID_BASE;
            if (index < sRaidDestinations.size())
            {
                player->PlayerTalkClass->CloseGossip();
                TeleportToDestination(player, sRaidDestinations[index]);
            }
        }
        return true;
    }

    switch (action)
    {
        case ACTION_RESTORE_HP_MP:
            if (!NpcServicesRestoreHpMpEnabled)
            {
                player->PlayerTalkClass->CloseGossip();
                return true;
            }
            player->PlayerTalkClass->CloseGossip();
            if (IsInCombatNotify(player))
                return false;

            if (player->GetPowerType() == POWER_MANA)
                player->SetPower(POWER_MANA, player->GetMaxPower(POWER_MANA));
            player->SetHealth(player->GetMaxHealth());

            player->GetSession()->SendNotification("|cffFFFF00NPC SERVICES \n |cffFFFFFFHP & MP successfully restored!");
            CastServiceVisual(player);
            break;

        case ACTION_RESET_INSTANCES:
            if (!NpcServicesResetInstancesEnabled)
            {
                player->PlayerTalkClass->CloseGossip();
                return true;
            }
            player->PlayerTalkClass->CloseGossip();
            for (auto itr = player->GetBoundInstances().begin(); itr != player->GetBoundInstances().end();)
            {
                if (itr->first != player->GetMapId())
                {
                    player->UnbindInstance(itr->first, true);
                    itr = player->GetBoundInstances().begin();
                }
                else
                {
                    ++itr;
                }
            }
            player->GetSession()->SendNotification("|cffFFFF00NPC SERVICES \n |cffFFFFFFInstances successfully reset!");
            CastServiceVisual(player);
            break;

        case ACTION_RESET_COOLDOWNS:
            if (!NpcServicesResetCooldownsEnabled)
            {
                player->PlayerTalkClass->CloseGossip();
                return true;
            }
            player->PlayerTalkClass->CloseGossip();
            if (IsInCombatNotify(player))
                return false;

            player->RemoveAllSpellCooldown();
            player->GetSession()->SendNotification("|cffFFFF00NPC SERVICES \n |cffFFFFFFCooldowns successfully reset!");
            CastServiceVisual(player);
            break;

        case ACTION_LEAVE_COMBAT:
            if (!NpcServicesLeaveCombatEnabled)
            {
                player->PlayerTalkClass->CloseGossip();
                return true;
            }
            player->PlayerTalkClass->CloseGossip();
            player->CombatStop();
            player->GetSession()->SendNotification("|cffFFFF00NPC SERVICES \n |cffFFFFFFCombat successfully removed!");
            CastServiceVisual(player);
            break;

        case ACTION_REMOVE_SICKNESS:
            if (!NpcServicesRemoveSicknessEnabled)
            {
                player->PlayerTalkClass->CloseGossip();
                return true;
            }
            player->PlayerTalkClass->CloseGossip();
            if (player->HasAura(SPELL_RESURRECTION_SICKNESS))
                player->RemoveAurasDueToSpell(SPELL_RESURRECTION_SICKNESS);
            player->GetSession()->SendNotification("|cffFFFF00NPC SERVICES \n |cffFFFFFFSickness successfully removed!");
            CastServiceVisual(player);
            break;

        case ACTION_REPAIR_ITEMS:
            if (!NpcServicesRepairItemsEnabled)
            {
                player->PlayerTalkClass->CloseGossip();
                return true;
            }
            player->PlayerTalkClass->CloseGossip();
            player->DurabilityRepairAll(false, 0.0f);
            player->GetSession()->SendNotification("|cffFFFF00NPC SERVICES \n |cffFFFFFFItems repaired successfully!");
            CastServiceVisual(player);
            break;

        case ACTION_RESET_TALENTS:
            if (!NpcServicesResetTalentsEnabled)
            {
                player->PlayerTalkClass->CloseGossip();
                return true;
            }
            player->PlayerTalkClass->CloseGossip();
            player->ResetTalents(true);
            player->GetSession()->SendNotification("|cffFFFF00NPC SERVICES \n |cffFFFFFFTalents reset successfully!");
            CastServiceVisual(player);
            break;

        case ACTION_OPEN_BANK:
            if (!NpcServicesOpenBankEnabled)
            {
                player->PlayerTalkClass->CloseGossip();
                return true;
            }
            player->PlayerTalkClass->CloseGossip();
            player->GetSession()->SendShowBank(player->GetObjectGuid());
            break;

        default:
            break;
    }

    return true;
}

void AddSC_Npc_Services()
{
    new NpcServicesConfig();
    new NpcServicesAnnounce();
    new NpcServices();
}
