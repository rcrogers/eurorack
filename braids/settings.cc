// Copyright 2012 Olivier Gillet.
//
// Author: Olivier Gillet (ol.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// Settings

#include "braids/settings.h"

#include <cstring>

#include "stmlib/system/storage.h"

namespace braids {

using namespace stmlib;

const SettingsData kInitSettings = {
  MACRO_OSC_SHAPE_CSAW, // shape
  RESOLUTION_16_BIT,    // resolution
  SAMPLE_RATE_96K,      // sample_rate
  0,                    // trig_destination
  false,                // auto_trig (Trig source)
  1,                    // trig_delay
  false,                // meta_modulation
  PITCH_RANGE_EXTERNAL, // pitch_range
  2,                    // pitch_octave
  PITCH_QUANTIZATION_OFF, //pitch_quantization
  false,                // vco_flatten
  false,                // vco_drift
  false,                // signature
  2,                    // brightness
  0,                    // trig_ad_shape
  0,                    // mod1_shape
  0,                    // mod2_shape
  0,                    // mod1_depth
  0,                    // mod2_depth
  10,                   // mod1_ad_ratio
  10,                   // mod2_ad_ratio
  20,                   // mod1_rate
  20,                   // mod2_rate  
  50,                   // pitch_cv_offset
  15401,                // pitch_cv_scale
  2048,                 // fm_cv_offset
};

Storage<0x8020000, 4> storage;

void Settings::Init() {
  if (!storage.ParsimoniousLoad(&data_, &version_token_)) {
    Reset();
  }
}

void Settings::Reset() {
  memcpy(&data_, &kInitSettings, sizeof(SettingsData));
}

void Settings::Save() {
  storage.ParsimoniousSave(data_, &version_token_);
}

const char* const boolean_values[] = { "OFF ", "ON  " };

const char* const algo_values[] = {
    "CSAW",
    "^\x88\x8D_",
    "\x88\x8A\x8C\x8D",
    "SYNC",
    "FOLD",
    "\x8E\x8E\x8E\x8E",
    "\x88\x88x3",
    "\x8C_x3",
    "/\\x3",
    "SIx3",
    "RING",
    "\x88\x89\x88\x89",
    "\x88\x88\x8E\x8E",
    "TOY*",
    "ZLPF",
    "ZPKF",
    "ZBPF",
    "ZHPF",
    "VOSM",
    "VOWL",
    "VFOF",
    "FM  ",
    "FBFM",
    "WTFM",
    "PLUK",
    "BOWD",
    "BLOW",
    "FLUT",
    "BELL",
    "DRUM",
    "KICK",
    "CYMB",
    "SNAR",
    "WTBL",
    "WMAP",
    "WLIN",
    "WTx4",
    "NOIS",
    "TWNQ",
    "CLKN",
    "CLOU",
    "PRTC",
    // "QPSK",
    // "NAME" // For your algorithm
};

const char* const bits_values[] = {
    "2BIT",
    "3BIT",
    "4BIT",
    "6BIT",
    "8BIT",
    "12B ",
    "16B " };
    
const char* const rates_values[] = {
    "   0",
    "   1",
    "   2",
    "   3",
    "   4",
    "   5",
    "   6",
    "   7",
    "   8",
    "   9",
    "  10",
    "  11",
    "  12",
    "  13",
    "  14",
    "  15",
    "  16",
    "  17",
    "  18",
    "  19",
    "  20",
    "  21",
    "  22",
    "  23",
    "  24",
    "  25",
    "  26",
    "  27",
    "  28",
    "  29",
    "  30",
    "  31",
    "  32",
    "  33",
    "  34",
    "  35",
    "  36",
    "  37",
    "  38",
    "  39",
    "  40",
    "  41",
    "  42",
    "  43",
    "  44",
    "  45",
    "  46",
    "  47",
    "  48",
    "  49",
    "  50",
    "  51",
    "  52",
    "  53",
    "  54",
    "  55",
    "  56",
    "  57",
    "  58",
    "  59",
    "  60",
    "  61",
    "  62",
    "  63",
    "  64",
    "  65",
    "  66",
    "  67",
    "  68",
    "  69",
    "  70",
    "  71",
    "  72",
    "  73",
    "  74",
    "  75",
    "  76",
    "  77",
    "  78",
    "  79",
    "  80",
    "  81",
    "  82",
    "  83",
    "  84",
    "  85",
    "  86",
    "  87",
    "  88",
    "  89",
    "  90",
    "  91",
    "  92",
    "  93",
    "  94",
    "  95",
    "  96",
    "  97",
    "  98",
    "  99",
    " 100",
    " 101",
    " 102",
    " 103",
    " 104",
    " 105",
    " 106",
    " 107",
    " 108",
    " 109",
    " 110",
    " 111",
    " 112",
    " 113",
    " 114",
    " 115",
    " 116",
    " 117",
    " 118",
    " 119",
    " 120",
    " 121",
    " 122",
    " 123",
    " 124",
    " 125",
    " 126",
    " 127",
};
    
const char* const quantization_values[] = { "OFF ", "QRTR", "SEMI" };

const char* const trig_source_values[] = { "EXT.", "AUTO" };

const char* const pitch_range_values[] = {
    "EXT.",
    "FREE",
    "XTND",
    "440 ",
    "LFO "
};

const char* const octave_values[] = { "-2", "-1", "0", "1", "2" };

const char* const trig_delay_values[] = {
    "NONE",
    "125u",
    "250u",
    "500u",
    "1ms ",
    "2ms ",
    "4ms "
};

const char* const ad_shape_values[] = {
    "TT  ",
    "PIK ",
    "PING",
    "TONG",
    "BONG",
    "LONG",
    "SLOW",
    "WOMP",
    "YIFF",
};

const char* const mod_depth_values[] = {
    "   0",
    "  10",
    "  20",
    "  30",
    "  40",
    "  50",
    "  60",
    "  70",
    "  80",
    "  90",
    " 100",
    " 110",
    " 120",
    " 130",    
    " 140",    
    " 150",    
    " 160",    
    " 170",    
    " 180",    
    " 190",    
    " 200",    
    " 210",    
    " 220",    
    " 230",    
    " 240",    
    " 250",    
};

const char* const trig_destination_values[] = {
    "SYNC",
    "TIMB",
    "LEVL",
    "T+L ",
    "COLR",
    "T+C ",
    "L+C ",
    "ALL "
};

const char* const brightness_values[] = {
    "\xff   ",
    "\xff\xff  ",
    "\xff\xff\xff\xff",
};

const char* const meta_values[] = { 
    "FREQ", // 0
    "META", // 1
    "RATE", // 2
};

const char* const ad_ratio_values[] = { 
    "0.02", // 0
    "0.10", // 1
    "0.20", // 2
    "0.30", // 3
    "0.40", // 4
    "0.50", // 5
    "0.60", // 6
    "0.70", // 7
    "0.80", // 8
    "0.90", // 9
    "1.00", // 10
    "1.11", // 11
    "1.25", // 12
    "1.43", // 13
    "1.66", // 14
    "2.00", // 15
    "2.50", // 16
    "3.33", // 17
    "5.00", // 18
    "10.0", // 19
    "50.0", // 20
};

const char* const mod_shape_values[] = { 
    "EXPO",  // 0 exponentially-curved triangle
    "LINR",  // 1 linear triangle
    "WIGL",  // 2 wiggly, using ws_sine_fold (a show about nothing?)
    "SINE",  // 3 sine-ish, using ws_moderate_overdrive
    "SQRE",  // 4 square-ish, using ws_violent_overdrive
    "BOWF",  // 5 bowing friction LUT
};

const char* const mod_mode_values[] = { 
    "OFF ",  // 0 
    "ENV ",  // 1 
    "LFO ",  // 2 
};

/* static */
const SettingMetadata Settings::metadata_[] = {
  { 0, MACRO_OSC_SHAPE_LAST - 1, "WAVE", algo_values },
  { 0, RESOLUTION_LAST - 1, "BITS", bits_values },
  { 0, SAMPLE_RATE_LAST - 1, "RATE", rates_values },
  { 0, 7, "TDST", trig_destination_values },
  { 0, 1, "TSRC", trig_source_values },
  { 0, 6, "TDLY", trig_delay_values },
  { 0, 2, "FMCV", meta_values },
  { 0, 4, "RANG", pitch_range_values }, // enable LFO pitch range
  { 0, 4, "OCTV", octave_values },
  { 0, PITCH_QUANTIZATION_LAST - 1, "QNTZ", quantization_values },
  { 0, 1, "FLAT", boolean_values },
  { 0, 1, "DRFT", boolean_values },
  { 0, 1, "SIGN", boolean_values },
  { 0, 2, "BRIG", brightness_values },
  { 0, 8, "TENV", ad_shape_values },
  { 0, 5, "SHP1", mod_shape_values },
  { 0, 5, "SHP2", mod_shape_values },
  { 0, 25, "DEP1", mod_depth_values },
  { 0, 25, "DEP2", mod_depth_values },
  { 0, 20, "A:D1", ad_ratio_values },
  { 0, 20, "A:D2", ad_ratio_values },
  { 0, 2, "MOD1", mod_mode_values },
  { 0, 2, "MOD2", mod_mode_values },
  { 0, 127, "RAT1", rates_values },
  { 0, 127, "RAT2", rates_values },
  { 0, 0, "CAL.", NULL },
  { 0, 0, "    ", NULL },  // Placeholder for CV tester
  { 0, 0, "BT3g", NULL },  // Placeholder for version string
};

/* static */
const Setting Settings::settings_order_[] = {
  SETTING_OSCILLATOR_SHAPE,
  SETTING_SAMPLE_RATE, 
  SETTING_BRIGHTNESS, 
  SETTING_TRIG_DESTINATION,
  SETTING_TRIG_AD_SHAPE,
  SETTING_META_MODULATION,
  SETTING_TRIG_SOURCE,
  SETTING_TRIG_DELAY,
  SETTING_PITCH_RANGE,
  SETTING_PITCH_OCTAVE,
  SETTING_PITCH_QUANTIZER,
  SETTING_VCO_FLATTEN,
  SETTING_VCO_DRIFT,
  SETTING_SIGNATURE,
  SETTING_RESOLUTION,
  SETTING_MOD1_SHAPE,
  SETTING_MOD2_SHAPE,
  SETTING_MOD1_DEPTH,
  SETTING_MOD2_DEPTH,
  SETTING_MOD1_AD_RATIO,
  SETTING_MOD2_AD_RATIO,
  SETTING_MOD1_MODE,
  SETTING_MOD2_MODE,
  SETTING_MOD1_RATE,
  SETTING_MOD2_RATE,
  SETTING_CALIBRATION,
  SETTING_CV_TESTER,
  SETTING_VERSION,
};

/* extern */
Settings settings;

}  // namespace braids
