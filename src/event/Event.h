#pragma once
#include <string>
#include <boost/serialization/access.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/assume_abstract.hpp>
#include <boost/serialization/export.hpp>

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

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Event)
BOOST_CLASS_EXPORT_KEY(Event)
