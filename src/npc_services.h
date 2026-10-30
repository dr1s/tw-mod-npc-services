/*
 * tw-mod-npc-services
 *
 * Tortoise-WoW port of AzerothCore mod-npc-services.
 * Provides a single NPC that offers common convenience services via gossip.
 */

#pragma once

#include "ScriptObjects.h"

class NpcServices : public CreatureScript
{
public:
    NpcServices() : CreatureScript("npc_services") {}

    bool OnGossipHello(Player* player, Creature* creature) override;
    bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action) override;
};

void AddSC_Npc_Services();
