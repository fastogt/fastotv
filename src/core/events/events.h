#pragma once

#include "core/events/events_base.h"

namespace core {
struct VideoFrame;
class VideoState;
}

enum Scancode {
  SCANCODE_UNKNOWN = 0,

  /**
   *  \name Usage page 0x07
   *
   *  These values are from usage page 0x07 (USB keyboard page).
   */
  /* @{ */

  SCANCODE_A = 4,
  SCANCODE_B = 5,
  SCANCODE_C = 6,
  SCANCODE_D = 7,
  SCANCODE_E = 8,
  SCANCODE_F = 9,
  SCANCODE_G = 10,
  SCANCODE_H = 11,
  SCANCODE_I = 12,
  SCANCODE_J = 13,
  SCANCODE_K = 14,
  SCANCODE_L = 15,
  SCANCODE_M = 16,
  SCANCODE_N = 17,
  SCANCODE_O = 18,
  SCANCODE_P = 19,
  SCANCODE_Q = 20,
  SCANCODE_R = 21,
  SCANCODE_S = 22,
  SCANCODE_T = 23,
  SCANCODE_U = 24,
  SCANCODE_V = 25,
  SCANCODE_W = 26,
  SCANCODE_X = 27,
  SCANCODE_Y = 28,
  SCANCODE_Z = 29,

  SCANCODE_1 = 30,
  SCANCODE_2 = 31,
  SCANCODE_3 = 32,
  SCANCODE_4 = 33,
  SCANCODE_5 = 34,
  SCANCODE_6 = 35,
  SCANCODE_7 = 36,
  SCANCODE_8 = 37,
  SCANCODE_9 = 38,
  SCANCODE_0 = 39,

  SCANCODE_RETURN = 40,
  SCANCODE_ESCAPE = 41,
  SCANCODE_BACKSPACE = 42,
  SCANCODE_TAB = 43,
  SCANCODE_SPACE = 44,

  SCANCODE_MINUS = 45,
  SCANCODE_EQUALS = 46,
  SCANCODE_LEFTBRACKET = 47,
  SCANCODE_RIGHTBRACKET = 48,
  SCANCODE_BACKSLASH = 49, /**< Located at the lower left of the return
                                *   key on ISO keyboards and at the right end
                                *   of the QWERTY row on ANSI keyboards.
                                *   Produces REVERSE SOLIDUS (backslash) and
                                *   VERTICAL LINE in a US layout, REVERSE
                                *   SOLIDUS and VERTICAL LINE in a UK Mac
                                *   layout, NUMBER SIGN and TILDE in a UK
                                *   Windows layout, DOLLAR SIGN and POUND SIGN
                                *   in a Swiss German layout, NUMBER SIGN and
                                *   APOSTROPHE in a German layout, GRAVE
                                *   ACCENT and POUND SIGN in a French Mac
                                *   layout, and ASTERISK and MICRO SIGN in a
                                *   French Windows layout.
                                */
  SCANCODE_NONUSHASH = 50, /**< ISO USB keyboards actually use this code
                                *   instead of 49 for the same key, but all
                                *   OSes I've seen treat the two codes
                                *   identically. So, as an implementor, unless
                                *   your keyboard generates both of those
                                *   codes and your OS treats them differently,
                                *   you should generate SCANCODE_BACKSLASH
                                *   instead of this code. As a user, you
                                *   should not rely on this code because SDL
                                *   will never generate it with most (all?)
                                *   keyboards.
                                */
  SCANCODE_SEMICOLON = 51,
  SCANCODE_APOSTROPHE = 52,
  SCANCODE_GRAVE = 53, /**< Located in the top left corner (on both ANSI
                            *   and ISO keyboards). Produces GRAVE ACCENT and
                            *   TILDE in a US Windows layout and in US and UK
                            *   Mac layouts on ANSI keyboards, GRAVE ACCENT
                            *   and NOT SIGN in a UK Windows layout, SECTION
                            *   SIGN and PLUS-MINUS SIGN in US and UK Mac
                            *   layouts on ISO keyboards, SECTION SIGN and
                            *   DEGREE SIGN in a Swiss German layout (Mac:
                            *   only on ISO keyboards), CIRCUMFLEX ACCENT and
                            *   DEGREE SIGN in a German layout (Mac: only on
                            *   ISO keyboards), SUPERSCRIPT TWO and TILDE in a
                            *   French Windows layout, COMMERCIAL AT and
                            *   NUMBER SIGN in a French Mac layout on ISO
                            *   keyboards, and LESS-THAN SIGN and GREATER-THAN
                            *   SIGN in a Swiss German, German, or French Mac
                            *   layout on ANSI keyboards.
                            */
  SCANCODE_COMMA = 54,
  SCANCODE_PERIOD = 55,
  SCANCODE_SLASH = 56,

  SCANCODE_CAPSLOCK = 57,

  SCANCODE_F1 = 58,
  SCANCODE_F2 = 59,
  SCANCODE_F3 = 60,
  SCANCODE_F4 = 61,
  SCANCODE_F5 = 62,
  SCANCODE_F6 = 63,
  SCANCODE_F7 = 64,
  SCANCODE_F8 = 65,
  SCANCODE_F9 = 66,
  SCANCODE_F10 = 67,
  SCANCODE_F11 = 68,
  SCANCODE_F12 = 69,

