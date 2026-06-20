/**
 *  @file header.c
 *  @brief drumlogue SDK unit header
 */

#include "unit.h"

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_delfx,
    .api = UNIT_API_VERSION,
    .dev_id = 0x54415552U,
    .unit_id = 0x00000201U,
    .version = 0x00010102U,
    .name = "GrainD V1.1.2",
    .num_presets = 0,
    .num_params = 9,
    .params = {
        {0, 100, 50, 60, k_unit_param_type_percent, 0, 0, 0, {"DENSITY"}},
        {10, 100, 50, 45, k_unit_param_type_percent, 0, 0, 0, {"G-LEN"}},
        {0, 5, 1, 1, k_unit_param_type_strings, 0, 0, 0, {"RHYTHM"}},
        {0, 100, 50, 55, k_unit_param_type_percent, 0, 0, 0, {"MIX"}},

        {0, 36, 24, 24, k_unit_param_type_strings, 0, 0, 0, {"PITCH"}},
        {0, 100, 50, 55, k_unit_param_type_percent, 0, 0, 0, {"SPREAD"}},
        {0, 100, 50, 70, k_unit_param_type_percent, 0, 0, 0, {"PAN"}},
        {0, 2, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"COLOR"}},

        {0, 100, 50, 35, k_unit_param_type_percent, 0, 0, 0, {"CHAOS"}},
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
