# Optional CMake hooks for tw-mod-npc-services.
#
# Source files under src/ are discovered automatically,
# conf/*.conf.dist files are copied automatically, and Addtw_mod_npc_servicesScripts() is invoked by the
# generated module loader.
#
# This file is included twice:
#   TORTOISE_MODULE_CMAKE_PHASE=DISCOVERY
#   TORTOISE_MODULE_CMAKE_PHASE=POST_TARGETS
#
# Use TW_* helpers for compatibility with this core's module system. Do not use AzerothCore AC_* names.

if(TORTOISE_MODULE_CMAKE_PHASE STREQUAL "DISCOVERY")
  # No custom CMake required for this module.
endif()
