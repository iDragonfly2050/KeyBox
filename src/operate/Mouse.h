#pragma once

#include <boost/serialization/access.hpp>

class Mouse {
public:
	enum MouseType {
		NOTYPE,
		MOUSEMOVE_ABSOLUTE,
		MOUSEMOVE_RELATIVE,
		LBUTTON_DOWN,
		LBUTTON_UP,
		RBUTTON_DOWN,
		RBUTTON_UP,
		MBUTTON_DOWN,
		MBUTTON_UP,
		LBUTTON_DOUBLE_CLICK,
		// 无视右键与中键双击
// 		MBUTTON_DOUBLE_CLICK,
// 		RBUTTON_DOUBLE_CLICK,
		MOUSEWHEEL_SCROLL
	};

	Mouse(MouseType mouseType = NOTYPE, int x = 0, int y = 0, int wheelScrollDelta = 0);

	MouseType getMouseType() const { return _mouseType; }
	int getX() const { return _x; }
	int getY() const { return _y; }
	int getWheelScrollDelta() const { return _wheelScrollDelta; }

private:
	MouseType _mouseType;
	int _x;
	int _y;
	int _wheelScrollDelta;

private:
	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version) {
		ar & _mouseType;
		ar & _x;
		ar & _y;
		ar & _wheelScrollDelta;
	}
};
