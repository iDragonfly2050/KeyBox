#pragma once
#include "Event.h"
#include <chrono>
#include <boost/serialization/access.hpp>
#include <boost/serialization/export.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
using namespace std::chrono;

class SleepEvent : public Event {
public:
	SleepEvent(microseconds sleepTime, bool trueSleep = false);
	virtual void processEvent() override;
	std::chrono::microseconds getSleepTime() const { return _sleepTime; }

private:
	microseconds _sleepTime;
	bool _trueSleep;

private:
	SleepEvent() : _sleepTime(0), _trueSleep(false) { }

	friend class boost::serialization::access;

	template<class Archive>
	void save(Archive & ar, const unsigned int version) const {
		ar << boost::serialization::base_object<const Event>(*this);
		__int64 sleepTime = _sleepTime.count();
		ar << sleepTime;
	}

	template<class Archive>
	void load(Archive & ar, const unsigned int version) {
		ar >> boost::serialization::base_object<Event>(*this);
		__int64 sleepTime;
		ar >> sleepTime;
		_sleepTime = milliseconds(sleepTime);
	}

	template<class Archive>
	void serialize(Archive & ar, const unsigned int file_version) {
		boost::serialization::split_member(ar, *this, file_version);
		ar & _trueSleep;
	}
};

BOOST_CLASS_EXPORT_KEY(SleepEvent)
