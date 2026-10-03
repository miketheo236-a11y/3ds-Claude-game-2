// Whispering Woods - a tiny forest adventure for the Nintendo 3DS.
// Find the 8 moon crystals to wake the Forest Spirit.
// Circle Pad / D-Pad: walk   B or R: sprint   START: quit   A: play again (after winning)
#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define TOP_W 400
#define TOP_H 240
#define WORLD_W 1600
#define WORLD_H 1200
#define NT 120 // trees
#define NC 8   // crystals
#define NF 30  // fireflies
#define Z 0.5f
#define RGB(r, g, b) C2D_Color32(r, g, b, 255)

static float tx[NT], ty[NT], cx[NC], cy[NC], fx[NF], fy[NF];
static bool got[NC];
static float pX, pY, t, msgT;
static int found;
static char msg[64];
static u32 seed = 2024;
static C2D_TextBuf tb;

static float rnd(void) { seed = seed * 1664525u + 1013904223u; return (seed >> 8) / 16777216.0f; }

static void reset(void) {
    pX = WORLD_W / 2; pY = WORLD_H / 2; found = 0; msgT = 0;
    for (int i = 0; i < NT; i++) {
        for (;;) {
            float x = 30 + rnd() * (WORLD_W - 60), y = 70 + rnd() * (WORLD_H - 100);
            bool ok = hypotf(x - pX, y - pY) > 90;
            for (int j = 0; j < i && ok; j++) if (hypotf(x - tx[j], y - ty[j]) < 34) ok = false;
            if (ok) { tx[i] = x; ty[i] = y; break; }
        }
    }
    for (int i = 0; i < NC; i++) {
        got[i] = false;
        for (;;) {
            float x = 40 + rnd() * (WORLD_W - 80), y = 40 + rnd() * (WORLD_H - 80);
            bool ok = hypotf(x - pX, y - pY) > 200;
            for (int j = 0; j < NT && ok; j++) if (hypotf(x - tx[j], y - ty[j]) < 30) ok = false;
            for (int j = 0; j < i && ok; j++) if (hypotf(x - cx[j], y - cy[j]) < 250) ok = false;
            if (ok) { cx[i] = x; cy[i] = y; break; }
        }
    }
    for (int i = 0; i < NF; i++) { fx[i] = rnd() * WORLD_W; fy[i] = rnd() * WORLD_H; }
}

static bool blocked(float x, float y) {
    if (x < 10 || y < 10 || x > WORLD_W - 10 || y > WORLD_H - 10) return true;
    for (int i = 0; i < NT; i++) if (hypotf(x - tx[i], y - ty[i]) < 13) return true;
    return false;
}

static void text(const char *s, float x, float y, float sc, u32 clr, bool center) {
    C2D_Text tt; float w, h;
    C2D_TextBufClear(tb);
    C2D_TextParse(&tt, tb, s);
    C2D_TextOptimize(&tt);
    C2D_TextGetDimensions(&tt, sc, sc, &w, &h);
    C2D_DrawText(&tt, C2D_WithColor, center ? x - w / 2 : x, y, 0.9f, sc, sc, clr);
}

static void pine(float x, float y) { // x,y = base of the trunk (screen coords)
    C2D_DrawRectSolid(x - 3, y - 8, Z, 6, 9, RGB(90, 60, 35));
    for (int i = 0; i < 3; i++) {
        float by = y - 6 - i * 15, hw = 17 - i * 4;
        u32 c = RGB(20 + i * 12, 95 + i * 18, 45 + i * 6);
        C2D_DrawTriangle(x - hw, by, c, x + hw, by, c, x, by - 26, c, Z);
    }
}

static void player(float x, float y, bool moving) {
    float b = moving ? sinf(t * 14) * 1.5f : 0;
    u32 hat = RGB(40, 120, 60);
    C2D_DrawCircleSolid(x, y, Z, 7, C2D_Color32(0, 0, 0, 70));
    C2D_DrawCircleSolid(x, y - 9 + b, Z, 7, RGB(170, 50, 40));
    C2D_DrawCircleSolid(x, y - 17 + b, Z, 5, RGB(240, 200, 160));
    C2D_DrawTriangle(x - 6, y - 18 + b, hat, x + 6, y - 18 + b, hat, x, y - 28 + b, hat, Z);
}

