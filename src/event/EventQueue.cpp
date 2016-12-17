#include "stdafx.h"
#include "EventQueue.h"
#include "manager/KeyManager.h"
#include <thread>

void EventQueue::push(std::shared_ptr<Event> event) {
	_events.push_back(std::shared_ptr<Event>(event));
}

void EventQueue::pop() {
	_events.pop_back();
}

std::shared_ptr<Event> EventQueue::getEvent(int index) const {
	if (index >= 0 && (size_t)index < _events.size())
		return _events[index];
	else
		return std::shared_ptr<Event>(nullptr);
}

size_t EventQueue::getSize() const {
	return _events.size();
}

void EventQueue::clear() {
	_events.clear();
}

void EventQueue::play() const {
	if (_events.empty()) return;

	auto keyManager = KeyManager::getInstance();
	for (int i = 0; i < keyManager->getPlayCount(); i++) {
		for (auto & event : _events) {
			if (keyManager->isUserPressStopPlay()) {
				keyManager->setUserPressStopPlay(false);
				return;
			}
			event->processEvent();
		}
		// 还有下一次
		if (i + 1 < keyManager->getPlayCount())
			std::this_thread::sleep_for(milliseconds(keyManager->getLoopSleepTime()));
	}
}
