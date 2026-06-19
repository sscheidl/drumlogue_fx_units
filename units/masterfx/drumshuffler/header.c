/**
 *  @file header.c
 *  @brief drumlogue SDK unit header
 */

#include "unit.h"

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_masterfx,
    .api = UNIT_API_VERSION,
    .dev_id = 0x54415552U,
    .unit_id = 0x00000003U,
    .version = 0x00010000U,
    .name = "DrumShuffler",
    .num_presets = 0,
    .num_params = 8,
    .params = {
        {0, 100, 50, 30, k_unit_param_type_percent, 0, 0, 0, {"SHUFFLE"}},
        {0, 100, 50, 20, k_unit_param_type_percent, 0, 0, 0, {"SWING"}},
        {0, 100, 50, 60, k_unit_param_type_percent, 0, 0, 0, {"WIDTH"}},
        {0, 100, 50, 40, k_unit_param_type_percent, 0, 0, 0, {"MIX"}},

        {0, 3, 2, 2, k_unit_param_type_strings, 0, 0, 0, {"DELAY"}},
        {0, 100, 50, 30, k_unit_param_type_percent, 0, 0, 0, {"MOD"}},
        {0, 3, 2, 2, k_unit_param_type_strings, 0, 0, 0, {"RATE"}},
        {0, 100, 50, 50, k_unit_param_type_percent, 0, 0, 0, {"COLOR"}},

        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},

        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},

        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},

        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}}};
