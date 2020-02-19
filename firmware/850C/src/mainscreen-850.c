/*
 * Bafang LCD 850C firmware
 *
 * Copyright (C) Casainho, 2018.
 *
 * Released under the GPL License, Version 3
 */

#include <math.h>
#include "stdio.h"
#include "main.h"
#include "utils.h"
#include "screen.h"
#include "rtc.h"
#include "fonts.h"
#include "uart.h"
#include "mainscreen.h"
#include "eeprom.h"
#include "buttons.h"
#include "lcd.h"
#include "adc.h"
#include "ugui.h"
#include "configscreen.h"
#include "battery_gui.h"
#include "state.h"
#include "eeprom.h"

Field batteryField = FIELD_CUSTOM(renderBattery);

uint8_t ui8_g_configuration_clock_hours;
uint8_t ui8_g_configuration_clock_minutes;

static void mainScreenOnEnter() {
	// Set the font preference for this screen
	editable_label_font = &SMALL_TEXT_FONT;
	editable_value_font = &SMALL_TEXT_FONT;
	editable_units_font = &SMALL_TEXT_FONT;
}

static void mainScreenOnDirtyClean() {
  // main screen mask
  // horizontal lines
  UG_DrawLine(0, 33, 319, 33, MAIN_SCREEN_FIELD_LABELS_COLOR);
  UG_DrawLine(0, 155, 319, 155, MAIN_SCREEN_FIELD_LABELS_COLOR);
  UG_DrawLine(0, 235, 319, 235, MAIN_SCREEN_FIELD_LABELS_COLOR);
  UG_DrawLine(0, 315, 319, 315, MAIN_SCREEN_FIELD_LABELS_COLOR);

  // vertical line
  UG_DrawLine(159, 156, 159, 314, MAIN_SCREEN_FIELD_LABELS_COLOR);

  UG_SetBackcolor(C_BLACK);
  UG_SetForecolor(MAIN_SCREEN_FIELD_LABELS_COLOR);
  UG_FontSelect(&FONT_10X16);
  UG_PutString(12, 46, "ASSIST");

  // wheel speed
  if(ui_vars.ui8_units_type == 0)
  {
    UG_PutString(260, 46 , "KM/H");
  }
  else
  {
    UG_PutString(265, 46 , "MPH");
  }
}

void mainScreenOnPostUpdate(void) {
  // because printing numbers of wheel speed will make dirty the dot, always print it
  // wheel speed print dot
//  UG_FillCircle(245, 130, 3, C_WHITE);
  UG_FillFrame(244, 129, 250, 135, C_WHITE);
}

/**
 * Appears at the bottom of all screens, includes status msgs or critical fault alerts
 * FIXME - get rid of this nasty define - instead add the concept of Subscreens, so that the battery bar
 * at the top and the status bar at the bottom can be shared across all screens
 */
#define STATUS_BAR \
{ \
    .x = 4, .y = SCREEN_HEIGHT - 18, \
    .width = 0, .height = -1, \
    .field = &warnField, \
    .font = &SMALL_TEXT_FONT, \
}

#define BATTERY_BAR \
  { \
      .x = 0, .y = 0, \
      .width = -1, .height = -1, \
      .field = &batteryField, \
      .font = &MY_FONT_BATTERY, \
  }, \
  { \
      .x = 8 + ((7 + 1 + 1) * 10) + (1 * 2) + 10, .y = 2, \
      .width = -5, .height = -1, \
      .font = &REGULAR_TEXT_FONT, \
      .align_x = AlignLeft, \
      .unit_align_x = AlignLeft, \
      .field = &socField \
  }, \
	{ \
		.x = 228, .y = 2, \
		.width = -5, .height = -1, \
		.font = &REGULAR_TEXT_FONT, \
		.unit_align_x = AlignRight, \
		.field = &timeField \
	}

