#pragma once
#include <string>
#include <boost/serialization/access.hpp>

class Event {
public:
	virtual void processEvent() = 0;
	virtual ~Event() = default;
private:
	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version) {

	}
};
