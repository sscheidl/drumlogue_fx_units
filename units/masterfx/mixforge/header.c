/**
 *  @file header.c
 *  @brief drumlogue SDK unit header
 *
 *  Copyright (c) 2020-2022 KORG Inc. All rights reserved.
 *
 */

#include "unit.h"  // Note: Include common definitions for all units

// ---- Unit header definition  --------------------------------------------------------------------

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),                     // leave as is, size of this header
    .target = UNIT_TARGET_PLATFORM | k_unit_module_masterfx,  // target platform and module for this unit
    .api = UNIT_API_VERSION,                                  // logue sdk API version against which unit was built
    .dev_id = 0x54415552U,                                    // developer id: "TAUR"
    .unit_id = 0x00000001U,                                   // Id for this unit, unique within the developer id
    .version = 0x00010000U,                                   // This unit's version: major.minor.patch (major<<16 minor<<8 patch).
    .name = "MixForge",                                       // Name for this unit, will be displayed on device
    .num_presets = 0,                                         // Number of internal presets this unit has
    .num_params = 12,                                         // Number of parameters for this unit, max 24
    .params = {
        // Format: min, max, center, default, type, fractional, frac. type, <reserved>, name

        // See common/runtime.h for type enum and unit_param_t structure

        // Page 1: DJ filter
        {0, 100, 50, 50, k_unit_param_type_percent, 0, 0, 0, {"FILTER"}},
        {0, 100, 0, 15, k_unit_param_type_percent, 0, 0, 0, {"RESONANCE"}},
        {0, 2, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"SLOPE"}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},

        // Page 2: compressor
        {-600, 0, 0, -180, k_unit_param_type_db, 1, 1, 0, {"THRESH"}},
        {1, 20, 1, 4, k_unit_param_type_none, 0, 0, 0, {"RATIO"}},
        {1, 200, 1, 10, k_unit_param_type_msec, 0, 0, 0, {"ATTACK"}},
        {10, 2000, 10, 300, k_unit_param_type_msec, 0, 0, 0, {"RELEASE"}},

        // Page 3: saturation and bitcrush
        {0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"DRIVE"}},
        {0, 3, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"SAT MODE"}},
        {0, 1, 0, 0, k_unit_param_type_onoff, 0, 0, 0, {"BITCRUSH"}},
        {4, 16, 16, 16, k_unit_param_type_none, 0, 0, 0, {"BITS"}},

        // Page 4
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},

        // Page 5
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},

        // Page 6
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}}};