  SCANCODE_PRINTSCREEN = 70,
  SCANCODE_SCROLLLOCK = 71,
  SCANCODE_PAUSE = 72,
  SCANCODE_INSERT = 73, /**< insert on PC, help on some Mac keyboards (but
                                 does send code 73, not 117) */
  SCANCODE_HOME = 74,
  SCANCODE_PAGEUP = 75,
  SCANCODE_DELETE = 76,
  SCANCODE_END = 77,
  SCANCODE_PAGEDOWN = 78,
  SCANCODE_RIGHT = 79,
  SCANCODE_LEFT = 80,
  SCANCODE_DOWN = 81,
  SCANCODE_UP = 82,

  SCANCODE_NUMLOCKCLEAR = 83, /**< num lock on PC, clear on Mac keyboards
                                   */
  SCANCODE_KP_DIVIDE = 84,
  SCANCODE_KP_MULTIPLY = 85,
  SCANCODE_KP_MINUS = 86,
  SCANCODE_KP_PLUS = 87,
  SCANCODE_KP_ENTER = 88,
  SCANCODE_KP_1 = 89,
  SCANCODE_KP_2 = 90,
  SCANCODE_KP_3 = 91,
  SCANCODE_KP_4 = 92,
  SCANCODE_KP_5 = 93,
  SCANCODE_KP_6 = 94,
  SCANCODE_KP_7 = 95,
  SCANCODE_KP_8 = 96,
  SCANCODE_KP_9 = 97,
  SCANCODE_KP_0 = 98,
  SCANCODE_KP_PERIOD = 99,

  SCANCODE_NONUSBACKSLASH = 100, /**< This is the additional key that ISO
                                      *   keyboards have over ANSI ones,
                                      *   located between left shift and Y.
                                      *   Produces GRAVE ACCENT and TILDE in a
                                      *   US or UK Mac layout, REVERSE SOLIDUS
                                      *   (backslash) and VERTICAL LINE in a
                                      *   US or UK Windows layout, and
                                      *   LESS-THAN SIGN and GREATER-THAN SIGN
                                      *   in a Swiss German, German, or French
                                      *   layout. */
  SCANCODE_APPLICATION = 101,    /**< windows contextual menu, compose */
  SCANCODE_POWER = 102,          /**< The USB document says this is a status flag,
                                      *   not a physical key - but some Mac keyboards
                                      *   do have a power key. */
  SCANCODE_KP_EQUALS = 103,
  SCANCODE_F13 = 104,
  SCANCODE_F14 = 105,
  SCANCODE_F15 = 106,
  SCANCODE_F16 = 107,
  SCANCODE_F17 = 108,
  SCANCODE_F18 = 109,
  SCANCODE_F19 = 110,
  SCANCODE_F20 = 111,
  SCANCODE_F21 = 112,
  SCANCODE_F22 = 113,
  SCANCODE_F23 = 114,
  SCANCODE_F24 = 115,
  SCANCODE_EXECUTE = 116,
  SCANCODE_HELP = 117,
  SCANCODE_MENU = 118,
  SCANCODE_SELECT = 119,
  SCANCODE_STOP = 120,
  SCANCODE_AGAIN = 121, /**< redo */
  SCANCODE_UNDO = 122,
  SCANCODE_CUT = 123,
  SCANCODE_COPY = 124,
  SCANCODE_PASTE = 125,
  SCANCODE_FIND = 126,
  SCANCODE_MUTE = 127,
  SCANCODE_VOLUMEUP = 128,
  SCANCODE_VOLUMEDOWN = 129,
  /* not sure whether there's a reason to enable these */
  /*     SCANCODE_LOCKINGCAPSLOCK = 130,  */
  /*     SCANCODE_LOCKINGNUMLOCK = 131, */
  /*     SCANCODE_LOCKINGSCROLLLOCK = 132, */
  SCANCODE_KP_COMMA = 133,
  SCANCODE_KP_EQUALSAS400 = 134,

  SCANCODE_INTERNATIONAL1 = 135, /**< used on Asian keyboards, see
                                          footnotes in USB doc */
  SCANCODE_INTERNATIONAL2 = 136,
  SCANCODE_INTERNATIONAL3 = 137, /**< Yen */
  SCANCODE_INTERNATIONAL4 = 138,
  SCANCODE_INTERNATIONAL5 = 139,
  SCANCODE_INTERNATIONAL6 = 140,
  SCANCODE_INTERNATIONAL7 = 141,
  SCANCODE_INTERNATIONAL8 = 142,
  SCANCODE_INTERNATIONAL9 = 143,
  SCANCODE_LANG1 = 144, /**< Hangul/English toggle */
  SCANCODE_LANG2 = 145, /**< Hanja conversion */
  SCANCODE_LANG3 = 146, /**< Katakana */
  SCANCODE_LANG4 = 147, /**< Hiragana */
  SCANCODE_LANG5 = 148, /**< Zenkaku/Hankaku */
  SCANCODE_LANG6 = 149, /**< reserved */
  SCANCODE_LANG7 = 150, /**< reserved */
  SCANCODE_LANG8 = 151, /**< reserved */
  SCANCODE_LANG9 = 152, /**< reserved */

