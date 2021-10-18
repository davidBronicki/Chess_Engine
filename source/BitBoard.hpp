#pragma once

#include "BitDefs.hpp"

typedef ull BitBoard;

inline constexpr BitBoard boardUnion(
	BitBoard b1,
	BitBoard const& b2)
{
	return b1 |= b2;
}

inline constexpr BitBoard boardIntersect(
	BitBoard b1,
	BitBoard const& b2)
{
	return b1 &= b2;
}


inline BitBoard shiftLeft(
	BitBoard board,
	uc shift)
{
	return (board & leftMoveMask[shift]) >> shift;
}

inline BitBoard shiftRight(
	BitBoard board,
	uc shift)
{
	return (board & rightMoveMask[shift]) << shift;
}

inline constexpr BitBoard shiftUp(
	BitBoard board,
	uc shift)
{
	return board <<= (shift * 8);
}

inline constexpr BitBoard shiftDown(
	BitBoard board,
	uc shift)
{
	return board >>= (shift * 8);
}

inline constexpr BitBoard indexToBitBoard(
	uc boardIndex)
{
	return BitBoard{boardIndex < 64 ? 1ull << boardIndex : 0ull};
}

inline constexpr int firstIndex(
	BitBoard board)
{
	return board == 0 ? 64 : __builtin_ctzll(board);
}

inline constexpr int lastIndex(
	BitBoard board)
{
	return board == 0 ? 64 : 63 - __builtin_clzll(board);
}

inline constexpr int cardinality(
	BitBoard board)
{
	return __builtin_popcountll(board);
}
