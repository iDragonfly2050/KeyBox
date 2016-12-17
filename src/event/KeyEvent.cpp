#include "stdafx.h"
#include "Windows.h"
#include <thread>
#include <boost/serialization/export.hpp>
#include "KeyEvent.h"
BOOST_CLASS_EXPORT(KeyEvent)

/////////////////////////////////////////////////////////////////////////
// public:
/////////////////////////////////////////////////////////////////////////

void KeyEvent::processEvent() {
	switch (_state) {
		case UP:
			keybd_event(_key.getVkCode(), _key.getScanCode(), KEYEVENTF_KEYUP, 0);
			break;
		case DOWN:
			keybd_event(_key.getVkCode(), _key.getScanCode(), 0, 0);
			break;
		default:
			break;
	}
}