  SCANCODE_ALTERASE = 153, /**< Erase-Eaze */
  SCANCODE_SYSREQ = 154,
  SCANCODE_CANCEL = 155,
  SCANCODE_CLEAR = 156,
  SCANCODE_PRIOR = 157,
  SCANCODE_RETURN2 = 158,
  SCANCODE_SEPARATOR = 159,
  SCANCODE_OUT = 160,
  SCANCODE_OPER = 161,
  SCANCODE_CLEARAGAIN = 162,
  SCANCODE_CRSEL = 163,
  SCANCODE_EXSEL = 164,

  SCANCODE_KP_00 = 176,
  SCANCODE_KP_000 = 177,
  SCANCODE_THOUSANDSSEPARATOR = 178,
  SCANCODE_DECIMALSEPARATOR = 179,
  SCANCODE_CURRENCYUNIT = 180,
  SCANCODE_CURRENCYSUBUNIT = 181,
  SCANCODE_KP_LEFTPAREN = 182,
  SCANCODE_KP_RIGHTPAREN = 183,
  SCANCODE_KP_LEFTBRACE = 184,
  SCANCODE_KP_RIGHTBRACE = 185,
  SCANCODE_KP_TAB = 186,
  SCANCODE_KP_BACKSPACE = 187,
  SCANCODE_KP_A = 188,
  SCANCODE_KP_B = 189,
  SCANCODE_KP_C = 190,
  SCANCODE_KP_D = 191,
  SCANCODE_KP_E = 192,
  SCANCODE_KP_F = 193,
  SCANCODE_KP_XOR = 194,
  SCANCODE_KP_POWER = 195,
  SCANCODE_KP_PERCENT = 196,
  SCANCODE_KP_LESS = 197,
  SCANCODE_KP_GREATER = 198,
  SCANCODE_KP_AMPERSAND = 199,
  SCANCODE_KP_DBLAMPERSAND = 200,
  SCANCODE_KP_VERTICALBAR = 201,
  SCANCODE_KP_DBLVERTICALBAR = 202,
  SCANCODE_KP_COLON = 203,
  SCANCODE_KP_HASH = 204,
  SCANCODE_KP_SPACE = 205,
  SCANCODE_KP_AT = 206,
  SCANCODE_KP_EXCLAM = 207,
  SCANCODE_KP_MEMSTORE = 208,
  SCANCODE_KP_MEMRECALL = 209,
  SCANCODE_KP_MEMCLEAR = 210,
  SCANCODE_KP_MEMADD = 211,
  SCANCODE_KP_MEMSUBTRACT = 212,
  SCANCODE_KP_MEMMULTIPLY = 213,
  SCANCODE_KP_MEMDIVIDE = 214,
  SCANCODE_KP_PLUSMINUS = 215,
  SCANCODE_KP_CLEAR = 216,
  SCANCODE_KP_CLEARENTRY = 217,
  SCANCODE_KP_BINARY = 218,
  SCANCODE_KP_OCTAL = 219,
  SCANCODE_KP_DECIMAL = 220,
  SCANCODE_KP_HEXADECIMAL = 221,

  SCANCODE_LCTRL = 224,
  SCANCODE_LSHIFT = 225,
  SCANCODE_LALT = 226, /**< alt, option */
  SCANCODE_LGUI = 227, /**< windows, command (apple), meta */
  SCANCODE_RCTRL = 228,
  SCANCODE_RSHIFT = 229,
  SCANCODE_RALT = 230, /**< alt gr, option */
  SCANCODE_RGUI = 231, /**< windows, command (apple), meta */

  SCANCODE_MODE = 257, /**< I'm not sure if this is really not covered
                            *   by any of the above, but since there's a
                            *   special KMOD_MODE for it I'm adding it here
                            */

  /* @} */ /* Usage page 0x07 */

  /**
   *  \name Usage page 0x0C
   *
   *  These values are mapped from usage page 0x0C (USB consumer page).
   */
  /* @{ */

  SCANCODE_AUDIONEXT = 258,
  SCANCODE_AUDIOPREV = 259,
  SCANCODE_AUDIOSTOP = 260,
  SCANCODE_AUDIOPLAY = 261,
  SCANCODE_AUDIOMUTE = 262,
  SCANCODE_MEDIASELECT = 263,
  SCANCODE_WWW = 264,
  SCANCODE_MAIL = 265,
  SCANCODE_CALCULATOR = 266,
  SCANCODE_COMPUTER = 267,
  SCANCODE_AC_SEARCH = 268,
  SCANCODE_AC_HOME = 269,
  SCANCODE_AC_BACK = 270,
  SCANCODE_AC_FORWARD = 271,
  SCANCODE_AC_STOP = 272,
  SCANCODE_AC_REFRESH = 273,
  SCANCODE_AC_BOOKMARKS = 274,

  /* @} */ /* Usage page 0x0C */

  /**
   *  \name Walther keys
   *
   *  These are values that Christian Walther added (for mac keyboard?).
   */
  /* @{ */

