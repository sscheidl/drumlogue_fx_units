/**
 *  @file header.c
 *  @brief drumlogue SDK unit header
 */

#include "unit.h"

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_revfx,
    .api = UNIT_API_VERSION,
    .dev_id = 0x54415552U,
    .unit_id = 0x00000101U,
    .version = 0x00010000U,
    .name = "GateVerb80",
    .num_presets = 0,
    .num_params = 8,
    .params = {
        {0, 100, 50, 60, k_unit_param_type_percent, 0, 0, 0, {"SIZE"}},
        {0, 100, 50, 70, k_unit_param_type_percent, 0, 0, 0, {"GATE"}},
        {0, 100, 50, 50, k_unit_param_type_percent, 0, 0, 0, {"TONE"}},
        {0, 100, 50, 60, k_unit_param_type_percent, 0, 0, 0, {"COMP"}},

        {0, 40, 10, 15, k_unit_param_type_msec, 0, 0, 0, {"PREDELAY"}},
        {0, 100, 20, 30, k_unit_param_type_percent, 0, 0, 0, {"HP"}},
        {0, 100, 50, 75, k_unit_param_type_percent, 0, 0, 0, {"WIDTH"}},
        {0, 100, 50, 40, k_unit_param_type_percent, 0, 0, 0, {"MIX"}},

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
