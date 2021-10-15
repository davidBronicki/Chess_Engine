#pragma once

#include <cmath>

struct Value
{
	float value;
	int movesToMate;

	Value(){}
	Value(float value):value(value){}
	Value(float value, int movesToMate):value(value), movesToMate(movesToMate){}
};

inline bool operator==(Value a, Value b)
{
	return a.value == b.value && a.movesToMate == b.movesToMate;
}

inline bool operator>(Value a, Value b)
{
	return a.value > b.value
		|| (a.value == HUGE_VALF && a.movesToMate < b.movesToMate)
		|| (a.value ==-HUGE_VALF && a.movesToMate > b.movesToMate);
}

inline bool operator<(Value a, Value b)
{
	return b < a;
}

inline bool operator>=(Value a, Value b)
{
	return !(a < b);
}

inline bool operator<=(Value a, Value b)
{
	return !(a > b);
}
