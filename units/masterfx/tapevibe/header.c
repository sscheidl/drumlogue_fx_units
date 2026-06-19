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
    .unit_id = 0x00000002U,
    .version = 0x00010000U,
    .name = "TapeVibe",
    .num_presets = 0,
    .num_params = 10,
    .params = {
        {0, 100, 30, 35, k_unit_param_type_percent, 0, 0, 0, {"DRIVE"}},
        {0, 100, 50, 50, k_unit_param_type_percent, 0, 0, 0, {"TONE"}},
        {0, 100, 30, 40, k_unit_param_type_percent, 0, 0, 0, {"AGE"}},
        {0, 100, 50, 40, k_unit_param_type_percent, 0, 0, 0, {"MIX"}},

        {0, 100, 20, 20, k_unit_param_type_percent, 0, 0, 0, {"WOW"}},
        {0, 100, 15, 15, k_unit_param_type_percent, 0, 0, 0, {"FLUTTER"}},
        {0, 100, 20, 20, k_unit_param_type_percent, 0, 0, 0, {"HISS"}},
        {0, 2, 0, 1, k_unit_param_type_strings, 0, 0, 0, {"HISS TYPE"}},

        {0, 100, 10, 10, k_unit_param_type_percent, 0, 0, 0, {"DROPOUT"}},
        {0, 2, 0, 1, k_unit_param_type_strings, 0, 0, 0, {"DROP TYPE"}},
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
