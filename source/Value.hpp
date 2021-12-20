#pragma once

#include <math.h>
#include <iostream>
// #include <compare>

struct Value
{
	long materialValue;//milli pawn? centi pawn?

	//matePlyNumber has an inverted evaluation direction (lower is better)
	short matePlyNumber;//negative = white getting mated

	Value():materialValue(0), matePlyNumber(0){}
	Value(long materialValue):materialValue(materialValue), matePlyNumber(0){}
	Value(long materialValue, long movesToMate):materialValue(materialValue), matePlyNumber(movesToMate){}

	inline Value better(long materialIncrease) const
	{
		if (matePlyNumber == 0)
			return Value{materialValue + materialIncrease};
		if (matePlyNumber != 1)//max val
			return Value{0, matePlyNumber - 1};
		return *this;
	}
	inline Value worse(long materialDecrease) const
	{
		if (matePlyNumber == 0)
			return Value{materialValue - materialDecrease};
		if (matePlyNumber != -1)//min val
			return Value{0, matePlyNumber + 1};
		return *this;
	}

	// inline bool operator==(Value other) const
	// {
	// 	return materialValue == other.materialValue && matePlyNumber == other.matePlyNumber;
	// }

	// inline auto operator<=>(Value other) const
	// {
	// 	return a.materialValue > b.materialValue && a.matePlyNumber == 0 && b.matePlyNumber == 0 ||
	// 		a.matePlyNumber != 0 && b.matePlyNumber != 0 &&
	// 			static_cast<unsigned short>(a.matePlyNumber) < static_cast<unsigned short>(b.matePlyNumber) ||
	// 		(a.matePlyNumber == 0 || b.matePlyNumber == 0) && a.matePlyNumber > b.matePlyNumber;
	// }

};

inline bool isNullWindow(Value a, Value b)
{
	return a.matePlyNumber == 0 && b.matePlyNumber == 0 &&
			std::abs(a.materialValue - b.materialValue) <= 2 ||
		std::abs(a.matePlyNumber - b.matePlyNumber) > 0 &&
		std::abs(a.matePlyNumber - b.matePlyNumber) <= 3;
}

inline Value operator-(Value a)
{
	return {-a.materialValue, -a.matePlyNumber};
}

inline bool operator==(Value a, Value b)
{
	return a.materialValue == b.materialValue && a.matePlyNumber == b.matePlyNumber;
}

// inline bool operator!=(Value a, Value b)
// {
// 	return !(a == b);
// }

inline bool operator>(Value a, Value b)
{
	return a.materialValue > b.materialValue && a.matePlyNumber == 0 && b.matePlyNumber == 0 ||
		a.matePlyNumber != 0 && b.matePlyNumber != 0 &&
			static_cast<unsigned short>(a.matePlyNumber) < static_cast<unsigned short>(b.matePlyNumber) ||
		(a.matePlyNumber == 0 || b.matePlyNumber == 0) && a.matePlyNumber > b.matePlyNumber;
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

inline std::ostream& operator<<(std::ostream& os, Value value)
{
	if (value.matePlyNumber == 0)
	{
		os << value.materialValue;
	}
	else
	{
		os << "M";
		os << value.matePlyNumber;
	}
	return os;
}