  SCANCODE_BRIGHTNESSDOWN = 275,
  SCANCODE_BRIGHTNESSUP = 276,
  SCANCODE_DISPLAYSWITCH = 277, /**< display mirroring/dual display
                                         switch, video mode switch */
  SCANCODE_KBDILLUMTOGGLE = 278,
  SCANCODE_KBDILLUMDOWN = 279,
  SCANCODE_KBDILLUMUP = 280,
  SCANCODE_EJECT = 281,
  SCANCODE_SLEEP = 282,

  SCANCODE_APP1 = 283,
  SCANCODE_APP2 = 284,

  /* @} */ /* Walther keys */

  /* Add any other keys here. */

  NUM_SCANCODES = 512 /**< not a key, just marks the number of scancodes
                               for array bounds */
};

#define K_SCANCODE_MASK (1 << 30)
#define SCANCODE_TO_KEYCODE(X) (X | K_SCANCODE_MASK)

enum {
  K_UNKNOWN = 0,

  K_RETURN = '\r',
  K_ESCAPE = '\033',
  K_BACKSPACE = '\b',
  K_TAB = '\t',
  K_SPACE = ' ',
  K_EXCLAIM = '!',
  K_QUOTEDBL = '"',
  K_HASH = '#',
  K_PERCENT = '%',
  K_DOLLAR = '$',
  K_AMPERSAND = '&',
  K_QUOTE = '\'',
  K_LEFTPAREN = '(',
  K_RIGHTPAREN = ')',
  K_ASTERISK = '*',
  K_PLUS = '+',
  K_COMMA = ',',
  K_MINUS = '-',
  K_PERIOD = '.',
  K_SLASH = '/',
  K_0 = '0',
  K_1 = '1',
  K_2 = '2',
  K_3 = '3',
  K_4 = '4',
  K_5 = '5',
  K_6 = '6',
  K_7 = '7',
  K_8 = '8',
  K_9 = '9',
  K_COLON = ':',
  K_SEMICOLON = ';',
  K_LESS = '<',
  K_EQUALS = '=',
  K_GREATER = '>',
  K_QUESTION = '?',
  K_AT = '@',
  /*
     Skip uppercase letters
   */
  K_LEFTBRACKET = '[',
  K_BACKSLASH = '\\',
  K_RIGHTBRACKET = ']',
  K_CARET = '^',
  K_UNDERSCORE = '_',
  K_BACKQUOTE = '`',
  K_a = 'a',
  K_b = 'b',
  K_c = 'c',
  K_d = 'd',
  K_e = 'e',
  K_f = 'f',
  K_g = 'g',
  K_h = 'h',
  K_i = 'i',
  K_j = 'j',
  K_k = 'k',
  K_l = 'l',
  K_m = 'm',
  K_n = 'n',
  K_o = 'o',
  K_p = 'p',
  K_q = 'q',
  K_r = 'r',
  K_s = 's',
  K_t = 't',
  K_u = 'u',
  K_v = 'v',
  K_w = 'w',
  K_x = 'x',
  K_y = 'y',
  K_z = 'z',

  K_CAPSLOCK = SCANCODE_TO_KEYCODE(SCANCODE_CAPSLOCK),

  K_F1 = SCANCODE_TO_KEYCODE(SCANCODE_F1),
  K_F2 = SCANCODE_TO_KEYCODE(SCANCODE_F2),
  K_F3 = SCANCODE_TO_KEYCODE(SCANCODE_F3),
  K_F4 = SCANCODE_TO_KEYCODE(SCANCODE_F4),
  K_F5 = SCANCODE_TO_KEYCODE(SCANCODE_F5),
  K_F6 = SCANCODE_TO_KEYCODE(SCANCODE_F6),
  K_F7 = SCANCODE_TO_KEYCODE(SCANCODE_F7),
  K_F8 = SCANCODE_TO_KEYCODE(SCANCODE_F8),
  K_F9 = SCANCODE_TO_KEYCODE(SCANCODE_F9),
  K_F10 = SCANCODE_TO_KEYCODE(SCANCODE_F10),
  K_F11 = SCANCODE_TO_KEYCODE(SCANCODE_F11),
  K_F12 = SCANCODE_TO_KEYCODE(SCANCODE_F12),

  K_PRINTSCREEN = SCANCODE_TO_KEYCODE(SCANCODE_PRINTSCREEN),
  K_SCROLLLOCK = SCANCODE_TO_KEYCODE(SCANCODE_SCROLLLOCK),
  K_PAUSE = SCANCODE_TO_KEYCODE(SCANCODE_PAUSE),
  K_INSERT = SCANCODE_TO_KEYCODE(SCANCODE_INSERT),
  K_HOME = SCANCODE_TO_KEYCODE(SCANCODE_HOME),
  K_PAGEUP = SCANCODE_TO_KEYCODE(SCANCODE_PAGEUP),
  K_DELETE = '\177',
  K_END = SCANCODE_TO_KEYCODE(SCANCODE_END),
  K_PAGEDOWN = SCANCODE_TO_KEYCODE(SCANCODE_PAGEDOWN),
  K_RIGHT = SCANCODE_TO_KEYCODE(SCANCODE_RIGHT),
  K_LEFT = SCANCODE_TO_KEYCODE(SCANCODE_LEFT),
  K_DOWN = SCANCODE_TO_KEYCODE(SCANCODE_DOWN),
  K_UP = SCANCODE_TO_KEYCODE(SCANCODE_UP),

