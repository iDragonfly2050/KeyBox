#include "stdafx.h"
#include "KeyManager.h"
#include "resource.h"
#include <Windows.h>
#include <Shellapi.h>
#include <algorithm>
#include <mutex>

extern std::mutex mutex;
extern HINSTANCE hInst;
extern HMENU hMenu;
extern NOTIFYICONDATA stateIcon;

/////////////////////////////////////////////////////////////////////////
// public:
/////////////////////////////////////////////////////////////////////////
const Key & KeyManager::getKeyByName(const char * name) const {
	auto result = std::find_if(_keys.cbegin(), _keys.cend(), [=](const Key & k) {
		if (strcmp(k.getName(), name) == 0)
			return true;
		return false;
	});

	if (result != _keys.cend())
		return *result;
	else
		return getKeyByName("Null");
}

const char * KeyManager::getNameByKey(const Key & key) const {
	auto result = std::find_if(_keys.cbegin(), _keys.cend(), [=](const Key & k) {
		if (k.getVkCode() == key.getVkCode() && k.getScanCode() == key.getScanCode())
			return true;
		return false;
	});

	if (result != _keys.cend())
		return result->getName();
	else
		return "";
}

EventQueue & KeyManager::getRegister(const char * name) {
	if (_registers.count(name))
		return _registers[name];
	return _registers["0"];
}

bool KeyManager::getCombinedKeyState(const std::string & key) const {
	if (_combinedKeyStates.count(key))
		return _combinedKeyStates.at(key);
	return false;
}

void KeyManager::processCombinedKey(const Key & key, KeyEventState state) {
	if (!key.isCombinedKey()) return;
	_combinedKeyStates[key.getName()] = state == DOWN ? true : false;
}

void KeyManager::setOnRecord(bool val) {
	_onRecord = val;
	stateIcon.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(val ? IDI_RECORD : IDI_APP));
	Shell_NotifyIcon(NIM_MODIFY, &stateIcon);
}

void KeyManager::setOnPlay(bool val) {
	_onPlay = val;
	stateIcon.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(val ? IDI_PLAY : IDI_APP));
	Shell_NotifyIcon(NIM_MODIFY, &stateIcon);
	if (!_onPlay) {
		if (isOnRecord())
			setOnRecord(isOnRecord());
		if (isOnQuickRecord())
			setOnQuickRecord(isOnQuickRecord());
	}
}

void KeyManager::setOnQuickRecord(bool val) {
	_onQuickRecord = val;
	stateIcon.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(val ? IDI_RECORD : IDI_APP));
	Shell_NotifyIcon(NIM_MODIFY, &stateIcon);
}

void KeyManager::setOnQucikPlay(bool val) {
	_onQucikPlay = val;
	stateIcon.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(val ? IDI_PLAY : IDI_APP));
	Shell_NotifyIcon(NIM_MODIFY, &stateIcon);
	if (!_onQucikPlay) {
		if (isOnRecord())
			setOnRecord(isOnRecord());
		if (isOnQuickRecord())
			setOnQuickRecord(isOnQuickRecord());
	}
}

bool KeyManager::isUserPressStopPlay() {
	bool userPressStopPlay;
	mutex.lock();
	userPressStopPlay = _userPressStopPlay;
	mutex.unlock();
	return userPressStopPlay;
}

void KeyManager::setUserPressStopPlay(bool val) {
	mutex.lock();
	_userPressStopPlay = val;
	mutex.unlock();
}

void KeyManager::setOnRequestRegister(bool val) {
	_onRequestRegister = val;
	stateIcon.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(val ? IDI_REQUEST_REGISTER : IDI_APP));
	Shell_NotifyIcon(NIM_MODIFY, &stateIcon);
	if (!_onRequestRegister) {
		if (isOnRecord())
			setOnRecord(isOnRecord());
		if (isOnQuickRecord())
			setOnQuickRecord(isOnQuickRecord());
	}
}

void KeyManager::setCurrentQuickRegister(const std::string & val) {
	_currentQuickRegister = val;
	saveSettings();
}

void KeyManager::setOnRecordSetting(bool val) {
	_onRecordSetting = val;
	stateIcon.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(val ? IDI_RECORDSETTING : IDI_APP));
	Shell_NotifyIcon(NIM_MODIFY, &stateIcon);
}

void KeyManager::setOnPlaySetting(bool val) {
	_onPlaySetting = val;
	stateIcon.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(val ? IDI_PLAYSETTING : IDI_APP));
	Shell_NotifyIcon(NIM_MODIFY, &stateIcon);
}

