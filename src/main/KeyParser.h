#pragma once
#include <Windows.h>
#include "operate/Key.h"
#include "operate/Mouse.h"
#include "event/KeyEvent.h"

bool onRecord(int isKey, const Mouse & mouse, const Key & key, KeyEventState state);
bool onPlay(int isKey, const Mouse & mouse, const Key & key, KeyEventState state);
bool onQuickRecord(int isKey, const Mouse & mouse, const Key & key, KeyEventState state);
bool onQucikPlay(int isKey, const Mouse & mouse, const Key & key, KeyEventState state);
bool onRequestRegister(const Key & key, KeyEventState state);
bool onRecordSetting(const Key & key, KeyEventState state);
bool onPlaySetting(const Key & key, KeyEventState state);
LRESULT CALLBACK parserKey(HWND hwnd, HHOOK keyboardHook, LPARAM lParam, int nCode, WPARAM wParam);
LRESULT CALLBACK parserMouse(HWND hWnd, HHOOK mouseHook, LPARAM lParam, int nCode, WPARAM wParam);