//
// Screenscommon/src/state.c
//
Screen mainScreen = {
  .onPress = mainscreen_onpress,
  .onEnter = mainScreenOnEnter,
  .onDirtyClean = mainScreenOnDirtyClean,
  .onPostUpdate = mainScreenOnPostUpdate,
  .onCustomized = eeprom_write_variables,

  .fields = {
    BATTERY_BAR,
    {
      .x = 20, .y = 77,
      .width = 45, .height = -1,
      .field = &assistLevelField,
      .font = &BIG_NUMBERS_TEXT_FONT,
      .label_align_x = AlignHidden,
      .align_x = AlignCenter,
      .unit_align_x = AlignRight,
      .unit_align_y = AlignTop,
      .border = BorderNone,
    },
    {
      .x = 117, .y = 56,
      .width = 123, // 2 digits
      .height = 99,
      .field = &wheelSpeedIntegerField,
      .font = &HUGE_NUMBERS_TEXT_FONT,
      .label_align_x = AlignHidden,
      .align_x = AlignRight,
      .unit_align_x = AlignRight,
      .unit_align_y = AlignTop,
      .show_units = Hide,
      .border = BorderNone,
    },
    {
      .x = 253, .y = 77,
      .width = 45, // 1 digit
      .height = 72,
      .field = &wheelSpeedDecimalField,
      .font = &BIG_NUMBERS_TEXT_FONT,
      .label_align_x = AlignHidden,
      .align_x = AlignCenter,
      .unit_align_x = AlignCenter,
      .unit_align_y = AlignTop,
      .show_units = Hide,
      .border = BorderNone,
    },
    {
      .x = 1, .y = 161,
      .width = XbyEighths(4) - 4,
      .height = 72,
      .align_x = AlignRight,
      .inset_y = 12,
      .inset_x = 16, // space for 5 digits
      .field = &custom1,
      .font = &MEDIUM_NUMBERS_TEXT_FONT,
      .label_align_y = AlignTop,
      .border = BorderNone,
    },
    {
      .x = XbyEighths(4) + 1, .y = 161,
      .width = XbyEighths(4) - 4,
      .height = 72,
      .align_x = AlignRight,
      .inset_y = 12,
      .inset_x = 28, // space for 4 digits
      .field = &custom2,
      .font = &MEDIUM_NUMBERS_TEXT_FONT,
      .label_align_y = AlignTop,
      .border = BorderNone,
    },
    {
      .x = 1, .y = 240,
      .width = XbyEighths(4) - 4,
      .height = 72,
      .align_x = AlignRight,
      .inset_y = 12,
      .inset_x = 16, // space for 5 digits
      .field = &custom3,
      .font = &MEDIUM_NUMBERS_TEXT_FONT,
      .label_align_y = AlignTop,
      .border = BorderNone,
    },
    {
      .x = XbyEighths(4) + 1, .y = 240,
      .width = XbyEighths(4) - 4,
      .height = 72,
      .align_x = AlignRight,
      .inset_y = 12,
      .inset_x = 28, // space for 4 digits
      .field = &custom4,
      .font = &MEDIUM_NUMBERS_TEXT_FONT,
      .label_align_y = AlignTop,
      .border = BorderNone,
    },
    {
      .x = 0, .y = 322,
      .width = SCREEN_WIDTH, .height = 136,
      .field = &graphs,
    },
    STATUS_BAR,
    {
      .field = NULL
    }
  }
};


// Screens in a loop, shown when the user short presses the power button
Screen *screens[] = { &mainScreen, NULL };

// Show our battery graphic
void battery_display() {
	static uint8_t old_soc = 0xff;

	// Only trigger redraws if something changed
  if (ui_vars.ui8_battery_soc_symbol) {
    if (ui_vars.volt_based_soc != old_soc) {
      old_soc = ui_vars.volt_based_soc;
      batteryField.rw->dirty = true;
    }
  }
  else {
    if (ui16_g_battery_soc_watts_hour != old_soc) {
      old_soc = ui16_g_battery_soc_watts_hour;
      batteryField.rw->dirty = true;
    }
  }
}

void clock_time(void) {
  rtc_time_t *p_rtc_time;

  // get current time
  p_rtc_time = rtc_get_time();
  ui8_g_configuration_clock_hours = p_rtc_time->ui8_hours;
  ui8_g_configuration_clock_minutes = p_rtc_time->ui8_minutes;

  // force to be [0 - 12] depending on SI or Ipmerial units
  if (ui_vars.ui8_units_type) {
    if(ui8_g_configuration_clock_hours > 12) {
      ui8_g_configuration_clock_hours -= 12;
    }
  }
}
void onSetConfigurationClockHours(uint32_t v) {
  static rtc_time_t rtc_time;

  // save the new clock time
  rtc_time.ui8_hours = v;
  rtc_time.ui8_minutes = ui8_g_configuration_clock_minutes;
  rtc_set_time(&rtc_time);
}

void onSetConfigurationClockMinutes(uint32_t v) {
  static rtc_time_t rtc_time;

  // save the new clock time
  rtc_time.ui8_hours = ui8_g_configuration_clock_hours;
  rtc_time.ui8_minutes = v;
  rtc_set_time(&rtc_time);
}

void onSetConfigurationDisplayLcdBacklightOnBrightness(uint32_t v) {

  ui_vars.ui8_lcd_backlight_on_brightness = v;
  set_lcd_backlight();
}

void onSetConfigurationDisplayLcdBacklightOffBrightness(uint32_t v) {

  ui_vars.ui8_lcd_backlight_off_brightness = v;
  set_lcd_backlight();
}