  K_NUMLOCKCLEAR = SCANCODE_TO_KEYCODE(SCANCODE_NUMLOCKCLEAR),
  K_KP_DIVIDE = SCANCODE_TO_KEYCODE(SCANCODE_KP_DIVIDE),
  K_KP_MULTIPLY = SCANCODE_TO_KEYCODE(SCANCODE_KP_MULTIPLY),
  K_KP_MINUS = SCANCODE_TO_KEYCODE(SCANCODE_KP_MINUS),
  K_KP_PLUS = SCANCODE_TO_KEYCODE(SCANCODE_KP_PLUS),
  K_KP_ENTER = SCANCODE_TO_KEYCODE(SCANCODE_KP_ENTER),
  K_KP_1 = SCANCODE_TO_KEYCODE(SCANCODE_KP_1),
  K_KP_2 = SCANCODE_TO_KEYCODE(SCANCODE_KP_2),
  K_KP_3 = SCANCODE_TO_KEYCODE(SCANCODE_KP_3),
  K_KP_4 = SCANCODE_TO_KEYCODE(SCANCODE_KP_4),
  K_KP_5 = SCANCODE_TO_KEYCODE(SCANCODE_KP_5),
  K_KP_6 = SCANCODE_TO_KEYCODE(SCANCODE_KP_6),
  K_KP_7 = SCANCODE_TO_KEYCODE(SCANCODE_KP_7),
  K_KP_8 = SCANCODE_TO_KEYCODE(SCANCODE_KP_8),
  K_KP_9 = SCANCODE_TO_KEYCODE(SCANCODE_KP_9),
  K_KP_0 = SCANCODE_TO_KEYCODE(SCANCODE_KP_0),
  K_KP_PERIOD = SCANCODE_TO_KEYCODE(SCANCODE_KP_PERIOD),

  K_APPLICATION = SCANCODE_TO_KEYCODE(SCANCODE_APPLICATION),
  K_POWER = SCANCODE_TO_KEYCODE(SCANCODE_POWER),
  K_KP_EQUALS = SCANCODE_TO_KEYCODE(SCANCODE_KP_EQUALS),
  K_F13 = SCANCODE_TO_KEYCODE(SCANCODE_F13),
  K_F14 = SCANCODE_TO_KEYCODE(SCANCODE_F14),
  K_F15 = SCANCODE_TO_KEYCODE(SCANCODE_F15),
  K_F16 = SCANCODE_TO_KEYCODE(SCANCODE_F16),
  K_F17 = SCANCODE_TO_KEYCODE(SCANCODE_F17),
  K_F18 = SCANCODE_TO_KEYCODE(SCANCODE_F18),
  K_F19 = SCANCODE_TO_KEYCODE(SCANCODE_F19),
  K_F20 = SCANCODE_TO_KEYCODE(SCANCODE_F20),
  K_F21 = SCANCODE_TO_KEYCODE(SCANCODE_F21),
  K_F22 = SCANCODE_TO_KEYCODE(SCANCODE_F22),
  K_F23 = SCANCODE_TO_KEYCODE(SCANCODE_F23),
  K_F24 = SCANCODE_TO_KEYCODE(SCANCODE_F24),
  K_EXECUTE = SCANCODE_TO_KEYCODE(SCANCODE_EXECUTE),
  K_HELP = SCANCODE_TO_KEYCODE(SCANCODE_HELP),
  K_MENU = SCANCODE_TO_KEYCODE(SCANCODE_MENU),
  K_SELECT = SCANCODE_TO_KEYCODE(SCANCODE_SELECT),
  K_STOP = SCANCODE_TO_KEYCODE(SCANCODE_STOP),
  K_AGAIN = SCANCODE_TO_KEYCODE(SCANCODE_AGAIN),
  K_UNDO = SCANCODE_TO_KEYCODE(SCANCODE_UNDO),
  K_CUT = SCANCODE_TO_KEYCODE(SCANCODE_CUT),
  K_COPY = SCANCODE_TO_KEYCODE(SCANCODE_COPY),
  K_PASTE = SCANCODE_TO_KEYCODE(SCANCODE_PASTE),
  K_FIND = SCANCODE_TO_KEYCODE(SCANCODE_FIND),
  K_MUTE = SCANCODE_TO_KEYCODE(SCANCODE_MUTE),
  K_VOLUMEUP = SCANCODE_TO_KEYCODE(SCANCODE_VOLUMEUP),
  K_VOLUMEDOWN = SCANCODE_TO_KEYCODE(SCANCODE_VOLUMEDOWN),
  K_KP_COMMA = SCANCODE_TO_KEYCODE(SCANCODE_KP_COMMA),
  K_KP_EQUALSAS400 = SCANCODE_TO_KEYCODE(SCANCODE_KP_EQUALSAS400),

