#include "stdafx.h"
#include <thread>
#include <boost/serialization/export.hpp>
#include "SleepEvent.h"
#include "manager/KeyManager.h"
BOOST_CLASS_EXPORT(SleepEvent)

SleepEvent::SleepEvent(microseconds sleepTime, bool trueSleep) {
	_sleepTime = sleepTime;
	_trueSleep = trueSleep;
}

void SleepEvent::processEvent() {
	auto keyManager = KeyManager::getInstance();
	if (keyManager->getSleepTime() == 0 || _trueSleep) {
		std::this_thread::sleep_for(_sleepTime);
	}
	else {
		std::this_thread::sleep_for(milliseconds(keyManager->getSleepTime()));
	}
}