void KeyManager::setMouseMode(int val) {
	if (val <= 4 && val >= 0)
		_mouseMode = val;
	else
		_mouseMode = 0;
	saveSettings();
	CheckMenuRadioItem(hMenu, ID_MOUSE_MODE_0, ID_MOUSE_MODE_4, ID_MOUSE_MODE_0 + _mouseMode, MF_BYCOMMAND);
}

void KeyManager::setRecordDoubleClick(int val) {
	if (val <= 1 && val >= 0)
		_recordDoubleClick = val;
	else
		_recordDoubleClick = 0;
	saveSettings();
	CheckMenuItem(hMenu, ID_RECORD_DOUBLE_CLICK, _recordDoubleClick == 0 ? MF_UNCHECKED : MF_CHECKED | MF_BYCOMMAND);
}

void KeyManager::setPlayCount(int val) {
	if (val == 0)
		val = INT_MAX;
	else if (val < 0)
		val = 1;
	_playCount = val;
	saveSettings();
}

void KeyManager::setSleepTime(int val) {
	if (val < 0) val = 0;
	_sleepTime = val;
	saveSettings();
	for (size_t i = 0; i < sizeof(_sleepTimeLevels) / sizeof(_sleepTimeLevels[0]); i++) {
		if (_sleepTime == _sleepTimeLevels[i]) {
			CheckMenuRadioItem(hMenu, ID_SLEEP_TIME_0, ID_SLEEP_TIME_9, ID_SLEEP_TIME_0 + i, MF_BYCOMMAND);
			return;
		}
	}
	CheckMenuRadioItem(hMenu, ID_SLEEP_TIME_0, ID_SLEEP_TIME_9, 0, MF_BYCOMMAND);
}

void KeyManager::setLoopSleepTime(int val) {
	if (val < 0) val = 0;
	_loopSleepTime = val;
	saveSettings();
	for (size_t i = 0; i < sizeof(_loopTimeLevels) / sizeof(_loopTimeLevels[0]); i++) {
		if (_loopSleepTime == _loopTimeLevels[i]) {
			CheckMenuRadioItem(hMenu, ID_LOOP_TIME_0, ID_LOOP_TIME_9, ID_LOOP_TIME_0 + i, MF_BYCOMMAND);
			return;
		}
	}
	CheckMenuRadioItem(hMenu, ID_LOOP_TIME_0, ID_LOOP_TIME_9, 0, MF_BYCOMMAND);
}