  K_ALTERASE = SCANCODE_TO_KEYCODE(SCANCODE_ALTERASE),
  K_SYSREQ = SCANCODE_TO_KEYCODE(SCANCODE_SYSREQ),
  K_CANCEL = SCANCODE_TO_KEYCODE(SCANCODE_CANCEL),
  K_CLEAR = SCANCODE_TO_KEYCODE(SCANCODE_CLEAR),
  K_PRIOR = SCANCODE_TO_KEYCODE(SCANCODE_PRIOR),
  K_RETURN2 = SCANCODE_TO_KEYCODE(SCANCODE_RETURN2),
  K_SEPARATOR = SCANCODE_TO_KEYCODE(SCANCODE_SEPARATOR),
  K_OUT = SCANCODE_TO_KEYCODE(SCANCODE_OUT),
  K_OPER = SCANCODE_TO_KEYCODE(SCANCODE_OPER),
  K_CLEARAGAIN = SCANCODE_TO_KEYCODE(SCANCODE_CLEARAGAIN),
  K_CRSEL = SCANCODE_TO_KEYCODE(SCANCODE_CRSEL),
  K_EXSEL = SCANCODE_TO_KEYCODE(SCANCODE_EXSEL),

  K_KP_00 = SCANCODE_TO_KEYCODE(SCANCODE_KP_00),
  K_KP_000 = SCANCODE_TO_KEYCODE(SCANCODE_KP_000),
  K_THOUSANDSSEPARATOR = SCANCODE_TO_KEYCODE(SCANCODE_THOUSANDSSEPARATOR),
  K_DECIMALSEPARATOR = SCANCODE_TO_KEYCODE(SCANCODE_DECIMALSEPARATOR),
  K_CURRENCYUNIT = SCANCODE_TO_KEYCODE(SCANCODE_CURRENCYUNIT),
  K_CURRENCYSUBUNIT = SCANCODE_TO_KEYCODE(SCANCODE_CURRENCYSUBUNIT),
  K_KP_LEFTPAREN = SCANCODE_TO_KEYCODE(SCANCODE_KP_LEFTPAREN),
  K_KP_RIGHTPAREN = SCANCODE_TO_KEYCODE(SCANCODE_KP_RIGHTPAREN),
  K_KP_LEFTBRACE = SCANCODE_TO_KEYCODE(SCANCODE_KP_LEFTBRACE),
  K_KP_RIGHTBRACE = SCANCODE_TO_KEYCODE(SCANCODE_KP_RIGHTBRACE),
  K_KP_TAB = SCANCODE_TO_KEYCODE(SCANCODE_KP_TAB),
  K_KP_BACKSPACE = SCANCODE_TO_KEYCODE(SCANCODE_KP_BACKSPACE),
  K_KP_A = SCANCODE_TO_KEYCODE(SCANCODE_KP_A),
  K_KP_B = SCANCODE_TO_KEYCODE(SCANCODE_KP_B),
  K_KP_C = SCANCODE_TO_KEYCODE(SCANCODE_KP_C),
  K_KP_D = SCANCODE_TO_KEYCODE(SCANCODE_KP_D),
  K_KP_E = SCANCODE_TO_KEYCODE(SCANCODE_KP_E),
  K_KP_F = SCANCODE_TO_KEYCODE(SCANCODE_KP_F),
  K_KP_XOR = SCANCODE_TO_KEYCODE(SCANCODE_KP_XOR),
  K_KP_POWER = SCANCODE_TO_KEYCODE(SCANCODE_KP_POWER),
  K_KP_PERCENT = SCANCODE_TO_KEYCODE(SCANCODE_KP_PERCENT),
  K_KP_LESS = SCANCODE_TO_KEYCODE(SCANCODE_KP_LESS),
  K_KP_GREATER = SCANCODE_TO_KEYCODE(SCANCODE_KP_GREATER),
  K_KP_AMPERSAND = SCANCODE_TO_KEYCODE(SCANCODE_KP_AMPERSAND),
  K_KP_DBLAMPERSAND = SCANCODE_TO_KEYCODE(SCANCODE_KP_DBLAMPERSAND),
  K_KP_VERTICALBAR = SCANCODE_TO_KEYCODE(SCANCODE_KP_VERTICALBAR),
  K_KP_DBLVERTICALBAR = SCANCODE_TO_KEYCODE(SCANCODE_KP_DBLVERTICALBAR),
  K_KP_COLON = SCANCODE_TO_KEYCODE(SCANCODE_KP_COLON),
  K_KP_HASH = SCANCODE_TO_KEYCODE(SCANCODE_KP_HASH),
  K_KP_SPACE = SCANCODE_TO_KEYCODE(SCANCODE_KP_SPACE),
  K_KP_AT = SCANCODE_TO_KEYCODE(SCANCODE_KP_AT),
  K_KP_EXCLAM = SCANCODE_TO_KEYCODE(SCANCODE_KP_EXCLAM),
  K_KP_MEMSTORE = SCANCODE_TO_KEYCODE(SCANCODE_KP_MEMSTORE),
  K_KP_MEMRECALL = SCANCODE_TO_KEYCODE(SCANCODE_KP_MEMRECALL),
  K_KP_MEMCLEAR = SCANCODE_TO_KEYCODE(SCANCODE_KP_MEMCLEAR),
  K_KP_MEMADD = SCANCODE_TO_KEYCODE(SCANCODE_KP_MEMADD),
  K_KP_MEMSUBTRACT = SCANCODE_TO_KEYCODE(SCANCODE_KP_MEMSUBTRACT),
  K_KP_MEMMULTIPLY = SCANCODE_TO_KEYCODE(SCANCODE_KP_MEMMULTIPLY),
  K_KP_MEMDIVIDE = SCANCODE_TO_KEYCODE(SCANCODE_KP_MEMDIVIDE),
  K_KP_PLUSMINUS = SCANCODE_TO_KEYCODE(SCANCODE_KP_PLUSMINUS),
  K_KP_CLEAR = SCANCODE_TO_KEYCODE(SCANCODE_KP_CLEAR),
  K_KP_CLEARENTRY = SCANCODE_TO_KEYCODE(SCANCODE_KP_CLEARENTRY),
  K_KP_BINARY = SCANCODE_TO_KEYCODE(SCANCODE_KP_BINARY),
  K_KP_OCTAL = SCANCODE_TO_KEYCODE(SCANCODE_KP_OCTAL),
  K_KP_DECIMAL = SCANCODE_TO_KEYCODE(SCANCODE_KP_DECIMAL),
  K_KP_HEXADECIMAL = SCANCODE_TO_KEYCODE(SCANCODE_KP_HEXADECIMAL),

