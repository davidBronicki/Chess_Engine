#pragma once

#include <math.h>

struct Value
{
	float value;
	short matePlyNumber;

	Value():value(0), matePlyNumber(0){}
	Value(float value):value(value), matePlyNumber(0){}
	Value(float value, short movesToMate):value(value), matePlyNumber(movesToMate){}
};

inline bool operator==(Value a, Value b)
{
	return a.value == b.value && a.matePlyNumber == b.matePlyNumber;
}

inline Value operator-(Value a)
{
	return {-a.value, a.matePlyNumber};
}

inline bool operator>(Value a, Value b)
{
	return a.value > b.value
		|| (a.value == HUGE_VALF && b.value == HUGE_VALF && a.matePlyNumber < b.matePlyNumber)
		|| (a.value ==-HUGE_VALF && b.value ==-HUGE_VALF && a.matePlyNumber > b.matePlyNumber);
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
