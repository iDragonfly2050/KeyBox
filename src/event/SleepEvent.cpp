#include "stdafx.h"
#include <thread>
#include "manager/KeyManager.h"
#include "SleepEvent.h"
#include <boost/serialization/export.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
BOOST_CLASS_EXPORT_IMPLEMENT(SleepEvent)

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
