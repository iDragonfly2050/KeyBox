#include "stdafx.h"
#include <locale>
#include <codecvt>
#include <string>
#include <chrono>   
#include <thread>
#include <functional>
#include <mutex>
#include <sstream>
#include <memory>
#include "KeyParser.h"
#include "operate/Mouse.h"
#include "event/KeyEvent.h"
#include "manager/KeyManager.h"
#include "event/SleepEvent.h"
#include "event/StopEvent.h"
#include "event/MouseEvent.h"
using namespace std::chrono;

extern std::mutex mutex;

static const std::wstring GetWstr(const char *c) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wc = converter.from_bytes(std::string(c));
	return wc;
}

// 普通录制中
bool onRecord(int isKey, const Mouse & mouse, const Key & key, KeyEventState state) {
	////////////////////////////////////////////////////////////////////////////////
	// 录制前准备工作
	////////////////////////////////////////////////////////////////////////////////

	// 阻塞一部分按键
	if (isKey) {
		std::set<std::string> needToBlock1 = { "F1", "F2", "F3", "PageUp", "PageDown" };
		if (needToBlock1.count(key.getName())) return true;
	}

	static bool needToBlockFirstUp = true;
	static bool registerNeedToClear = false;
	static bool firstMouseEvent = true;
	static bool playing = false;
	auto keyManager = KeyManager::getInstance();
	auto & currentRecordRegister = keyManager->getCurrentRecordRegister();

	if (needToBlockFirstUp) {
		needToBlockFirstUp = false;
		registerNeedToClear = true;
		return true;
	}

	// 递归寄存器
	if (keyManager->isOnPlay()) {
		onPlay(true, mouse, key, state);
	}
	else if (isKey && keyManager->isOnRequestRegister()) {
		onRequestRegister(key, state);
		return true;
	}
	else if (isKey && keyManager->getCombinedKeyState("RWin") || keyManager->getCombinedKeyState("Menu")) {
		keyManager->setOnRequestRegister(true);
		keyManager->setOnRequestPlayRegister(true);
		// 特殊处理：普通播放完以后，RWin或Menu一直按着没有松开，此时再按下寄存器，应直接调用OnRequestRegister
		if (!key.isCombinedKey())
			onRequestRegister(key, state);
		return true;
	}

	// 阻塞不该录制的按键
	if (isKey) {
		std::set<std::string> needToBlock2 = { "F1", "F2", "F3", "F12", "RWin", "Menu", "PageUp", "PageDown" };
		if (needToBlock2.count(key.getName())) return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	// 真正开始录制
	////////////////////////////////////////////////////////////////////////////////

	// 键盘输入
	if (isKey) {
		auto now = system_clock::now();
		auto difft = now - keyManager->getRecordTime();
		keyManager->setRecordTime(now);
		auto duration = duration_cast<microseconds>(difft);

		currentRecordRegister.push(std::shared_ptr<SleepEvent>(new SleepEvent(duration)));

		// 清除第一次延时
		if (registerNeedToClear) {
			currentRecordRegister.clear();
			registerNeedToClear = false;
		}

		// 结束录制
		if (!needToBlockFirstUp && key == "PrtSc" && state == DOWN) {
			needToBlockFirstUp = true;
			firstMouseEvent = true;
			keyManager->setOnRecord(false);
			currentRecordRegister.push(std::shared_ptr<StopEvent>(new StopEvent()));
			keyManager->saveRegisters();
			// 录制的过程中在递归寄存器
			if (keyManager->isOnPlay() || keyManager->isOnQuickPlay())
				keyManager->setUserPressStopPlay(true);
			return true;
		}

		currentRecordRegister.push(std::shared_ptr<KeyEvent>(new KeyEvent(key, state)));
		return false;
	}
	// 鼠标输入
	else {
		auto mouseMode = keyManager->getMouseMode();
		auto mouseType = mouse.getMouseType();
		const int FAKE_SLEEP_TIME = 0;

		// 鼠标模式：0
		if (mouseMode == 0) return false;
		// 排除不明按键
		if (mouseType == Mouse::NOTYPE) return false;
		// 1和3模式不记录移动
		if (mouseType == Mouse::MOUSEMOVE_ABSOLUTE && (mouseMode == 1 || mouseMode == 3)) return false;

		auto now = system_clock::now();
		auto difft = now - keyManager->getRecordTime();
		keyManager->setRecordTime(now);
		auto duration = duration_cast<microseconds>(difft);

		currentRecordRegister.push(std::shared_ptr<SleepEvent>(new SleepEvent(duration)));

		// 清除第一次延时
		if (registerNeedToClear) {
			currentRecordRegister.clear();
			registerNeedToClear = false;
		}

		int x = mouse.getX();
		int y = mouse.getY();
		int wheelScrollDelta = mouse.getWheelScrollDelta();
		// 鼠标模式：1或2预处理
		if (mouseMode == 1 || mouseMode == 2) {
			// 记录下录制过程中第一次鼠标事件时的位置
			if (firstMouseEvent) {
				firstMouseEvent = false;
				POINT point;
				GetCursorPos(&point);
				keyManager->setLastX(point.x);
				keyManager->setLastY(point.y);
			}
			x -= keyManager->getLastX();
			y -= keyManager->getLastY();
		}

		// 非移动事件拆分成移动+点击
		if (mouseType != Mouse::MOUSEMOVE_ABSOLUTE) {
			// 移动
			if (mouseMode == 1 || mouseMode == 2) {
				currentRecordRegister.push(std::shared_ptr<MouseEvent>(new MouseEvent(Mouse(Mouse::MOUSEMOVE_RELATIVE, x, y, 0))));
				currentRecordRegister.push(std::shared_ptr<SleepEvent>(new SleepEvent(milliseconds(FAKE_SLEEP_TIME))));
			}
			else {
				currentRecordRegister.push(std::shared_ptr<MouseEvent>(new MouseEvent(Mouse(Mouse::MOUSEMOVE_ABSOLUTE, x, y, 0))));
				currentRecordRegister.push(std::shared_ptr<SleepEvent>(new SleepEvent(milliseconds(FAKE_SLEEP_TIME))));
			}
			// 点击
			currentRecordRegister.push(std::shared_ptr<MouseEvent>(new MouseEvent(Mouse(mouseType, 0, 0, wheelScrollDelta))));
		}
		// 移动事件的处理
		else {
			if (mouseMode == 2)
				currentRecordRegister.push(std::shared_ptr<MouseEvent>(new MouseEvent(Mouse(Mouse::MOUSEMOVE_RELATIVE, x, y, 0))));
			else
				currentRecordRegister.push(std::shared_ptr<MouseEvent>(new MouseEvent(Mouse(Mouse::MOUSEMOVE_ABSOLUTE, x, y, 0))));
			// 移动事件直接处理完毕
			return false;
		}

		// 合并双击左键
		if (keyManager->getRecordDoubleClick() == 1 && mouseType == Mouse::LBUTTON_UP) {
			int bugReport = 0;
			// 双击的过程中，最少产生15个事件，少于15个事件则不判断
			auto size = currentRecordRegister.getSize();
			if (size < 15) {
				bugReport = 1;
				return false;
			}

			size_t bottomI, mi = 0;
			Mouse::MouseType mt[4] = { Mouse::LBUTTON_UP, Mouse::LBUTTON_DOWN, Mouse::LBUTTON_UP, Mouse::LBUTTON_DOWN };
			std::shared_ptr<MouseEvent> mouseEvents[4];
			std::vector<std::pair<int, int>> moveXY;
			std::vector<std::shared_ptr<SleepEvent>> sleepEvents;

			for (int i = size - 1; i >= 0; i--) {
				auto event = currentRecordRegister.getEvent(i);
				// 双击过程中不允许有键盘事件
				if (std::dynamic_pointer_cast<KeyEvent>(event) != nullptr) {
					bugReport = 2;
					return false;
				}

				// 鼠标事件
				auto mouseEvent = std::dynamic_pointer_cast<MouseEvent>(event);
				if (mouseEvent != nullptr) {
					auto mouse = mouseEvent->getMouse();
					// 双击相关鼠标点击事件
					if (mouse.getMouseType() == mt[mi]) {
						mi++;
						bottomI = i;
					}
					// 鼠标绝对移动事件
					else if (mouse.getMouseType() == Mouse::MOUSEMOVE_ABSOLUTE) {
						moveXY.push_back(std::pair<int, int>(mouse.getX(), mouse.getY()));
					}
					// 鼠标相对移动事件
					else if (mouse.getMouseType() == Mouse::MOUSEMOVE_RELATIVE) {
						moveXY.push_back(std::pair<int, int>(mouse.getX() + keyManager->getLastX(), mouse.getY() + keyManager->getLastY()));
					}
					// 非双击相关鼠标事件
					else {
						bugReport = 3;
						return false;
					}
					continue;
				}

				// 延时时间
				auto sleepEvent = std::dynamic_pointer_cast<SleepEvent>(event);
				if (sleepEvent != nullptr)
					sleepEvents.push_back(sleepEvent);
			}

			// 没找齐双击的4个事件
			if (mi < 4) {
				bugReport = 4;
				return false;
			}

			// 计算双击时间
			milliseconds doubleClickSleepTime(0);
			for (auto & sleepEvent : sleepEvents)
				doubleClickSleepTime += duration_cast<milliseconds>(sleepEvent->getSleepTime());
			doubleClickSleepTime -= milliseconds(4 * FAKE_SLEEP_TIME);
			// 双击过程的时间太长，不算双击
			if (doubleClickSleepTime > milliseconds(GetDoubleClickTime())) {
				bugReport = 5;
				return false;
			}

			// 计算总偏移值
			if (moveXY.size() >= 2) {
				int dx = 0, dy = 0;
				for (size_t i = 1; i < moveXY.size(); i++) {
					dx += abs(moveXY[i].first - moveXY[i - 1].first);
					dy += abs(moveXY[i].second - moveXY[i - 1].second);
				}
				// 偏移值太大不算双击
				if (dx > 10 || dy > 10) {
					bugReport = 6;
					return false;
				}

			}

			// 弹出双击过程所有事件
			while (currentRecordRegister.getSize() > bottomI)
				currentRecordRegister.pop();

			// 压入双击事件
			currentRecordRegister.push(std::shared_ptr<MouseEvent>(new MouseEvent(Mouse::LBUTTON_DOUBLE_CLICK)));
		}
		return false;
	}
}

// 普通播放中
bool onPlay(int isKey, const Mouse & mouse, const Key & key, KeyEventState state) {
	static bool playing = false;
	static bool needToGetRelative = true;
	auto keyManager = KeyManager::getInstance();

	// Play状态中，但是还没开始播放
	if (!playing) {
		// 鼠标模式：1或2预处理
		auto mouseMode = keyManager->getMouseMode();
		if (mouseMode == 1 || mouseMode == 2) {
			// 记录下录制过程中第一次鼠标事件时的位置
			if (needToGetRelative) {
				needToGetRelative = false;
				POINT point;
				GetCursorPos(&point);
				keyManager->setLastX(point.x);
				keyManager->setLastY(point.y);
			}
		}
		playing = true;
		std::thread playQueue([]() {
			KeyManager::getInstance()->getCurrentPlayRegister().play();
			mutex.lock();
			KeyManager::getInstance()->setOnPlay(false);
			playing = false;
			needToGetRelative = true;
			mutex.unlock();
		});
		playQueue.detach();
		return true;
	}

	if (isKey) {
		// 播放的时候监听F12按下，触发中止
		if (playing && key == "F12" && state == DOWN) {
			keyManager->setUserPressStopPlay(true);
			return true;
		}

		// 播放的时候阻塞需要阻塞的按键
		std::set<std::string> needToBlock = { "F1", "F2", "F3", "F12", "PrtSc", "RWin", "Menu", "PageUp", "PageDown" };
		return needToBlock.count(key.getName()) > 0;
	}
	else {
		return false;
	}
}

// 快速录制中
bool onQuickRecord(int isKey, const Mouse & mouse, const Key & key, KeyEventState state) {
	////////////////////////////////////////////////////////////////////////////////
	// 录制前准备工作
	////////////////////////////////////////////////////////////////////////////////

	// 阻塞一部分按键
	if (isKey) {
		std::set<std::string> needToBlock1 = { "F1", "F3", "PrtSc", "PageUp", "PageDown" };
		if (needToBlock1.count(key.getName())) return true;
	}

	static bool needToBlockFirstUp = true;
	static bool firstMouseEvent = true;
	static bool registerNeedToClear = false;
	auto keyManager = KeyManager::getInstance();
	auto & currentQuickPlayRegister = keyManager->getCurrentQuickRegister();

	if (needToBlockFirstUp) {
		needToBlockFirstUp = false;
		registerNeedToClear = true;
		return true;
	}

	// 递归寄存器
	if (keyManager->isOnPlay()) {
		onPlay(true, Mouse(Mouse::NOTYPE), key, state);
	}
	else if (isKey && keyManager->isOnRequestRegister()) {
		onRequestRegister(key, state);
		return true;
	}
	else if (isKey && keyManager->getCombinedKeyState("RWin") || keyManager->getCombinedKeyState("Menu")) {
		keyManager->setOnRequestRegister(true);
		keyManager->setOnRequestPlayRegister(true);
		// 特殊处理：普通播放完以后，RWin或Menu一直按着没有松开，此时再按下寄存器，应直接调用OnRequestRegister
		if (!key.isCombinedKey())
			onRequestRegister(key, state);
		return true;
	}

	// 阻塞不该录制的按键
	std::set<std::string> needToBlock2 = { "F1", "F3", "F12", "PrtSc", "RWin", "Menu", "PageUp", "PageDown" };
	if (needToBlock2.count(key.getName())) return true;

	////////////////////////////////////////////////////////////////////////////////
	// 真正开始录制
	////////////////////////////////////////////////////////////////////////////////

	if (isKey) {
		auto now = system_clock::now();
		auto difft = now - keyManager->getRecordTime();
		keyManager->setRecordTime(now);
		auto duration = duration_cast<microseconds>(difft);

		currentQuickPlayRegister.push(std::shared_ptr<SleepEvent>(new SleepEvent(duration)));

		// 清除第一次延时
		if (registerNeedToClear) {
			currentQuickPlayRegister.clear();
			registerNeedToClear = false;
		}

		// 结束录制
		if (!needToBlockFirstUp && key == "F2" && state == DOWN) {
			needToBlockFirstUp = true;
			firstMouseEvent = true;
			keyManager->setOnQuickRecord(false);
			currentQuickPlayRegister.push(std::shared_ptr<StopEvent>(new StopEvent()));
			keyManager->saveRegisters();
			return true;
		}

		currentQuickPlayRegister.push(std::shared_ptr<KeyEvent>(new KeyEvent(key, state)));
		return false;
	}
	else {
		auto mouseMode = keyManager->getMouseMode();
		auto mouseType = mouse.getMouseType();
		const int FAKE_SLEEP_TIME = 0;

		// 鼠标模式：0
		if (mouseMode == 0) return false;
		// 排除不明按键
		if (mouseType == Mouse::NOTYPE) return false;
		// 1和3模式不记录移动
		if (mouseType == Mouse::MOUSEMOVE_ABSOLUTE && (mouseMode == 1 || mouseMode == 3)) return false;

		auto now = system_clock::now();
		auto difft = now - keyManager->getRecordTime();
		keyManager->setRecordTime(now);
		auto duration = duration_cast<microseconds>(difft);

		currentQuickPlayRegister.push(std::shared_ptr<SleepEvent>(new SleepEvent(duration)));

		// 清除第一次延时
		if (registerNeedToClear) {
			currentQuickPlayRegister.clear();
			registerNeedToClear = false;
		}

		int x = mouse.getX();
		int y = mouse.getY();
		int wheelScrollDelta = mouse.getWheelScrollDelta();
		// 鼠标模式：1或2预处理
		if (mouseMode == 1 || mouseMode == 2) {
			// 记录下录制过程中第一次鼠标事件时的位置
			if (firstMouseEvent) {
				firstMouseEvent = false;
				POINT point;
				GetCursorPos(&point);
				keyManager->setLastX(point.x);
				keyManager->setLastY(point.y);
			}
			x -= keyManager->getLastX();
			y -= keyManager->getLastY();
		}

		// 非移动事件拆分成移动+点击
		if (mouseType != Mouse::MOUSEMOVE_ABSOLUTE) {
			// 移动
			if (mouseMode == 1 || mouseMode == 2) {
				currentQuickPlayRegister.push(std::shared_ptr<MouseEvent>(new MouseEvent(Mouse(Mouse::MOUSEMOVE_RELATIVE, x, y, 0))));
				currentQuickPlayRegister.push(std::shared_ptr<SleepEvent>(new SleepEvent(milliseconds(FAKE_SLEEP_TIME))));
			}
			else {
				currentQuickPlayRegister.push(std::shared_ptr<MouseEvent>(new MouseEvent(Mouse(Mouse::MOUSEMOVE_ABSOLUTE, x, y, 0))));
				currentQuickPlayRegister.push(std::shared_ptr<SleepEvent>(new SleepEvent(milliseconds(FAKE_SLEEP_TIME))));
			}
			// 点击
			currentQuickPlayRegister.push(std::shared_ptr<MouseEvent>(new MouseEvent(Mouse(mouseType, 0, 0, wheelScrollDelta))));
		}
		// 移动事件的处理
		else {
			if (mouseMode == 2)
				currentQuickPlayRegister.push(std::shared_ptr<MouseEvent>(new MouseEvent(Mouse(Mouse::MOUSEMOVE_RELATIVE, x, y, 0))));
			else
				currentQuickPlayRegister.push(std::shared_ptr<MouseEvent>(new MouseEvent(Mouse(Mouse::MOUSEMOVE_ABSOLUTE, x, y, 0))));
			// 移动事件直接处理完毕
			return false;
		}

		// 合并双击左键
		if (keyManager->getRecordDoubleClick() == 1 && mouseType == Mouse::LBUTTON_UP) {
			int bugReport = 0;
			// 双击的过程中，最少产生15个事件，少于15个事件则不判断
			auto size = currentQuickPlayRegister.getSize();
			if (size < 15) {
				bugReport = 1;
				return false;
			}

			size_t bottomI, mi = 0;
			Mouse::MouseType mt[4] = { Mouse::LBUTTON_UP, Mouse::LBUTTON_DOWN, Mouse::LBUTTON_UP, Mouse::LBUTTON_DOWN };
			std::shared_ptr<MouseEvent> mouseEvents[4];
			std::vector<std::pair<int, int>> moveXY;
			std::vector<std::shared_ptr<SleepEvent>> sleepEvents;

			for (int i = size - 1; i >= 0; i--) {
				auto event = currentQuickPlayRegister.getEvent(i);
				// 双击过程中不允许有键盘事件
				if (std::dynamic_pointer_cast<KeyEvent>(event) != nullptr) {
					bugReport = 2;
					return false;
				}

				// 鼠标事件
				auto mouseEvent = std::dynamic_pointer_cast<MouseEvent>(event);
				if (mouseEvent != nullptr) {
					auto mouse = mouseEvent->getMouse();
					// 双击相关鼠标点击事件
					if (mouse.getMouseType() == mt[mi]) {
						mi++;
						bottomI = i;
					}
					// 鼠标绝对移动事件
					else if (mouse.getMouseType() == Mouse::MOUSEMOVE_ABSOLUTE) {
						moveXY.push_back(std::pair<int, int>(mouse.getX(), mouse.getY()));
					}
					// 鼠标相对移动事件
					else if (mouse.getMouseType() == Mouse::MOUSEMOVE_RELATIVE) {
						moveXY.push_back(std::pair<int, int>(mouse.getX() + keyManager->getLastX(), mouse.getY() + keyManager->getLastY()));
					}
					// 非双击相关鼠标事件
					else {
						bugReport = 3;
						return false;
					}
					continue;
				}

				// 延时时间
				auto sleepEvent = std::dynamic_pointer_cast<SleepEvent>(event);
				if (sleepEvent != nullptr)
					sleepEvents.push_back(sleepEvent);
			}

			// 没找齐双击的4个事件
			if (mi < 4) {
				bugReport = 4;
				return false;
			}

			// 计算双击时间
			milliseconds doubleClickSleepTime(0);
			for (auto & sleepEvent : sleepEvents)
				doubleClickSleepTime += duration_cast<milliseconds>(sleepEvent->getSleepTime());
			doubleClickSleepTime -= milliseconds(4 * FAKE_SLEEP_TIME);
			// 双击过程的时间太长，不算双击
			if (doubleClickSleepTime > milliseconds(GetDoubleClickTime())) {
				bugReport = 5;
				return false;
			}

			// 计算总偏移值
			if (moveXY.size() >= 2) {
				int dx = 0, dy = 0;
				for (size_t i = 1; i < moveXY.size(); i++) {
					dx += abs(moveXY[i].first - moveXY[i - 1].first);
					dy += abs(moveXY[i].second - moveXY[i - 1].second);
				}
				// 偏移值太大不算双击
				if (dx > 10 || dy > 10) {
					bugReport = 6;
					return false;
				}

			}

			// 弹出双击过程所有事件
			while (currentQuickPlayRegister.getSize() > bottomI)
				currentQuickPlayRegister.pop();

			// 压入双击事件
			currentQuickPlayRegister.push(std::shared_ptr<MouseEvent>(new MouseEvent(Mouse::LBUTTON_DOUBLE_CLICK)));
		}
		return false;
	}
}

// 快速播放中
bool onQucikPlay(int isKey, const Mouse & mouse, const Key & key, KeyEventState state) {
	static bool playing = false;
	static bool needToGetRelative = true;
	auto keyManager = KeyManager::getInstance();

	// QucikPlay状态中，但是还没开始播放
	if (!playing) {
		// 鼠标模式：1或2预处理
		auto mouseMode = keyManager->getMouseMode();
		if (mouseMode == 1 || mouseMode == 2) {
			// 记录下录制过程中第一次鼠标事件时的位置
			if (needToGetRelative) {
				needToGetRelative = false;
				POINT point;
				GetCursorPos(&point);
				keyManager->setLastX(point.x);
				keyManager->setLastY(point.y);
			}
		}
		playing = true;
		std::thread playQueue([]() {
			KeyManager::getInstance()->getCurrentQuickRegister().play();
			mutex.lock();
			KeyManager::getInstance()->setOnQucikPlay(false);
			playing = false;
			needToGetRelative = true;
			mutex.unlock();
		});
		playQueue.detach();
		return true;
	}

	if (isKey) {
		// 播放的时候监听F12按下，触发中止
		if (playing && key == "F12" && state == DOWN)
			keyManager->setUserPressStopPlay(true);

		// 播放的时候阻塞需要阻塞的按键
		std::set<std::string> needToBlock = { "F1", "F2", "F3", "F12", "PrtSc", "RWin", "Menu", "PageUp", "PageDown" };
		return needToBlock.count(key.getName()) > 0;
	}
	else {
		return false;
	}
}

// 请求寄存器中
bool onRequestRegister(const Key & key, KeyEventState state) {
	auto keyManager = KeyManager::getInstance();

	if (keyManager->isOnRequestQucikRegister()) {
		if (state == DOWN) {
			std::set<std::string> needToBlock = { "F1", "F2", "F3", "F12", "PrtSc", "PageUp", "PageDown" };
			if (needToBlock.count(key.getName())) return true;
			if (key == "RWin" || key == "Menu")
				keyManager->setCurrentQuickRegister("0");
			else
				keyManager->setCurrentQuickRegister(key.getName());
			keyManager->setOnRequestQucikRegister(false);
			keyManager->setOnRequestRegister(false);
		}
	}
	else if (keyManager->isOnRequestRecordRegister()) {
		if (state == DOWN) {
			std::set<std::string> needToBlock = { "F1", "F2", "F3", "F12", "RWin", "Menu", "PageUp", "PageDown" };
			if (needToBlock.count(key.getName())) return  true;
			if (key == "PrtSc") {
				keyManager->setOnRequestRecordRegister(false);
				keyManager->setOnRequestRegister(false);
				return true;
			}
			keyManager->setOnRequestRecordRegister(false);
			keyManager->setOnRequestRegister(false);
			keyManager->setCurrentRecordRegister(key.getName());
			keyManager->setOnRecord(true);
		}
	}
	else if (keyManager->isOnRequestPlayRegister()) {
		if ((key == "RWin" || key == "Menu") && state == UP) {
			keyManager->setOnRequestPlayRegister(false);
			if (!keyManager->isOnRecord() && !keyManager->isOnQuickRecord())
				keyManager->setOnRequestQucikRegister(true);
			else
				keyManager->setOnRequestRegister(false);
		}
		else if (state == DOWN) {
			std::set<std::string> needToBlock = { "F1", "F2", "F3", "F12", "PrtSc", "RWin", "Menu", "PageUp", "PageDown" };
			if (needToBlock.count(key.getName())) return  true;
			keyManager->setCurrentPlayRegister(key.getName());
			keyManager->setOnRequestPlayRegister(false);
			keyManager->setOnRequestRegister(false);
			keyManager->setOnPlay(true);
		}
	}
	return true;
}

// 设置录制选项中
bool onRecordSetting(const Key & key, KeyEventState state) {
	// 阻塞需要阻塞的按键
	std::set<std::string> needToBlock = { "F1", "F2", "F3", "F12", "PrtSc", "RWin", "Menu", "PageDown" };
	if (needToBlock.count(key.getName())) return true;

	// 拦截所有弹起
	if (state == UP) return true;

	auto keyManager = KeyManager::getInstance();
	static char mod = 0;
	static std::set<char> modeSet = { 'M', 'D' };
	static std::string settings;

	// 结算
	if ((key.isAlpha() || key == "PageUp") && settings != "") {
		switch (mod) {
			case 'M': keyManager->setMouseMode(std::stoi(settings)); break;
			case 'D': keyManager->setRecordDoubleClick(std::stoi(settings)); break;
			default: break;
		}
		mod = 0;
		settings.clear();
	}

	// 结束
	if (key == "PageUp") {
		keyManager->setOnRecordSetting(false);
		return true;
	}

	// 切换模式
	if (key.isAlpha()) {
		char alphaChar = key.getAlphaChar();
		if (modeSet.count(alphaChar))
			mod = alphaChar;
		return true;
	}

	// 录入数字
	if (key.isNumber() || key.isNumPadNumber())
		settings += key.getNumberChar();
	return true;
}

// 设置播放选项中
bool onPlaySetting(const Key & key, KeyEventState state) {
	// 阻塞需要阻塞的按键
	std::set<std::string> needToBlock = { "F1", "F2", "F3", "F12", "PrtSc", "RWin", "Menu", "PageUp" };
	if (needToBlock.count(key.getName())) return true;

	// 拦截所有弹起
	if (state == UP) return true;

	auto keyManager = KeyManager::getInstance();
	static char mod = 0;
	static std::set<char> modeSet = { 'T', 'L', 'C' };
	static std::string settings;

	// 结算
	if ((key.isAlpha() || key == "PageDown") && settings != "") {
		switch (mod) {
			case 'T': keyManager->setSleepTime(std::stoi(settings)); break;
			case 'L': keyManager->setLoopSleepTime(std::stoi(settings)); break;
			case 'C': keyManager->setPlayCount(std::stoi(settings)); break;
			default: break;
		}
		mod = 0;
		settings.clear();
	}

	// 结束
	if (key == "PageDown") {
		keyManager->setOnPlaySetting(false);
		return true;
	}

	// 切换模式
	if (key.isAlpha()) {
		char alphaChar = key.getAlphaChar();
		if (modeSet.count(alphaChar))
			mod = alphaChar;
		return true;
	}

	// 录入数字
	if (key.isNumber() || key.isNumPadNumber())
		settings += key.getNumberChar();
	return true;
}

LRESULT CALLBACK parserKey(HWND hWnd, HHOOK keyboardHook, LPARAM lParam, int nCode, WPARAM wParam) {
	bool needToBlockKey = false;
	if (nCode >= 0) {
		// 初始化变量
// 		wchar_t text[50];
// 		const wchar_t *info = NULL;
		auto keyManager = KeyManager::getInstance();
// 		HDC hdc;
		PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
		Key key(p->vkCode, p->scanCode);
		KeyEventState state;

		// 判断按键状态
		switch (wParam) {
			case WM_KEYDOWN:
				state = DOWN;
// 				info = L"普通按鍵按下";
				break;
			case WM_KEYUP:
				state = UP;
// 				info = L"普通按鍵抬起";
				break;
			case WM_SYSKEYDOWN:
				state = DOWN;
// 				info = L"系統按鍵按下";
				break;
			case WM_SYSKEYUP:
				state = UP;
// 				info = L"系統按鍵抬起";
				break;
			default:
				state = NOSTATE;
// 				info = L"未知错误";
				break;
		}

		// 记录组合键状态
		if (key.isCombinedKey()) {
			keyManager->processCombinedKey(key, state);
		}

		if (keyManager->isOnRecord()) {
			needToBlockKey = onRecord(true, Mouse(Mouse::NOTYPE), key, state);
		}
		else if (keyManager->isOnQuickRecord()) {
			needToBlockKey = onQuickRecord(true, Mouse(Mouse::NOTYPE), key, state);
		}
		else if (keyManager->isOnPlay()) {
			needToBlockKey = onPlay(true, Mouse(Mouse::NOTYPE), key, state);
		}
		else if (keyManager->isOnQuickPlay()) {
			needToBlockKey = onQucikPlay(true, Mouse(Mouse::NOTYPE), key, state);
		}
		else if (keyManager->isOnRequestRegister()) {
			needToBlockKey = onRequestRegister(key, state);
		}
		else if (keyManager->isOnRecordSetting()) {
			needToBlockKey = onRecordSetting(key, state);
		}
		else if (keyManager->isOnPlaySetting()) {
			needToBlockKey = onPlaySetting(key, state);
		}
		else if (key == "F2" && state == DOWN) {
			// 快速录制
			keyManager->setOnQuickRecord(true);
			needToBlockKey = true;
		}
		else if (key == "F1" && state == DOWN) {
			// 快速播放
			keyManager->setOnQucikPlay(true);
			needToBlockKey = true;
		}
		else if (key == "PrtSc" && state == DOWN && !keyManager->getCombinedKeyState("LWin")) {
			// 普通录制
			keyManager->setOnRequestRegister(true);
			keyManager->setOnRequestRecordRegister(true);
			needToBlockKey = true;
		}
		else if (key == "PageUp" && state == DOWN) {
			// 设置录制选项
			keyManager->setOnRecordSetting(true);
			needToBlockKey = true;
		}
		else if (key == "PageDown" && state == DOWN) {
			// 设置播放选项
			keyManager->setOnPlaySetting(true);
			needToBlockKey = true;
		}
		else if (keyManager->getCombinedKeyState("RWin") || keyManager->getCombinedKeyState("Menu")) {
			keyManager->setOnRequestRegister(true);
			keyManager->setOnRequestPlayRegister(true);
			needToBlockKey = true;
			// 特殊处理：普通播放完以后，RWin或Menu一直按着没有松开，此时再按下寄存器，应直接调用OnRequestRegister
			if (!key.isCombinedKey())
				onRequestRegister(key, state);
		}

// 		std::wstring clearStr = L"                                                                        ";
// 		// 输出信息
// 		hdc = GetDC(hWnd);
// 		wsprintf(text, L"%s - 键盘码 [%04d], 扫描码 [%04d]  ", info, key.getVkCode(), key.getScanCode());
// 		TextOut(hdc, 10, 10, clearStr.c_str(), clearStr.size());
// 		TextOut(hdc, 10, 10, text, wcslen(text));
// 		wsprintf(text, L"按鍵目测为： %s                 ", GetWstr(key.getName()).c_str());
// 		TextOut(hdc, 10, 30, clearStr.c_str(), clearStr.size());
// 		TextOut(hdc, 10, 30, text, wcslen(text));
// 		wsprintf(text, L"是否快速录制中: %s   ", keyManager->isOnQuickRecord() ? L"true" : L"false");
// 		TextOut(hdc, 10, 50, clearStr.c_str(), clearStr.size());
// 		TextOut(hdc, 10, 50, text, wcslen(text));
// 		wsprintf(text, L"是否播放中: %s   ", keyManager->isOnQuickPlay() ? L"true" : L"false");
// 		TextOut(hdc, 10, 70, clearStr.c_str(), clearStr.size());
// 		TextOut(hdc, 10, 70, text, wcslen(text));
// 		ReleaseDC(hWnd, hdc);
	}

	// 将消息继续往下传递
	if (needToBlockKey)
		return true;
	else
		return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

LRESULT CALLBACK parserMouse(HWND hWnd, HHOOK mouseHook, LPARAM lParam, int nCode, WPARAM wParam) {
	if (nCode >= 0) {
// 		HDC hdc;
// 		std::wstring text1, text2;
		auto keyManager = KeyManager::getInstance();
		POINT point = ((MSLLHOOKSTRUCT*)lParam)->pt;
		auto zDelta = (short)HIWORD(((MSLLHOOKSTRUCT*)lParam)->mouseData);
		Mouse::MouseType mouseType = Mouse::NOTYPE;

		switch (wParam) {
			case WM_LBUTTONDOWN:
// 				text1 = L"左键按下";
				mouseType = Mouse::LBUTTON_DOWN;
				break;
			case WM_LBUTTONUP:
// 				text1 = L"左键抬起";
				mouseType = Mouse::LBUTTON_UP;
				break;
			case WM_RBUTTONDOWN:
// 				text1 = L"右键按下";
				mouseType = Mouse::RBUTTON_DOWN;
				break;
			case WM_RBUTTONUP:
// 				text1 = L"右键抬起";
				mouseType = Mouse::RBUTTON_UP;
				break;
			case WM_MBUTTONDOWN:
// 				text1 = L"中键按下";
				mouseType = Mouse::MBUTTON_DOWN;
				break;
			case WM_MBUTTONUP:
// 				text1 = L"中键抬起";
				mouseType = Mouse::MBUTTON_UP;
				break;
			case WM_MOUSEMOVE:
// 				text1 = L"鼠标移动";
				mouseType = Mouse::MOUSEMOVE_ABSOLUTE;
				break;
			case WM_MOUSEWHEEL:
				mouseType = Mouse::MOUSEWHEEL_SCROLL;
// 				if (zDelta > 0)
// 					text1 = L"滚轮滑上";
// 				else
// 					text1 = L"滚轮滑下";
// 				brea
			default:
				mouseType = Mouse::NOTYPE;
				break;
		}

		Mouse mouse(mouseType, point.x, point.y, zDelta);

		if (keyManager->isOnRecord()) {
			onRecord(false, mouse, Key(), NOSTATE);
		}
		else if (keyManager->isOnQuickRecord()) {
			onQuickRecord(false, mouse, Key(), NOSTATE);
		}
		else if (keyManager->isOnPlay()) {
			onPlay(false, mouse, Key(), NOSTATE);
		}
		else if (keyManager->isOnQuickPlay()) {
			onQucikPlay(false, mouse, Key(), NOSTATE);
		}

// 		HDC screen = GetDC(NULL);
// 		double screenX = GetDeviceCaps(screen, HORZRES);
// 		double screenY = GetDeviceCaps(screen, VERTRES);
// 		ReleaseDC(NULL, screen);
// 
// 		POINT p;
// 		GetCursorPos(&p);
// 
// 		std::wstringstream sout;
// 		sout << "(" << point.x << "," << point.y << ")" << " " << zDelta << " " << screenX << " " << screenY << " " << p.x << " " << p.y;
// 		text2 = sout.str();
// 
// 		std::wstring clearStr = L"                                                                        ";
// 		hdc = GetDC(hWnd);
// 		TextOut(hdc, 10, 90, clearStr.c_str(), clearStr.size());
// 		TextOut(hdc, 10, 90, text1.c_str(), text1.size());
// 		TextOut(hdc, 10, 110, clearStr.c_str(), clearStr.size());
// 		TextOut(hdc, 10, 110, text2.c_str(), text2.size());
// 		ReleaseDC(hWnd, hdc);
	}
	return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}
