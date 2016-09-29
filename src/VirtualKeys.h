#pragma once

#ifndef _VIRTUALKEYS_H_INCLUDED_
#define _VIRTUALKEYS_H_INCLUDED_

enum VirtualKeyEnumValues : intmax {
	KC_LButton  = 0x01,  // Left mouse button
	KC_RButton  = 0x02,  // Right mouse button
	KC_Cancel   = 0x03,  // Control-break processing
	KC_MButton  = 0x04,  // Middle mouse button (three-button mouse)
	KC_XButton1 = 0x05,  // X1 mouse button
	KC_XButton2 = 0x06,  // X2 mouse button
	// - = 0x07, // Undefined
	KC_Back = 0x08,  // BACKSPACE key
	KC_Tab  = 0x09,  // TAB key
	// - = 0x0A-0B, // Reserved
	KC_Clear  = 0x0C,  // CLEAR key
	KC_Return = 0x0D,  // ENTER key
	// - = 0x0E-0F, // Undefined
	KC_Shift   = 0x10,  // SHIFT key
	KC_Control = 0x11,  // CTRL key
	KC_Menu    = 0x12,  // ALT key
	KC_Alt     = 0x12,  // ALT key
	KC_Pause   = 0x13,  // PAUSE key
	KC_Capital = 0x14,  // CAPS LOCK key
	KC_Kana    = 0x15,  // IME Kana mode
	KC_Hanguel = 0x15,  // IME Hanguel mode (maintained for compatibility; use KC_Hangul)
	KC_Hangul  = 0x15,  // IME Hangul mode
	// - = 0x16, // Undefined
	KC_Junja = 0x17,  // IME Junja mode
	KC_Final = 0x18,  // IME final mode
	KC_Hanja = 0x19,  // IME Hanja mode
	KC_Kanji = 0x19,  // IME Kanji mode
	// - = 0x1A, // Undefined
	KC_Escape     = 0x1B,  // ESC key
	KC_Convert    = 0x1C,  // IME convert
	KC_NonConvert = 0x1D,  // IME nonconvert
	KC_Accept     = 0x1E,  // IME accept
	KC_ModeChange = 0x1F,  // IME mode change request
	KC_Space      = 0x20,  // SPACEBAR
	KC_Prior      = 0x21,  // PAGE UP key
	KC_Next       = 0x22,  // PAGE DOWN key
	KC_End        = 0x23,  // END key
	KC_Home       = 0x24,  // HOME key
	KC_Left       = 0x25,  // LEFT ARROW key
	KC_Up         = 0x26,  // UP ARROW key
	KC_Right      = 0x27,  // RIGHT ARROW key
	KC_Down       = 0x28,  // DOWN ARROW key
	KC_Select     = 0x29,  // SELECT key
	KC_Print      = 0x2A,  // PRINT key
	KC_Execute    = 0x2B,  // EXECUTE key
	KC_Snapshot   = 0x2C,  // PRINT SCREEN key
	KC_Insert     = 0x2D,  // INS key
	KC_Delete     = 0x2E,  // DEL key
	KC_Help       = 0x2F,  // HELP key
	KC_Key_0      = 0x30,  // 0 key
	KC_Key_1      = 0x31,  // 1 key
	KC_Key_2      = 0x32,  // 2 key
	KC_Key_3      = 0x33,  // 3 key
	KC_Key_4      = 0x34,  // 4 key
	KC_Key_5      = 0x35,  // 5 key
	KC_Key_6      = 0x36,  // 6 key
	KC_Key_7      = 0x37,  // 7 key
	KC_Key_8      = 0x38,  // 8 key
	KC_Key_9      = 0x39,  // 9 key
	// - = 0x3A-40, // Undefined
	KC_Key_A = 0x41,  // A key
	KC_Key_B = 0x42,  // B key
	KC_Key_C = 0x43,  // C key
	KC_Key_D = 0x44,  // D key
	KC_Key_E = 0x45,  // E key
	KC_Key_F = 0x46,  // F key
	KC_Key_G = 0x47,  // G key
	KC_Key_H = 0x48,  // H key
	KC_Key_I = 0x49,  // I key
	KC_Key_J = 0x4A,  // J key
	KC_Key_K = 0x4B,  // K key
	KC_Key_L = 0x4C,  // L key
	KC_Key_M = 0x4D,  // M key
	KC_Key_N = 0x4E,  // N key
	KC_Key_O = 0x4F,  // O key
	KC_Key_P = 0x50,  // P key
	KC_Key_Q = 0x51,  // Q key
	KC_Key_R = 0x52,  // R key
	KC_Key_S = 0x53,  // S key
	KC_Key_T = 0x54,  // T key
	KC_Key_U = 0x55,  // U key
	KC_Key_V = 0x56,  // V key
	KC_Key_W = 0x57,  // W key
	KC_Key_X = 0x58,  // X key
	KC_Key_Y = 0x59,  // Y key
	KC_Key_Z = 0x5A,  // Z key
	KC_LWin  = 0x5B,  // Left Windows key (Natural keyboard)
	KC_RWin  = 0x5C,  // Right Windows key (Natural keyboard)
	KC_Apps  = 0x5D,  // Applications key (Natural keyboard)
	// - = 0x5E, // Reserved
	KC_Sleep     = 0x5F,  // Computer Sleep key
	KC_Numpad0   = 0x60,  // Numeric keypad 0 key
	KC_Numpad1   = 0x61,  // Numeric keypad 1 key
	KC_Numpad2   = 0x62,  // Numeric keypad 2 key
	KC_Numpad3   = 0x63,  // Numeric keypad 3 key
	KC_Numpad4   = 0x64,  // Numeric keypad 4 key
	KC_Numpad5   = 0x65,  // Numeric keypad 5 key
	KC_Numpad6   = 0x66,  // Numeric keypad 6 key
	KC_Numpad7   = 0x67,  // Numeric keypad 7 key
	KC_Numpad8   = 0x68,  // Numeric keypad 8 key
	KC_Numpad9   = 0x69,  // Numeric keypad 9 key
	KC_Multiply  = 0x6A,  // Multiply key
	KC_Add       = 0x6B,  // Add key
	KC_Separator = 0x6C,  // Separator key
	KC_Subtract  = 0x6D,  // Subtract key
	KC_Decimal   = 0x6E,  // Decimal key
	KC_Divide    = 0x6F,  // Divide key
	KC_F1        = 0x70,  // F1 key
	KC_F2        = 0x71,  // F2 key
	KC_F3        = 0x72,  // F3 key
	KC_F4        = 0x73,  // F4 key
	KC_F5        = 0x74,  // F5 key
	KC_F6        = 0x75,  // F6 key
	KC_F7        = 0x76,  // F7 key
	KC_F8        = 0x77,  // F8 key
	KC_F9        = 0x78,  // F9 key
	KC_F10       = 0x79,  // F10 key
	KC_F11       = 0x7A,  // F11 key
	KC_F12       = 0x7B,  // F12 key
	KC_F13       = 0x7C,  // F13 key
	KC_F14       = 0x7D,  // F14 key
	KC_F15       = 0x7E,  // F15 key
	KC_F16       = 0x7F,  // F16 key
	KC_F17       = 0x80,  // F17 key
	KC_F18       = 0x81,  // F18 key
	KC_F19       = 0x82,  // F19 key
	KC_F20       = 0x83,  // F20 key
	KC_F21       = 0x84,  // F21 key
	KC_F22       = 0x85,  // F22 key
	KC_F23       = 0x86,  // F23 key
	KC_F24       = 0x87,  // F24 key
	// - = 0x88-8F, // Unassigned
	KC_NumLock = 0x90,  // NUM LOCK key
	KC_Scroll  = 0x91,  // SCROLL LOCK key
	// 0x92-96 OEM specific
	// - 0x97-9F // Unassigned
	KC_LShift              = 0xA0,  // Left SHIFT key
	KC_RShift              = 0xA1,  // Right SHIFT key
	KC_LControl            = 0xA2,  // Left CONTROL key
	KC_RControl            = 0xA3,  // Right CONTROL key
	KC_LMenu               = 0xA4,  // Left MENU key
	KC_RMenu               = 0xA5,  // Right MENU key
	KC_Browser_Back        = 0xA6,  // Browser Back key
	KC_Browser_Forward     = 0xA7,  // Browser Forward key
	KC_Browser_Refresh     = 0xA8,  // Browser Refresh key
	KC_Browser_Stop        = 0xA9,  // Browser Stop key
	KC_Browser_Search      = 0xAA,  // Browser Search key
	KC_Browser_Favorites   = 0xAB,  // Browser Favorites key
	KC_Browser_Home        = 0xAC,  // Browser Start and Home key
	KC_Volume_Mute         = 0xAD,  // Volume Mute key
	KC_Volume_Down         = 0xAE,  // Volume Down key
	KC_Volume_Up           = 0xAF,  // Volume Up key
	KC_Media_Next_Track    = 0xB0,  // Next Track key
	KC_Media_Prev_Track    = 0xB1,  // Previous Track key
	KC_Media_Stop          = 0xB2,  // Stop Media key
	KC_Media_Play_Pause    = 0xB3,  // Play/Pause Media key
	KC_Launch_Mail         = 0xB4,  // Start Mail key
	KC_Launch_Media_Select = 0xB5,  // Select Media key
	KC_Launch_App1         = 0xB6,  // Start Application 1 key
	KC_Launch_App2         = 0xB7,  // Start Application 2 key
	// - = 0xB8-B9, // Reserved
	KC_Oem_1 = 0xBA,       // Used for miscellaneous characters; it can vary by keyboard. For the US
	                       // standard keyboard, the ';:' key
	KC_Oem_plus   = 0xBB,  // For any country/region, the '+' key
	KC_Oem_comma  = 0xBC,  // For any country/region, the ',' key
	KC_Oem_minus  = 0xBD,  // For any country/region, the '-' key
	KC_Oem_period = 0xBE,  // For any country/region, the '.' key
	KC_Oem_2      = 0xBF,  // Used for miscellaneous characters; it can vary by keyboard. For the US
	                       // standard keyboard, the '/?' key
	KC_Oem_3 = 0xC0,       // Used for miscellaneous characters; it can vary by keyboard. For the US
	                       // standard keyboard, the '`~' key
	// - = 0xC1-D7, // Reserved
	// - = 0xD8-DA, // Unassigned
	KC_Oem_4 = 0xDB,  // Used for miscellaneous characters; it can vary by keyboard. For the US
	                  // standard keyboard, the '[{' key
	KC_Oem_5 = 0xDC,  // Used for miscellaneous characters; it can vary by keyboard. For the US
	                  // standard keyboard, the '\|' key
	KC_Oem_6 = 0xDD,  // Used for miscellaneous characters; it can vary by keyboard. For the US
	                  // standard keyboard, the ']}' key
	KC_Oem_7 = 0xDE,  // Used for miscellaneous characters; it can vary by keyboard. For the US
	                  // standard keyboard, the 'single-quote/double-quote' key
	KC_Oem_8 = 0xDF,  // Used for miscellaneous characters; it can vary by keyboard.
	// - = 0xE0, // Reserved
	// 0xE1 OEM specific
	KC_Oem_102 =
	    0xE2,  // Either the angle bracket key or the backslash key on the RT 102-key keyboard
	// 0xE3-E4 OEM specific
	KC_Processkey = 0xE5,  // IME PROCESS key
	// 0xE6 OEM specific
	KC_Packet = 0xE7,  // Used to pass Unicode characters as if they were keystrokes. The KC_Packet
	                   // key is the low word of a 32-bit Virtual Key value used for non-keyboard
	                   // input methods. For more information, see Remark in KEYBDINPUT, SendInput,
	                   // WM_KEYDOWN, and WM_KEYUP
	// - = 0xE8, // Unassigned
	// 0xE9-F5 OEM specific
	KC_Attn      = 0xF6,  // Attn key
	KC_CrSel     = 0xF7,  // CrSel key
	KC_ExSel     = 0xF8,  // ExSel key
	KC_ErEof     = 0xF9,  // Erase EOF key
	KC_Play      = 0xFA,  // Play key
	KC_Zoom      = 0xFB,  // Zoom key
	KC_Noname    = 0xFC,  // Reserved
	KC_Pa1       = 0xFD,  // PA1 key
	KC_Oem_Clear = 0xFE,  // Clear key

	KC_Count = 0xFF,
};

#endif  // _VIRTUALKEYS_H_INCLUDED_