int main(void) {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    cfguInit();
    tb = C2D_TextBufNew(256);
    C3D_RenderTarget *top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget *bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    reset();
    snprintf(msg, sizeof msg, "Find the 8 moon crystals!");
    msgT = 5;

    while (aptMainLoop()) {
        hidScanInput();
        u32 down = hidKeysDown(), held = hidKeysHeld();
        if (down & KEY_START) break;
        t += 1.0f / 60; if (msgT > 0) msgT -= 1.0f / 60;
        if (found == NC && (down & KEY_A)) { reset(); snprintf(msg, sizeof msg, "A new forest awaits..."); msgT = 4; }

        circlePosition cp; hidCircleRead(&cp);
        float mx = 0, my = 0;
        if (abs(cp.dx) > 20) mx = cp.dx / 156.0f;
        if (abs(cp.dy) > 20) my = -cp.dy / 156.0f;
        if (held & KEY_DLEFT) mx = -1;
        if (held & KEY_DRIGHT) mx = 1;
        if (held & KEY_DUP) my = -1;
        if (held & KEY_DDOWN) my = 1;
        float len = hypotf(mx, my);
        if (len > 1) { mx /= len; my /= len; }
        float sp = (held & (KEY_B | KEY_R)) ? 3.2f : 1.9f;
        float nx = pX + mx * sp, ny = pY + my * sp;
        if (!blocked(nx, pY)) pX = nx;
        if (!blocked(pX, ny)) pY = ny;

        for (int i = 0; i < NC; i++) {
            if (!got[i] && hypotf(pX - cx[i], pY - cy[i]) < 16) {
                got[i] = true; found++; msgT = 3;
                if (found == NC) snprintf(msg, sizeof msg, "The last crystal! The forest hums...");
                else snprintf(msg, sizeof msg, "Moon crystal found! (%d/%d)", found, NC);
            }
        }

        float camX = fminf(fmaxf(pX - TOP_W / 2, 0), WORLD_W - TOP_W);
        float camY = fminf(fmaxf(pY - TOP_H / 2, 0), WORLD_H - TOP_H);

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(top, RGB(44, 108, 56));
        C2D_SceneBegin(top);
        for (int gy = (int)(camY / 40); gy <= (int)((camY + TOP_H) / 40); gy++)
            for (int gx = (int)(camX / 40); gx <= (int)((camX + TOP_W) / 40); gx++)
                if ((gx + gy) & 1) C2D_DrawRectSolid(gx * 40 - camX, gy * 40 - camY, Z, 40, 40, RGB(36, 92, 48));

        for (int i = 0; i < NC; i++) {
            if (got[i]) continue;
            float sx = cx[i] - camX, sy = cy[i] - camY, p = 1 + 0.15f * sinf(t * 4 + i);
            u32 hi = RGB(160, 225, 255), lo = RGB(70, 140, 230);
            C2D_DrawCircleSolid(sx, sy - 10, Z, 16 * p, C2D_Color32(120, 200, 255, 50));
            C2D_DrawTriangle(sx - 6, sy - 10, lo, sx + 6, sy - 10, lo, sx, sy - 24, hi, Z);
            C2D_DrawTriangle(sx - 6, sy - 10, lo, sx + 6, sy - 10, lo, sx, sy, hi, Z);
        }
        for (int pass = 0; pass < 2; pass++) { // trees behind the player, player, trees in front
            if (pass == 1) player(pX - camX, pY - camY, len > 0.1f);
            for (int i = 0; i < NT; i++) {
                float sx = tx[i] - camX, sy = ty[i] - camY;
                if (sx < -40 || sx > TOP_W + 40 || sy < -10 || sy > TOP_H + 70) continue;
                if ((ty[i] <= pY) == (pass == 0)) pine(sx, sy);
            }
        }
        for (int i = 0; i < NF; i++) {
            float sx = fx[i] + sinf(t * 0.7f + i) * 20 - camX, sy = fy[i] + cosf(t * 0.5f + i * 2) * 15 - camY;
            int a = 110 + (int)(100 * sinf(t * 3 + i));
            C2D_DrawCircleSolid(sx, sy, Z, 5, C2D_Color32(255, 240, 120, a / 4));
            C2D_DrawCircleSolid(sx, sy, Z, 1.5f, C2D_Color32(255, 250, 170, a));
        }
        if (msgT > 0) {
            C2D_DrawRectSolid(0, 0, 0.8f, TOP_W, 26, C2D_Color32(0, 0, 0, 150));
            text(msg, TOP_W / 2, 4, 0.6f, RGB(255, 255, 255), true);
        }
        if (found == NC) {
            C2D_DrawRectSolid(0, 80, 0.8f, TOP_W, 80, C2D_Color32(0, 0, 0, 170));
            text("The Forest Spirit awakens!", TOP_W / 2, 92, 0.8f, RGB(160, 225, 255), true);
            text("A: wander again    START: quit", TOP_W / 2, 128, 0.5f, RGB(255, 255, 255), true);
        }

        C2D_TargetClear(bot, RGB(22, 30, 24));
        C2D_SceneBegin(bot);
        float s = 0.15f, mmx = 40, mmy = 34;
        text("WHISPERING WOODS", 160, 4, 0.6f, RGB(255, 235, 150), true);
        C2D_DrawRectSolid(mmx, mmy, Z, WORLD_W * s, WORLD_H * s, RGB(40, 80, 46));
        for (int i = 0; i < NT; i++) C2D_DrawRectSolid(mmx + tx[i] * s, mmy + ty[i] * s, Z, 2, 2, RGB(20, 60, 30));
        for (int i = 0; i < NC; i++) if (!got[i]) C2D_DrawCircleSolid(mmx + cx[i] * s, mmy + cy[i] * s, Z, 2.5f, RGB(120, 210, 255));
        if (((int)(t * 4)) & 1) C2D_DrawCircleSolid(mmx + pX * s, mmy + pY * s, Z, 3, RGB(255, 220, 60));
        char buf[40]; snprintf(buf, sizeof buf, "Crystals: %d / %d", found, NC);
        text(buf, 160, 218, 0.6f, RGB(160, 225, 255), true);
        C3D_FrameEnd(0);
    }

    C2D_TextBufDelete(tb);
    cfguExit();
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