  K_LCTRL = SCANCODE_TO_KEYCODE(SCANCODE_LCTRL),
  K_LSHIFT = SCANCODE_TO_KEYCODE(SCANCODE_LSHIFT),
  K_LALT = SCANCODE_TO_KEYCODE(SCANCODE_LALT),
  K_LGUI = SCANCODE_TO_KEYCODE(SCANCODE_LGUI),
  K_RCTRL = SCANCODE_TO_KEYCODE(SCANCODE_RCTRL),
  K_RSHIFT = SCANCODE_TO_KEYCODE(SCANCODE_RSHIFT),
  K_RALT = SCANCODE_TO_KEYCODE(SCANCODE_RALT),
  K_RGUI = SCANCODE_TO_KEYCODE(SCANCODE_RGUI),

  K_MODE = SCANCODE_TO_KEYCODE(SCANCODE_MODE),

  K_AUDIONEXT = SCANCODE_TO_KEYCODE(SCANCODE_AUDIONEXT),
  K_AUDIOPREV = SCANCODE_TO_KEYCODE(SCANCODE_AUDIOPREV),
  K_AUDIOSTOP = SCANCODE_TO_KEYCODE(SCANCODE_AUDIOSTOP),
  K_AUDIOPLAY = SCANCODE_TO_KEYCODE(SCANCODE_AUDIOPLAY),
  K_AUDIOMUTE = SCANCODE_TO_KEYCODE(SCANCODE_AUDIOMUTE),
  K_MEDIASELECT = SCANCODE_TO_KEYCODE(SCANCODE_MEDIASELECT),
  K_WWW = SCANCODE_TO_KEYCODE(SCANCODE_WWW),
  K_MAIL = SCANCODE_TO_KEYCODE(SCANCODE_MAIL),
  K_CALCULATOR = SCANCODE_TO_KEYCODE(SCANCODE_CALCULATOR),
  K_COMPUTER = SCANCODE_TO_KEYCODE(SCANCODE_COMPUTER),
  K_AC_SEARCH = SCANCODE_TO_KEYCODE(SCANCODE_AC_SEARCH),
  K_AC_HOME = SCANCODE_TO_KEYCODE(SCANCODE_AC_HOME),
  K_AC_BACK = SCANCODE_TO_KEYCODE(SCANCODE_AC_BACK),
  K_AC_FORWARD = SCANCODE_TO_KEYCODE(SCANCODE_AC_FORWARD),
  K_AC_STOP = SCANCODE_TO_KEYCODE(SCANCODE_AC_STOP),
  K_AC_REFRESH = SCANCODE_TO_KEYCODE(SCANCODE_AC_REFRESH),
  K_AC_BOOKMARKS = SCANCODE_TO_KEYCODE(SCANCODE_AC_BOOKMARKS),

  K_BRIGHTNESSDOWN = SCANCODE_TO_KEYCODE(SCANCODE_BRIGHTNESSDOWN),
  K_BRIGHTNESSUP = SCANCODE_TO_KEYCODE(SCANCODE_BRIGHTNESSUP),
  K_DISPLAYSWITCH = SCANCODE_TO_KEYCODE(SCANCODE_DISPLAYSWITCH),
  K_KBDILLUMTOGGLE = SCANCODE_TO_KEYCODE(SCANCODE_KBDILLUMTOGGLE),
  K_KBDILLUMDOWN = SCANCODE_TO_KEYCODE(SCANCODE_KBDILLUMDOWN),
  K_KBDILLUMUP = SCANCODE_TO_KEYCODE(SCANCODE_KBDILLUMUP),
  K_EJECT = SCANCODE_TO_KEYCODE(SCANCODE_EJECT),
  K_SLEEP = SCANCODE_TO_KEYCODE(SCANCODE_SLEEP)
};

typedef int32_t Keycode;

/**
 * \brief Enumeration of valid key mods (possibly OR'd together).
 */
