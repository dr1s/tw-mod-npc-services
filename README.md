# tw-mod-npc-services

Tortoise-WoW port of [AzerothCore mod-npc-services](https://github.com/azerothcore/mod-npc-services).

Adds a single NPC that offers common convenience services through a gossip menu.

## Features

- Restore HP and MP
- Reset instances
- Reset spell cooldowns
- Leave combat
- Remove resurrection sickness
- Repair all items
- Reset talents
- Open bank
- Teleport menu with Dungeons, Raids, and Faction Cities
- World Buffs menu with classic faction and neutral world buffs

The original AzerothCore "Learn Dual Talents" and "Open Mailbox" options have been removed because those features are not available in the Vanilla 1.12.1 client/core.

## Configuration

Copy `conf/tw-mod-npc-services.conf.dist` to `conf/tw-mod-npc-services.conf` and adjust the settings:

```ini
[NpcServices]
NpcServices.Enable                         = 1
NpcServices.Announce                       = 1
NpcServices.RestoreHpMp.Enable             = 1
NpcServices.ResetInstances.Enable          = 1
NpcServices.ResetCooldowns.Enable          = 1
NpcServices.LeaveCombat.Enable             = 1
NpcServices.RemoveSickness.Enable          = 1
NpcServices.RepairItems.Enable             = 1
NpcServices.ResetTalents.Enable            = 1
NpcServices.OpenBank.Enable                = 1
NpcServices.Teleport.Dungeons.Enable       = 1
NpcServices.Teleport.Raids.Enable          = 1
NpcServices.Teleport.FactionCities.Enable  = 1
NpcServices.WorldBuffs.Enable              = 1
NpcServices.WorldBuffs.RallyingCryOfTheDragonslayer.Enable  = 1
NpcServices.WorldBuffs.SpiritOfZandalar.Enable              = 1
NpcServices.WorldBuffs.MoldarsMoxie.Enable                  = 1
NpcServices.WorldBuffs.FengusFerocity.Enable                = 1
NpcServices.WorldBuffs.SlipkiksSavvy.Enable                 = 1
NpcServices.WorldBuffs.SongflowerSerenade.Enable            = 1
NpcServices.WorldBuffs.SaygesDarkFortuneDamage.Enable       = 1
NpcServices.WorldBuffs.SaygesDarkFortuneAgility.Enable      = 1
NpcServices.WorldBuffs.SaygesDarkFortuneArmor.Enable        = 1
NpcServices.WorldBuffs.SaygesDarkFortuneIntelligence.Enable = 1
NpcServices.WorldBuffs.SaygesDarkFortuneResistance.Enable   = 1
NpcServices.WorldBuffs.SaygesDarkFortuneSpirit.Enable       = 1
NpcServices.WorldBuffs.SaygesDarkFortuneStamina.Enable      = 1
NpcServices.WorldBuffs.SaygesDarkFortuneStrength.Enable     = 1
NpcServices.WorldBuffs.WarchiefsBlessing.Enable             = 1
NpcServices.VisualSpell                    = 26636
```

The default `VisualSpell` (26636) is a generic visual effect. If you prefer a different service-complete visual, change this to any valid spell ID from your DBC, or set it to `0` to disable the visual entirely.

Every service, teleport submenu, and world buff can be enabled or disabled independently. Setting `NpcServices.WorldBuffs.Enable = 0` disables the entire World Buffs menu regardless of individual toggles.

### Teleport Menu

The **Teleport** option opens a submenu with:

- **Dungeons** — populated automatically from `map_template` and `areatrigger_teleport`, including Tortoise custom dungeons.
- **Raids** — populated automatically from `map_template` and `areatrigger_teleport`, including Tortoise custom raids.
- **Faction Cities** — shows only the capital cities of the player's faction:
  - **Alliance:** Stormwind, Ironforge, Darnassus
  - **Horde:** Orgrimmar, Thunder Bluff, Undercity

### World Buffs Menu

The **World Buffs** option allows players to receive classic world buffs. Faction-specific buffs are filtered by the player's faction:

- **Alliance only:** Rallying Cry of the Dragonslayer
- **Horde only:** Warchief's Blessing
- **Neutral:** Spirit of Zandalar, Dire Maul buffs (Mol'dar's Moxie, Fengus' Ferocity, Slip'kik's Savvy), Songflower Serenade, and all Sayge's Dark Fortune buffs.

## Database

Run the migration in `data/sql/world/tw-mod-npc-services.sql` against your `world` database, or enable the auto-updater:

The SQL inserts a `creature_template` entry at `entry = 55002` with script name `npc_services`.

### Spawning the NPC

**In-game (GM command):**

```
.npc add 55002
```

## Credits

- Original module by MtgCore, Poszer, Talamortis, and Micrah.
- Ported to Tortoise-WoW from the AzerothCore version.
