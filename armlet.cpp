#pragma once
#include <windows.h>
#include <chrono>
#include <thread>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cstdlib>


namespace pedik {
static const uintptr_t CHUNK_TABLE = 0x10;
static const int STRIDE = 0x70;
static const int CHUNK = 0x200;
static const int NCHUNK = 0x40;
static const uintptr_t ENT_PENTITY = 0x10;
static const uintptr_t ID_HANDLE = 0x10;
static const uintptr_t ID_NAME = 0x18;
static const uintptr_t ID_DESIGNER = 0x20;
static const uintptr_t CTRL_ASSIGNED = 0x90C;
static const uintptr_t CTRL_PID = 0x908;
static const uintptr_t CTRL_ORDERS = 0x998;
static const uintptr_t CTRL_SEQ = 0x9B0;
static const uintptr_t E_HEALTH = 0x34C;
static const uintptr_t E_MAXHEALTH = 0x348;
static const uintptr_t E_TEAM = 0x3E7;
static const uintptr_t E_LIFE = 0x354;
static const uintptr_t NPC_TYPE = 0xB94;
static const uintptr_t NPC_LEVEL = 0xBAC;
static const uintptr_t ITEM_OWNER = 0x724;
static const int ARMLET_LINE = 300;
static const int CD_MS = 1000;
static bool isOn = false;
static long long lastMs = 0;


long long nowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}
bool r64(uintptr_t a, uintptr_t& o) {
    __try { o = *(uintptr_t volatile*)a; return true; }
    __except (EXCEPTION_EXECUTE_HANDLER) { o = 0; return false; }
}
bool r32(uintptr_t a, uint32_t& o) {
    __try { o = *(uint32_t volatile*)a; return true; }
    __except (EXCEPTION_EXECUTE_HANDLER) { o = 0; return false; }
}
bool ri32(uintptr_t a, int32_t& o) {
    __try { o = *(int32_t volatile*)a; return true; }
    __except (EXCEPTION_EXECUTE_HANDLER) { o = 0; return false; }
}
bool validEnt(uintptr_t e) {
    if (!e) return false;
    uintptr_t id = 0, back = 0;
    if (!r64(e + ENT_PENTITY, id) || !id) return false;
    if (!r64(id, back)) return false;
    return back == e;
}
uintptr_t byHandle(const std::vector<uintptr_t>& v, uint32_t h) {
    if (!h || h == 0xFFFFFFFF) return 0;
    for (uintptr_t e : v) {
        uintptr_t id = 0;
        if (!r64(e + ENT_PENTITY, id) || !id) continue;
        uint32_t cur = 0;
        if (!r32(id + ID_HANDLE, cur)) continue;
        if (cur == h && validEnt(e)) return e;
    }
    return 0;
}
uintptr_t localHero(const std::vector<uintptr_t>& v) {
    for (uintptr_t e : v) {
        uint32_t a = 0;
        r32(e + CTRL_ASSIGNED, a);
        if (!a || a == 0xFFFFFFFF) continue;
        uintptr_t c = byHandle(v, a);
        if (!c) continue;
        uint32_t t = 0;
        r32(c + NPC_TYPE, t);
        if (t == 1) return c;
    }
    return 0;
}
bool pushToggle(uintptr_t ctrl, uint32_t abIdx, uint32_t heroIdx) {
    uintptr_t vb = ctrl + CTRL_ORDERS;
    uintptr_t mem = 0;
    int32_t alloc = -1, size = -1;
    r64(vb, mem);
    ri32(vb + 0x8, alloc);
    ri32(vb + 0x10, size);
    if (size < 0 || size > 16 || alloc < size) return false;
    if (size > 0 && (!mem || !(mem >> 32))) return false;
    if (size >= alloc) {
        if (size != 0) return false;
        void* arr = malloc(8 * 0x40);
        if (!arr) return false;
        memset(arr, 0, 8 * 0x40);
        *(uintptr_t*)(vb + 0x0) = (uintptr_t)arr;
        *(int32_t*)(vb + 0x8) = 8;
        mem = (uintptr_t)arr;
        size = 0;
    }
    int32_t* u = (int32_t*)malloc(4);
    if (!u) return false;
    *u = (int32_t)heroIdx;
    uint8_t raw[0x40];
    memset(raw, 0, sizeof(raw));
    memcpy(raw + 0x00, &u, 8);
    *(int32_t*)(raw + 0x08) = 1;
    *(int32_t*)(raw + 0x10) = 1;
    int32_t pid = 0, seq = 0;
    ri32(ctrl + CTRL_PID, pid);
    ri32(ctrl + CTRL_SEQ, seq);
    *(int32_t*)(raw + 0x24) = pid;
    *(int32_t*)(raw + 0x28) = seq;
    *(int32_t*)(raw + 0x2C) = 8;
    *(uint32_t*)(raw + 0x34) = abIdx;
    memcpy((void*)(mem + (uintptr_t)size * 0x40), raw, 0x40);
    *(int32_t*)(vb + 0x10) = size + 1;
    *(int32_t*)(ctrl + CTRL_SEQ) = seq + 1;
    return true;
}


bool findArmlet(const std::vector<uintptr_t>& v, int pid, uint32_t& abIdx) {
    for (uintptr_t e : v) {
        uintptr_t id = 0;
        if (!r64(e + ENT_PENTITY, id) || !id) continue;
        uintptr_t np = 0;
        if (!r64(id + ID_DESIGNER, np)) continue;
        if (!np || !(np >> 32)) continue;
        char nm[16] = {};
        __try { memcpy(nm, (void*)np, 11); }
        __except (EXCEPTION_EXECUTE_HANDLER) { continue; }
        if (memcmp(nm, "item_armlet", 11) != 0) continue;
        int32_t ow = -999;
        ri32(e + ITEM_OWNER, ow);
        if (ow != pid) continue;
        uint32_t h = 0;
        r32(id + ID_HANDLE, h);
        if (!h || h == 0xFFFFFFFF) continue;
        abIdx = h & 0x7FFF;
        return true;
    }
    return false;
}
void tick(const std::vector<uintptr_t>& v) {
    uintptr_t hero = localHero(v);
    if (!hero) return;
    int32_t hp = 0, maxHp = 0;
    ri32(hero + E_HEALTH, hp);
    ri32(hero + E_MAXHEALTH, maxHp);
    if (hp <= 0) return;
    uintptr_t hid = 0;
    r64(hero + ENT_PENTITY, hid);
    uint32_t hh = 0;
    if (hid) r32(hid + ID_HANDLE, hh);
    if (!hh) return;
    uintptr_t ctrl = 0;
    for (uintptr_t e : v) {
        uint32_t a = 0;
        r32(e + CTRL_ASSIGNED, a);
        if (a && a == hh) { ctrl = e; break; }
    }
    if (!ctrl) return;
    int32_t pid = -1;
    ri32(ctrl + CTRL_PID, pid);


    uint32_t ab = 0;
    if (!findArmlet(v, pid, ab)) {
        isOn = false;
        return;
    }
    uint32_t hix = hh & 0x7FFF;
    long long now = nowMs();
    if (now - lastMs < CD_MS) return;


    if (!isOn && hp < ARMLET_LINE) {
        if (pushToggle(ctrl, ab, hix)) {
            isOn = true;
            lastMs = now;
        }
    } else if (isOn && hp >= maxHp - 50) {
        if (pushToggle(ctrl, ab, hix)) {
            isOn = false;
            lastMs = now;
        }
    }
}


}
