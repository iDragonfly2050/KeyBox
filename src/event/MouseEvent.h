#pragma once
#include "Event.h"
#include "Windows.h"
#include "operate/Mouse.h"
#include <boost/serialization/access.hpp>

class MouseEvent : public Event {
public:
	MouseEvent(Mouse mouse);

	virtual void processEvent() override;

	const Mouse & getMouse() const { return _mouse; }

private:
	void sendMouseEvent(int event, int x = 0, int y = 0, int mouseData = 0);

	Mouse _mouse;

private:
	MouseEvent() = default;
	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version) {
		ar & boost::serialization::base_object<Event>(*this);
		ar & _mouse;
	}
};
