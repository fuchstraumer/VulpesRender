#ifndef VULPES_VK_KEYCODES_H
#define VULPES_VK_KEYCODES_H
#include "vpr_stdafx.h"

#if defined(__linux__)
#include <xcb/xcb.h>
#elif defined(_WIN32) 
#define GLFW_INCLUDE_VULKAN
#include "glfw/glfw3.h"
#endif

namespace vulpes {

    struct KeyMap {

        constexpr KeyMap(const int& api_code);        
        // added as rows across keyboard.
        size_t KEY_ESCAPE, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12, KEY_PRINT_SCREEN, KEY_PAUSE_BREAK;
        size_t KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, KEY_HYPHEN, KEY_PLUS, KEY_BACKSPACE;
        size_t KEY_TAB, KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P, KEY_LEFT_BRACKET, KEY_RIGHT_BRACKET, KEY_BACKSLASH;
        size_t KEY_CAPS_LOCK, KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L, KEY_SEMICOLON, KEY_APOSTROPHE, KEY_ENTER;
        size_t KEY_L_SHIFT, KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M, KEY_COMMA, KEY_PERIOD, KEY_FORWARD_SLASH, KEY_R_SHIFT;
        size_t KEY_L_CTRL, KEY_L_ALT, KEY_SPACE, KEY_R_ALT, KEY_R_CTRL;
    };

    KeyMap::KeyMap(const int& api_code) {
        if constexpr(api_code == 0) {
            // GLFW
            KEY_1 = GLFW_KEY_1;
            KEY_2 = GLFW_KEY_2;
            KEY_3 = GLFW_KEY_3;
            KEY_4 = GLFW_KEY_4;
            KEY_5 = GLFW_KEY_5;
            KEY_6 = GLFW_KEY_6;
            KEY_7 = GLFW_KEY_7;
            KEY_8 = GLFW_KEY_8;
            KEY_9 = GLFW_KEY_9;
            KEY_0 = GLFW_KEY_0;
            KEY_TAB = GLFW_KEY_TAB;
            KEY_Q = GLFW_KEY_Q;
            KEY_W = GLFW_KEY_W;
            KEY_E = GLFW_KEY_E;
            KEY_R = GLFW_KEY_R;
            KEY_T = GLFW_KEY_T;
            KEY_Y = GLFW_KEY_Y;
            KEY_U = GLFW_KEY_U;
            KEY_I = GLFW_KEY_I;
            KEY_O = GLFW_KEY_O;
            KEY_P = GLFW_KEY_P;
            KEY_LEFT_BRACKET = GLFW_KEY_LEFT_BRACKET;
            KEY_RIGHT_BRACKET = GLFW_KEY_RIGHT_BRACKET;
            KEY_BACKSLASH = GLFW_KEY_BACKSLASH;
            KEY_A = GLFW_KEY_A;
            KEY_S = GLFW_KEY_S;
            KEY_D = GLFW_KEY_D;
            KEY_F = GLFW_KEY_F;
            KEY_G = GLFW_KEY_G;
            KEY_H = GLFW_KEY_H;
            KEY_J = GLFW_KEY_J;
            KEY_K = GLFW_KEY_K;
            KEY_L = GLFW_KEY_L;
            KEY_L_SHIFT = GLFW_KEY_LEFT_SHIFT;
            KEY_Z = GLFW_KEY_Z;
            KEY_X = GLFW_KEY_X;
            KEY_C = GLFW_KEY_C;
            KEY_V = GLFW_KEY_V;
            KEY_B = GLFW_KEY_B;
            KEY_N = GLFW_KEY_N;
            KEY_M = GLFW_KEY_M;
            KEY_COMMA = GLFW_KEY_COMMA;
            KEY_FORWARD_SLASH = GLFW_KEY_SLASH;
            KEY_R_SHIFT = GLFW_KEY_RIGHT_SHIFT;
            KEY_SEMICOLON = GLFW_KEY_SEMICOLON;
            KEY_APOSTROPHE = GLFW_KEY_APOSTROPHE;
            KEY_ENTER = GLFW_KEY_ENTER;
            KEY_L_CTRL = GLFW_KEY_LEFT_CTRL;
            KEY_L_ALT = GLFW_KEY_LEFT_ALT;
            KEY_SPACE = GLFW_KEY_SPACE;
            KEY_R_ALT = GLFW_KEY_RIGHT_ALT;
            KEY_R_CTRL = GLFW_KEY_RIGHT_SHIFT;
        }
        else if constexpr(api_code == 1) {
            //xcb/linux
        }
        else if constexpr(api_code == 1) {

        }
    }
}

#endif 