/////////////////////////////////////////////////////////////////////////
// private:
/////////////////////////////////////////////////////////////////////////
KeyManager::KeyManager() {
	/////////////////////////////////////////////////////////////
	// 录制与播放状态
	/////////////////////////////////////////////////////////////
	_onRecord = false;
	_onPlay = false;
	_onQuickRecord = false;
	_onQucikPlay = false;
	_userPressStopPlay = false;

	/////////////////////////////////////////////////////////////
	// 请求寄存器状态
	/////////////////////////////////////////////////////////////
	_onRequestRegister = false;
	_onRequestQucikRegister = false;
	_onRequestRecordRegister = false;
	_onRequestPlayRegister = false;

	/////////////////////////////////////////////////////////////
	// 寄存器
	/////////////////////////////////////////////////////////////
	_currentQuickRegister = "0";
	_currentRecordRegister = "0";
	_currentPlayRegister = "0";

	/////////////////////////////////////////////////////////////
	// 设置录制选项状态
	/////////////////////////////////////////////////////////////
	_onRecordSetting = false;

	/////////////////////////////////////////////////////////////
	// 设置播放选项状态
	/////////////////////////////////////////////////////////////
	_onPlaySetting = false;
	_onPlayCountSetting = false;
	_onSleepTimeSetting = false;
	_onLoopSleepTimeSetting = false;

	/////////////////////////////////////////////////////////////
	// 录制选项
	/////////////////////////////////////////////////////////////
	_mouseMode = 0;
	_recordDoubleClick = 0;

	/////////////////////////////////////////////////////////////
	// 播放选项
	/////////////////////////////////////////////////////////////
	_playCount = 1;
	_sleepTime = 10;
	_loopSleepTime = 100;

	/////////////////////////////////////////////////////////////
	// 记录时间
	/////////////////////////////////////////////////////////////
	setRecordTime(system_clock::now());

	/////////////////////////////////////////////////////////////
	// 屏幕大小
	/////////////////////////////////////////////////////////////

	HDC screen = GetDC(NULL);
	_screenX = GetDeviceCaps(screen, HORZRES);
	_screenY = GetDeviceCaps(screen, VERTRES);
	ReleaseDC(NULL, screen);

	/////////////////////////////////////////////////////////////
	// 鼠标坐标最后位置
	/////////////////////////////////////////////////////////////
	_lastX = 0;
	_lastY = 0;

	// 组合键状态
	_combinedKeyStates = std::map<std::string, bool>({
		{"LCtrl", false},
		{"RCtrl", false},
		{"LAtl", false},
		{"RAtl", false},
		{"LShift", false},
		{"RShift", false},
		{"LWin", false},
		{"RWin", false},
		{"Menu", false}
	});

	// 寄存器
	_registers = std::map <std::string, EventQueue>({
		{"`", EventQueue()},
		{"0", EventQueue()},
		{"1", EventQueue()},
		{"2", EventQueue()},
		{"3", EventQueue()},
		{"4", EventQueue()},
		{"5", EventQueue()},
		{"6", EventQueue()},
		{"7", EventQueue()},
		{"8", EventQueue()},
		{"9", EventQueue()},
		{"-", EventQueue()},
		{"=", EventQueue()},
		{"[", EventQueue()},
		{"]", EventQueue()},
		{"\\", EventQueue()},
		{";", EventQueue()},
		{"'", EventQueue()},
		{",", EventQueue()},
		{".", EventQueue()},
		{"/", EventQueue()},
		{"A", EventQueue()},
		{"B", EventQueue()},
		{"C", EventQueue()},
		{"D", EventQueue()},
		{"E", EventQueue()},
		{"F", EventQueue()},
		{"G", EventQueue()},
		{"H", EventQueue()},
		{"I", EventQueue()},
		{"J", EventQueue()},
		{"K", EventQueue()},
		{"L", EventQueue()},
		{"M", EventQueue()},
		{"N", EventQueue()},
		{"O", EventQueue()},
		{"P", EventQueue()},
		{"Q", EventQueue()},
		{"R", EventQueue()},
		{"S", EventQueue()},
		{"T", EventQueue()},
		{"U", EventQueue()},
		{"V", EventQueue()},
		{"W", EventQueue()},
		{"X", EventQueue()},
		{"Y", EventQueue()},
		{"Z", EventQueue()},
		{"Num 0", EventQueue()},
		{"Num 1", EventQueue()},
		{"Num 2", EventQueue()},
		{"Num 3", EventQueue()},
		{"Num 4", EventQueue()},
		{"Num 5", EventQueue()},
		{"Num 6", EventQueue()},
		{"Num 7", EventQueue()},
		{"Num 8", EventQueue()},
		{"Num 9", EventQueue()},
		{"Num /", EventQueue()},
		{"Num *", EventQueue()},
		{"Num -", EventQueue()},
		{"Num +", EventQueue()},
		{"Num .", EventQueue()},
		{"Up", EventQueue()},
		{"Down", EventQueue()},
		{"Left", EventQueue()},
		{"Right", EventQueue()}
	});

	// Todo: 缺一个右键
	// 所有按键
	_keys.push_back(Key(-1, -1, "Null"));

	_keys.push_back(Key(162, 29, "LCtrl"));
	_keys.push_back(Key(163, 29, "RCtrl"));
	_keys.push_back(Key(164, 56, "LAtl"));
	_keys.push_back(Key(165, 56, "RAtl"));
	_keys.push_back(Key(160, 42, "LShift"));
	_keys.push_back(Key(161, 54, "RShift"));
	_keys.push_back(Key(91, 91, "LWin"));
	_keys.push_back(Key(92, 92, "RWin"));
	_keys.push_back(Key(93, 93, "Menu"));

	_keys.push_back(Key(27, 1, "Esc"));
	_keys.push_back(Key(9, 15, "Tab"));
	_keys.push_back(Key(20, 58, "Caplock"));
	_keys.push_back(Key(8, 14, "BackSpace"));
	_keys.push_back(Key(13, 28, "Enter"));
	_keys.push_back(Key(32, 57, "Space"));

	_keys.push_back(Key(192, 41, "`"));
	_keys.push_back(Key(189, 12, "-"));
	_keys.push_back(Key(187, 13, "="));
	_keys.push_back(Key(219, 26, "["));
	_keys.push_back(Key(221, 27, "]"));
	_keys.push_back(Key(220, 43, "\\"));
	_keys.push_back(Key(186, 39, ";"));
	_keys.push_back(Key(222, 40, "'"));
	_keys.push_back(Key(188, 51, ","));
	_keys.push_back(Key(190, 52, "."));
	_keys.push_back(Key(191, 53, "/"));

	_keys.push_back(Key(44, 55, "PrtSc"));
	_keys.push_back(Key(145, 70, "ScrLk"));
	_keys.push_back(Key(19, 69, "Pause"));

	_keys.push_back(Key(45, 82, "Insert"));
	_keys.push_back(Key(46, 83, "Delete"));
	_keys.push_back(Key(36, 71, "Home"));
	_keys.push_back(Key(35, 79, "End"));
	_keys.push_back(Key(33, 73, "PageUp"));
	_keys.push_back(Key(34, 81, "PageDown"));

	_keys.push_back(Key(38, 72, "Up"));
	_keys.push_back(Key(40, 80, "Down"));
	_keys.push_back(Key(37, 75, "Left"));
	_keys.push_back(Key(39, 77, "Right"));

	_keys.push_back(Key(173, 0, "Mute"));
	_keys.push_back(Key(174, 0, "VolDown"));
	_keys.push_back(Key(175, 0, "VolUp"));
	_keys.push_back(Key(183, 0, "Calculator"));

	_keys.push_back(Key(112, 59, "F1"));
	_keys.push_back(Key(113, 60, "F2"));
	_keys.push_back(Key(114, 61, "F3"));
	_keys.push_back(Key(115, 62, "F4"));
	_keys.push_back(Key(116, 63, "F5"));
	_keys.push_back(Key(117, 64, "F6"));
	_keys.push_back(Key(118, 65, "F7"));
	_keys.push_back(Key(119, 66, "F8"));
	_keys.push_back(Key(120, 67, "F9"));
	_keys.push_back(Key(121, 68, "F10"));
	_keys.push_back(Key(122, 87, "F11"));
	_keys.push_back(Key(123, 88, "F12"));

	_keys.push_back(Key(144, 69, "NumLock"));
	_keys.push_back(Key(111, 53, "Num /"));
	_keys.push_back(Key(106, 55, "Num *"));
	_keys.push_back(Key(109, 74, "Num -"));
	_keys.push_back(Key(107, 78, "Num +"));
	_keys.push_back(Key(110, 83, "Num ."));
	_keys.push_back(Key(96, 82, "Num 0"));
	_keys.push_back(Key(97, 79, "Num 1"));
	_keys.push_back(Key(98, 80, "Num 2"));
	_keys.push_back(Key(99, 81, "Num 3"));
	_keys.push_back(Key(100, 75, "Num 4"));
	_keys.push_back(Key(101, 76, "Num 5"));
	_keys.push_back(Key(102, 77, "Num 6"));
	_keys.push_back(Key(103, 71, "Num 7"));
	_keys.push_back(Key(104, 72, "Num 8"));
	_keys.push_back(Key(105, 73, "Num 9"));

	_keys.push_back(Key(48, 11, "0"));
	_keys.push_back(Key(49, 2, "1"));
	_keys.push_back(Key(50, 3, "2"));
	_keys.push_back(Key(51, 4, "3"));
	_keys.push_back(Key(52, 5, "4"));
	_keys.push_back(Key(53, 6, "5"));
	_keys.push_back(Key(54, 7, "6"));
	_keys.push_back(Key(55, 8, "7"));
	_keys.push_back(Key(56, 9, "8"));
	_keys.push_back(Key(57, 10, "9"));

	_keys.push_back(Key(65, 30, "A"));
	_keys.push_back(Key(66, 48, "B"));
	_keys.push_back(Key(67, 46, "C"));
	_keys.push_back(Key(68, 32, "D"));
	_keys.push_back(Key(69, 18, "E"));
	_keys.push_back(Key(70, 33, "F"));
	_keys.push_back(Key(71, 34, "G"));
	_keys.push_back(Key(72, 35, "H"));
	_keys.push_back(Key(73, 23, "I"));
	_keys.push_back(Key(74, 36, "J"));
	_keys.push_back(Key(75, 37, "K"));
	_keys.push_back(Key(76, 38, "L"));
	_keys.push_back(Key(77, 50, "M"));
	_keys.push_back(Key(78, 49, "N"));
	_keys.push_back(Key(79, 24, "O"));
	_keys.push_back(Key(80, 25, "P"));
	_keys.push_back(Key(81, 16, "Q"));
	_keys.push_back(Key(82, 19, "R"));
	_keys.push_back(Key(83, 31, "S"));
	_keys.push_back(Key(84, 20, "T"));
	_keys.push_back(Key(85, 22, "U"));
	_keys.push_back(Key(86, 47, "V"));
	_keys.push_back(Key(87, 17, "W"));
	_keys.push_back(Key(88, 45, "X"));
	_keys.push_back(Key(89, 21, "Y"));
	_keys.push_back(Key(90, 44, "Z"));

	//////////////////////////////////////////////////////////////////
	// 载入设置
	//////////////////////////////////////////////////////////////////
	loadSettings();

	// 更新菜单项
	setRecordDoubleClick(getRecordDoubleClick());
	setMouseMode(getMouseMode());
	setSleepTime(getSleepTime());
	setLoopSleepTime(getLoopSleepTime());

	//////////////////////////////////////////////////////////////////
	// 载入寄存器
	//////////////////////////////////////////////////////////////////
	loadRegisters();
}
