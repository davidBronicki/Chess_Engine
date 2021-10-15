#include "Board.hpp"

#include <random>

ull Board::SquareHashes[64][16];
ull Board::ExtraHashes[256];
ull Board::TurnHash;

void Board::initializeGlobals()
{
	std::mt19937 eng(111111ull);
	std::uniform_int_distribution<ull> distr;
	for (int i = 0; i < 64; ++i)
	{
		for (int j = 0; j < 16; ++j)
		{
			Board::SquareHashes[i][j] = distr(eng);
		}
	}
	for (int i = 0; i < 256; ++i)
	{
		Board::ExtraHashes[i] = distr(eng);
	}
	Board::TurnHash = distr(eng);
}

Board::Board()
:
blacksTurn(false),
extraInfo(Extra::Black_Long | Extra::Black_Short
	| Extra::White_Long | Extra::White_Short),
plySinceLastPawnOrCapture(0),
plyNumber(0)
{
	for (int i = 0; i < 16; ++i)
	{
		pieceBoards[i] = BitBoard{0ull};
	}
	for (int i = 0; i < 64; ++i)
	{
		fullBoard[i] = 0;
	}
}

void Board::resetHash()
{
	hash = 0ull;
	for (int i = 0; i < 64; ++i)
	{
		hash ^= Board::SquareHashes[i][fullBoard[i]];
	}
	hash ^= Board::ExtraHashes[extraInfo];
	hash ^= blacksTurn * Board::TurnHash;
}
