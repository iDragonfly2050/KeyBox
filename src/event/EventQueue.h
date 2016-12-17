#pragma once
#include "stdafx.h"
#include <vector>
#include "KeyEvent.h"
#include <memory>
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>

class EventQueue {
public:
	EventQueue() {}

	void push(std::shared_ptr<Event> event);
	void pop();
	std::shared_ptr<Event> getEvent(int index) const;
	size_t getSize() const;
	void clear();
	void play() const;

private:
	std::vector<std::shared_ptr<Event>> _events;

private:
	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version) {
		ar & _events;
	}
};
