#pragma once

#include "Value.hpp"
#include "Board.hpp"

#include <string>

class AI_Weights
{
	static std::string file;

	static long pawnWeights[9];
	static long opponentPawnRelativeFileWeights[3];

	static long pieceBoardPositionWeights[16][64];

	static long whitePawnValue(PieceType const* pieceBoard, int index);
	static long blackPawnValue(PieceType const* pieceBoard, int index);
	static long pieceValue(PieceType const* pieceBoard, int index);

	public:

	static void initialize(std::string pathToWeights);

	static void addEntropy();

	static bool loadData(std::string pathToWeights);
	static bool saveData();
	static bool saveData(std::string pathToWeights);

	static long long evaulate(Board const& board);

	static long captureDelta(Board const& board, Move move);
};

// void evaluationInitialize(std::string pathToWeights);

inline Value staleEval(Board const& board) // no moves possible, draw or death
{
	if (board.inCheck())
	{
		return Value(0, -board.plyNumber);
	}
	return Value();
}

inline Value evaluate(Board const& board)
{
	long long temp = AI_Weights::evaulate(board);

	return {board.blacksTurn ? -temp : temp, 0};
}

// bool exchangeDeltaValid(Board const& board, Move move);

inline long captureDelta(Board const& board, Move move)
{
	return AI_Weights::captureDelta(board, move);
}
