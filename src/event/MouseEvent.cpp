#include "stdafx.h"
#include "Windows.h"
#include <thread>
#include <chrono>
#include "SleepEvent.h"
#include "MouseEvent.h"
#include "operate/Mouse.h"
#include "manager/KeyManager.h"
#include <boost/serialization/export.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
BOOST_CLASS_EXPORT_IMPLEMENT(MouseEvent)

using namespace std::chrono;

MouseEvent::MouseEvent(Mouse mouse) {
	_mouse = mouse;
}

void MouseEvent::processEvent() {
	switch (_mouse.getMouseType()) {
		case Mouse::MOUSEMOVE_ABSOLUTE:
			sendMouseEvent(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, _mouse.getX(), _mouse.getY());
			break;
		case Mouse::MOUSEMOVE_RELATIVE:
			sendMouseEvent(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, _mouse.getX() + KeyManager::getInstance()->getLastX(), _mouse.getY() + KeyManager::getInstance()->getLastY());
			break;
		case Mouse::LBUTTON_DOWN:
			sendMouseEvent(MOUSEEVENTF_LEFTDOWN);
			break;
		case Mouse::LBUTTON_UP:
			sendMouseEvent(MOUSEEVENTF_LEFTUP);
			break;
		case Mouse::RBUTTON_DOWN:
			sendMouseEvent(MOUSEEVENTF_RIGHTDOWN);
			break;
		case Mouse::RBUTTON_UP:
			sendMouseEvent(MOUSEEVENTF_RIGHTUP);
			break;
		case Mouse::MBUTTON_DOWN:
			sendMouseEvent(MOUSEEVENTF_MIDDLEDOWN);
			break;
		case Mouse::MBUTTON_UP:
			sendMouseEvent(MOUSEEVENTF_MIDDLEUP);
			break;
		case Mouse::LBUTTON_DOUBLE_CLICK:
			MouseEvent(Mouse(Mouse::LBUTTON_DOWN)).processEvent();
			SleepEvent(milliseconds(50), true).processEvent();
			MouseEvent(Mouse(Mouse::LBUTTON_UP)).processEvent();
			SleepEvent(milliseconds(50), true).processEvent();
			MouseEvent(Mouse(Mouse::LBUTTON_DOWN)).processEvent();
			SleepEvent(milliseconds(50), true).processEvent();
			MouseEvent(Mouse(Mouse::LBUTTON_UP)).processEvent();
			break;
			// 无视右键与中键双击
// 		case Mouse::RBUTTON_DOUBLE_CLICK:
// 			MouseEvent(Mouse(Mouse::RBUTTON_DOWN)).processEvent();
// 			SleepEvent(milliseconds(50)).processEvent();
// 			MouseEvent(Mouse(Mouse::RBUTTON_UP)).processEvent();
// 			SleepEvent(milliseconds(50)).processEvent();
// 			MouseEvent(Mouse(Mouse::RBUTTON_DOWN)).processEvent();
// 			SleepEvent(milliseconds(50)).processEvent();
// 			MouseEvent(Mouse(Mouse::RBUTTON_UP)).processEvent();
// 			break;
// 		case Mouse::MBUTTON_DOUBLE_CLICK:
// 			MouseEvent(Mouse(Mouse::MBUTTON_DOWN)).processEvent();
// 			SleepEvent(milliseconds(50)).processEvent();
// 			MouseEvent(Mouse(Mouse::MBUTTON_UP)).processEvent();
// 			SleepEvent(milliseconds(50)).processEvent();
// 			MouseEvent(Mouse(Mouse::MBUTTON_DOWN)).processEvent();
// 			SleepEvent(milliseconds(50)).processEvent();
// 			MouseEvent(Mouse(Mouse::MBUTTON_UP)).processEvent();
// 			break;
		case Mouse::MOUSEWHEEL_SCROLL:
			sendMouseEvent(MOUSEEVENTF_WHEEL, 0, 0, _mouse.getWheelScrollDelta());
			break;
		default:
			break;
	}
}

void MouseEvent::sendMouseEvent(int event, int x, int y, int mouseData) {
	static auto keyManager = KeyManager::getInstance();
	static double factorX = (double)0xFFFF / (double)keyManager->getScreenX();
	static double factorY = (double)0xFFFF / (double)keyManager->getScreenY();
	MOUSEINPUT mouseInput = { (int)(x * factorX), (int)(y * factorY), mouseData, event, 0, 0 };
	INPUT input = { INPUT_MOUSE, mouseInput };
	INPUT inputs[1] = { input };
	SendInput(1, inputs, sizeof(input));
}
