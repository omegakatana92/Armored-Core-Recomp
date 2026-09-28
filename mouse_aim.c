/*
 * Armored Core (USA, SCUS-94182) — mouse aim plugin for PSXRecomp.
 *
 * Every guest VBlank, while the mouse is captured, this adds the mouse
 * movement since the last VBlank to the player mech's facing and look angles
 * in guest RAM. The game keeps running its own logic and simply finds the
 * mech pointing where the mouse put it.
 *
 * Addresses (found with DuckStation's memory scanner, stable across missions):
 *
 *   0x801A26CA  facing (yaw), u16. 4096 units per full turn; the game uses the
 *               value modulo 4096 and lets the halfword wrap freely.
 *               0 = N, 1024 = W, 2048 = S, 3072 = E (grows counter-clockwise).
 *   0x800411C0  look (pitch), s16. -768 = fully up, +768 = fully down.
 *
 * F10 toggles mouse capture. Capture is released automatically when the
 * window loses focus. Nothing is written while the mouse is not captured.
 *
 * Runs on the thread that pumps SDL events (the VBlank callback is invoked
 * from the emulated GPU on the same thread as presentation), so reading SDL
 * mouse/keyboard state here is safe.
 */
#include "mod_plugins.h"
#include "psx_sdl.h"

#include <stdlib.h>
#include <string.h>

#define PKG_ID     "armoredcore.mouse-aim"
#define FEATURE_ID "mouse-aim"
#define PLUGIN_ID  "armoredcore.mouse-aim"

#define ADDR_YAW   0x801A26CAu
#define ADDR_PITCH 0x800411C0u

#define PITCH_MIN (-768)
#define PITCH_MAX 768

/* Angle units per mouse count at 100% sensitivity. 4096 units = 360 degrees,
 * so 1.0 is roughly 0.09 degrees per count. */
#define BASE_UNITS_PER_COUNT 1.0f

#define TOGGLE_KEY SDL_SCANCODE_F10

static float g_sens_x = BASE_UNITS_PER_COUNT;
static float g_sens_y = BASE_UNITS_PER_COUNT;
static int   g_invert_y = 0;

static int   g_captured = 0;
static int   g_toggle_was_down = 0;
static float g_carry_x = 0.0f; /* sub-unit remainders, so slow moves still count */
static float g_carry_y = 0.0f;

/* ---- options ------------------------------------------------------------ */

static long option_number(const char* id, long fallback, long lo, long hi) {
    char text[32] = "";
    if (!psx_mod_option_value(PKG_ID, FEATURE_ID, id, text, sizeof text) ||
        !text[0])
        return fallback;
    char* end = NULL;
    long v = strtol(text, &end, 10);
    if (!end || *end != '\0') return fallback;
    if (v < lo) v = lo;
    if (v > hi) v = hi;
    return v;
}

static int option_flag(const char* id) {
    char text[16] = "";
    return psx_mod_option_value(PKG_ID, FEATURE_ID, id, text, sizeof text) &&
           strcmp(text, "true") == 0;
}

/* ---- SDL2 / SDL3 differences -------------------------------------------- */

static void set_relative_mode(int on) {
#if defined(PSX_SDL3)
    SDL_Window* w = SDL_GetKeyboardFocus();
    if (w) (void)SDL_SetWindowRelativeMouseMode(w, on ? true : false);
#else
    (void)SDL_SetRelativeMouseMode(on ? SDL_TRUE : SDL_FALSE);
#endif
}

static void read_mouse_delta(float* dx, float* dy) {
#if defined(PSX_SDL3)
    float x = 0.0f, y = 0.0f;
    (void)SDL_GetRelativeMouseState(&x, &y);
    *dx = x;
    *dy = y;
#else
    int x = 0, y = 0;
    (void)SDL_GetRelativeMouseState(&x, &y);
    *dx = (float)x;
    *dy = (float)y;
#endif
}

static void set_captured(int on) {
    if (on == g_captured) return;
    g_captured = on;
    set_relative_mode(on);
    /* Drop whatever movement piled up before the switch. */
    float dx, dy;
    read_mouse_delta(&dx, &dy);
    g_carry_x = g_carry_y = 0.0f;
}

/* Take the whole-unit part of an accumulated float, keep the remainder. */
static int take_units(float* carry, float add) {
    *carry += add;
    int whole = (int)*carry; /* truncates toward zero */
    *carry -= (float)whole;
    return whole;
}

/* ---- plugin callbacks ---------------------------------------------------- */

static void mouse_aim_activate(void) {
    const float pct_x = (float)option_number("sensitivity", 100, 10, 1000);
    const float pct_y = (float)option_number("vertical_sensitivity", 100, 10, 1000);
    g_sens_x = BASE_UNITS_PER_COUNT * pct_x / 100.0f;
    g_sens_y = g_sens_x * pct_y / 100.0f;
    g_invert_y = option_flag("invert_y");
}

static void mouse_aim_vblank(void) {
    /* F10: toggle capture on the key's press edge. */
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    const int toggle_down = keys ? keys[TOGGLE_KEY] != 0 : 0;
    const int has_focus = SDL_GetKeyboardFocus() != NULL;

    if (toggle_down && !g_toggle_was_down && has_focus)
        set_captured(!g_captured);
    g_toggle_was_down = toggle_down;

    if (g_captured && !has_focus) set_captured(0);
    if (!g_captured || !psx_mod_game_started()) return;

    float dx, dy;
    read_mouse_delta(&dx, &dy);
    if (dx == 0.0f && dy == 0.0f) return;

    /* Guard: the look angle only lives in -768..768 during a mission. If it
     * reads as anything else we are probably in the garage or a menu, where
     * these addresses may hold other data, so write nothing. */
    int pitch = (int)(int16_t)psx_mod_read_half(ADDR_PITCH);
    if (pitch < PITCH_MIN || pitch > PITCH_MAX) return;

    /* Mouse right = turn clockwise = facing value goes down. */
    const int yaw_step = take_units(&g_carry_x, -dx * g_sens_x);
    if (yaw_step != 0) {
        const uint16_t yaw = psx_mod_read_half(ADDR_YAW);
        psx_mod_write_half(ADDR_YAW, (uint16_t)(yaw + (uint16_t)yaw_step));
    }

    /* Mouse down (positive dy) = look down = pitch goes up. */
    const float y_in = g_invert_y ? -dy : dy;
    const int pitch_step = take_units(&g_carry_y, y_in * g_sens_y);
    if (pitch_step != 0) {
        pitch += pitch_step;
        if (pitch < PITCH_MIN) pitch = PITCH_MIN;
        if (pitch > PITCH_MAX) pitch = PITCH_MAX;
        psx_mod_write_half(ADDR_PITCH, (uint16_t)(int16_t)pitch);
    }
}

PSX_MOD_CONSTRUCTOR(armoredcore_register_mouse_aim_plugin) {
    (void)psx_mod_register_activation_plugin(PLUGIN_ID, mouse_aim_activate);
    (void)psx_mod_register_vblank_plugin(PLUGIN_ID, mouse_aim_vblank);
}
