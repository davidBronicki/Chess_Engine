#pragma once

#include <cmath>

struct Value
{
	float value;
	int movesToMate;

	Value():value(0), movesToMate(0){}
	Value(float value):value(value), movesToMate(0){}
	Value(float value, int movesToMate):value(value), movesToMate(movesToMate){}
};

inline bool operator==(Value a, Value b)
{
	return a.value == b.value && a.movesToMate == b.movesToMate;
}

inline bool operator>(Value a, Value b)
{
	return a.value > b.value
		|| (a.value == HUGE_VALF && b.value == HUGE_VALF && a.movesToMate < b.movesToMate)
		|| (a.value ==-HUGE_VALF && b.value ==-HUGE_VALF && a.movesToMate > b.movesToMate);
}

inline bool operator<(Value a, Value b)
{
	return b > a;
}

inline bool operator>=(Value a, Value b)
{
	return !(a < b);
}

inline bool operator<=(Value a, Value b)
{
	return !(a > b);
}
