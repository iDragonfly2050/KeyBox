#pragma once
#include <chrono>
#include <boost/serialization/access.hpp>
#include "Event.h"
#include "operate/Key.h"
#include <boost/serialization/export.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
using namespace std::chrono;

enum KeyEventState { NOSTATE, UP, DOWN };

class KeyEvent : public Event {
public:
	KeyEvent(const Key & key, KeyEventState state) : _key(key), _state(state) {}

	virtual void processEvent() override;

private:
	Key _key;
	KeyEventState _state = NOSTATE;

private:
	KeyEvent() = default;
	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version) {
		ar & boost::serialization::base_object<Event>(*this);
		ar & _key;
		ar & _state;
	}
};

BOOST_CLASS_EXPORT_KEY(KeyEvent)
