#pragma once
// Revision K wiring: compile with -DREHA_DESKTOP_PANEL. Default remains BOOT-only.
#ifdef REHA_DESKTOP_PANEL
#if defined(REHA_PANEL) || defined(REHA_NAV_BUTTONS) || defined(REHA_VOLUME_PIN) || defined(REHA_RECORD_LED_PIN) || defined(REHA_PLAY_LED_PIN)
#error "Desktop encoder panel cannot be combined with legacy pot/LED profiles"
#endif
#define REHA_PANEL_WIRING_REV "K"
#define REHA_RECORD_PIN 1
#define REHA_PLAY_PIN 7
#define REHA_TAKE_PIN 0
#define REHA_SONG_PIN 3
#define REHA_PANEL_BACK_PIN 9
#define REHA_PANEL_RECORD_LED 10
#define REHA_LEVEL_A 6
#define REHA_LEVEL_B 5
#define REHA_LEVEL_PUSH 4
#define REHA_SDA 18
#define REHA_SCL 19
#ifndef REHA_ENCODER_EDGES
#define REHA_ENCODER_EDGES 4
#endif
#ifndef REHA_LEVEL_DIRECTION
#define REHA_LEVEL_DIRECTION 1
#endif
static_assert(REHA_ENCODER_EDGES == 2 || REHA_ENCODER_EDGES == 4, "Encoder edges must be 2 or 4");
static_assert(REHA_LEVEL_DIRECTION == 1 || REHA_LEVEL_DIRECTION == -1, "Volume direction must be +/-1");
#endif
// No external wiring enabled by default. Define REHA_PANEL only after bench
// verification of the proposed low-voltage switch and LED driver circuit.
#ifdef REHA_PANEL
#ifndef REHA_VOLUME_PIN
#define REHA_VOLUME_PIN 0
#endif
#ifndef REHA_RECORD_PIN
#define REHA_RECORD_PIN 1
#endif
#ifndef REHA_PLAY_PIN
#define REHA_PLAY_PIN 3
#endif
#ifndef REHA_RECORD_LED_PIN
#define REHA_RECORD_LED_PIN 4
#endif
#ifndef REHA_PLAY_LED_PIN
#define REHA_PLAY_LED_PIN 10
#endif
#endif
#ifndef REHA_RECORD_PIN
#define REHA_RECORD_PIN 9 // onboard BOOT
#endif
// Define REHA_RECORD_NC or REHA_PLAY_NC only for a verified NC switch input.
#ifdef REHA_NAV_BUTTONS
#define REHA_BACK_PIN 5
#define REHA_FORWARD_PIN 6
#define REHA_SHIFT_PIN 7
#endif
