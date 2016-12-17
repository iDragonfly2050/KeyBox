#include "stdafx.h"
#include "Mouse.h"

Mouse::Mouse(MouseType mouseType /*= NOTYPE*/, int x /*= 0*/, int y /*= 0*/, int wheelScrollDelta /*= 0*/) {
	_mouseType = mouseType;
	_x = x;
	_y = y;
	_wheelScrollDelta = wheelScrollDelta;
}
