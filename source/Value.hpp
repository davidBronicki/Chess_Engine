#pragma once

#include <math.h>

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

	// inline Value slightlyBetter() const
	// {
	// 	return better(1);
	// }
	// inline Value slightlyWorse() const
	// {
	// 	return worse(1);
	// }

	inline void incrPly()
	{
		if (matePlyNumber > 0)
			++matePlyNumber;
		else if (matePlyNumber < 0)
			--matePlyNumber;
	}

	inline void decrPly()
	{
		if (matePlyNumber > 1)
			--matePlyNumber;
		else if (matePlyNumber < -1)
			++matePlyNumber;
	}
};

inline bool isNullWindow(Value a, Value b)
{
	return a.matePlyNumber == 0 && b.matePlyNumber == 0 &&
			std::abs(a.materialValue - b.materialValue) <= 2 ||
		std::abs(a.matePlyNumber - b.matePlyNumber) > 0 &&
		std::abs(a.matePlyNumber - b.matePlyNumber) <= 3;
}

inline bool operator==(Value a, Value b)
{
	return a.materialValue == b.materialValue && a.matePlyNumber == b.matePlyNumber;
}

inline Value operator-(Value a)
{
	return {-a.materialValue, -a.matePlyNumber};
}

inline bool operator>(Value a, Value b)
{
	return a.materialValue > b.materialValue ||
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