enum Keymod {
  K_MOD_NONE = 0x0000,
  K_MOD_LSHIFT = 0x0001,
  K_MOD_RSHIFT = 0x0002,
  K_MOD_LCTRL = 0x0040,
  K_MOD_RCTRL = 0x0080,
  K_MOD_LALT = 0x0100,
  K_MOD_RALT = 0x0200,
  K_MOD_LGUI = 0x0400,
  K_MOD_RGUI = 0x0800,
  K_MOD_NUM = 0x1000,
  K_MOD_CAPS = 0x2000,
  K_MOD_MODE = 0x4000,
  K_MOD_RESERVED = 0x8000
};

#define K_MOD_CTRL (KMOD_LCTRL | KMOD_RCTRL)
#define K_MOD_SHIFT (KMOD_LSHIFT | KMOD_RSHIFT)
#define K_MOD_ALT (KMOD_LALT | KMOD_RALT)
#define K_MOD_GUI (KMOD_LGUI | KMOD_RGUI)

struct Keysym {
  Scancode scancode;
  Keycode sym;
  uint16_t mod;
};

namespace core {
namespace events {
// infos
struct StreamInfo {
  explicit StreamInfo(VideoState* stream);

  VideoState* stream_;
};

struct FrameInfo : public StreamInfo {
  FrameInfo(VideoState* stream, core::VideoFrame* frame);
  core::VideoFrame* frame;
};

struct QuitStreamInfo : public StreamInfo {
  QuitStreamInfo(VideoState* stream, int code);
  int code;
};

struct ChangeStreamInfo : public StreamInfo {
  enum ChangeDirection { NEXT_STREAM, PREV_STREAM, SEEK_STREAM };
  ChangeStreamInfo(VideoState* stream, ChangeDirection direction, size_t pos);

  ChangeDirection direction;
  size_t pos;
};

struct TimeInfo {
  TimeInfo();

  common::time64_t time_millisecond;
};

struct KeyPressInfo {
  KeyPressInfo(bool pressed, Keysym ks);

  bool is_pressed;
  Keysym ks;
};
struct KeyReleaseInfo {
  KeyReleaseInfo(bool pressed, Keysym ks);

  bool is_pressed;
  Keysym ks;
};

struct WindowResizeInfo {
  WindowResizeInfo(int width, int height);

  int width;
  int height;
};

struct WindowExposeInfo {};
struct WindowCloseInfo {};

struct MouseMoveInfo {};

/**
 *  Used as a mask when testing buttons in buttonstate.
 *   - Button 1:  Left mouse button
 *   - Button 2:  Middle mouse button
 *   - Button 3:  Right mouse button
 */
#define FASTO_BUTTON(X) (1 << ((X)-1))
#define FASTO_BUTTON_LEFT 1
#define FASTO_BUTTON_MIDDLE 2
#define FASTO_BUTTON_RIGHT 3
#define FASTO_BUTTON_X1 4
#define FASTO_BUTTON_X2 5
#define FASTO_BUTTON_LMASK FASTO_BUTTON(FASTO_BUTTON_LEFT)
#define FASTO_BUTTON_MMASK FASTO_BUTTON(FASTO_BUTTON_MIDDLE)
#define FASTO_BUTTON_RMASK FASTO_BUTTON(FASTO_BUTTON_RIGHT)
#define FASTO_BUTTON_X1MASK FASTO_BUTTON(FASTO_BUTTON_X1)
#define FASTO_BUTTON_X2MASK FASTO_BUTTON(FASTO_BUTTON_X2)

struct MousePressInfo {
  MousePressInfo(uint8_t button, uint8_t state);

  uint8_t button; /**< The mouse button index */
  uint8_t state;
};
struct MouseReleaseInfo {
  MouseReleaseInfo(uint8_t button, uint8_t state);

  uint8_t button; /**< The mouse button index */
  uint8_t state;
};

struct QuitInfo {};

typedef EventBase<TIMER_EVENT, TimeInfo> TimerEvent;

typedef EventBase<ALLOC_FRAME_EVENT, FrameInfo> AllocFrameEvent;
typedef EventBase<QUIT_STREAM_EVENT, QuitStreamInfo> QuitStreamEvent;
typedef EventBase<CHANGE_STREAM_EVENT, ChangeStreamInfo> ChangeStreamEvent;

typedef EventBase<KEY_PRESS_EVENT, KeyPressInfo> KeyPressEvent;
typedef EventBase<KEY_RELEASE_EVENT, KeyReleaseInfo> KeyReleaseEvent;

typedef EventBase<WINDOW_RESIZE_EVENT, WindowResizeInfo> WindowResizeEvent;
typedef EventBase<WINDOW_EXPOSE_EVENT, WindowExposeInfo> WindowExposeEvent;
typedef EventBase<WINDOW_CLOSE_EVENT, WindowCloseInfo> WindowCloseEvent;

typedef EventBase<MOUSE_MOVE_EVENT, MouseMoveInfo> MouseMoveEvent;
typedef EventBase<MOUSE_PRESS_EVENT, MousePressInfo> MousePressEvent;
typedef EventBase<MOUSE_RELEASE_EVENT, MouseReleaseInfo> MouseReleaseEvent;

typedef EventBase<QUIT_EVENT, QuitInfo> QuitEvent;

}  // namespace events {
}
