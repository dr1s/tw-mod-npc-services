SET @NpcName = 'Services NPC';
SET @NpcSubname = 'Tortoise-WoW';
SET @NpcEntry = 55002;
SET @NpcDisplayID = 2575;
SET @NpcLevel = 60;

DELETE FROM `creature_template` WHERE `entry` = @NpcEntry;
INSERT INTO `creature_template`
(
    `entry`, `name`, `subname`, `display_id1`, `display_id2`, `display_id3`, `display_id4`,
    `gossip_menu_id`, `level_min`, `level_max`, `health_min`, `health_max`, `mana_min`, `mana_max`, `armor`,
    `faction`, `npc_flags`, `rank`, `base_attack_time`, `ranged_attack_time`, `unit_class`, `unit_flags`, `type`,
    `dmg_min`, `dmg_max`, `attack_power`, `dmg_multiplier`, `script_name`
)
VALUES
(
    @NpcEntry, @NpcName, @NpcSubname, @NpcDisplayID, 0, 0, 0,
    0, @NpcLevel, @NpcLevel, 3000, 3000, 0, 0, 0,
    35, 1, 0, 2000, 2000, 2, 0, 7,
    0, 0, 0, 1, 'npc_services'
);
