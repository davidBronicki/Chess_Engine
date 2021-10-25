#include "Evaluation.hpp"


long linearPiecePositionValue[16][64];

void evaluationInitialize()
{
	for (int i = 0; i < 64; ++i)
	{
		linearPiecePositionValue[Piece::Pawn | Piece::Black][i] = -1000;
		linearPiecePositionValue[Piece::Bishop | Piece::Black][i] = -3000;
		linearPiecePositionValue[Piece::Knight | Piece::Black][i] = -3000;
		linearPiecePositionValue[Piece::Rook | Piece::Black][i] = -5000;
		linearPiecePositionValue[Piece::Queen | Piece::Black][i] = -9000;

		linearPiecePositionValue[Piece::Pawn | Piece::White][i] = 1000;
		linearPiecePositionValue[Piece::Bishop | Piece::White][i] = 3000;
		linearPiecePositionValue[Piece::Knight | Piece::White][i] = 3000;
		linearPiecePositionValue[Piece::Rook | Piece::White][i] = 5000;
		linearPiecePositionValue[Piece::Queen | Piece::White][i] = 9000;

		linearPiecePositionValue[Piece::King | Piece::Black][i] = 0;
		linearPiecePositionValue[Piece::King | Piece::White][i] = 0;
		linearPiecePositionValue[Piece::Black][i] = 0;
		linearPiecePositionValue[Piece::White][i] = 0;
		linearPiecePositionValue[Piece::All][i] = 0;
		linearPiecePositionValue[Piece::None][i] = 0;
	}
}


Value evaluate(Board const& board)
{
	Value value;
	value.materialValue += 1000 * cardinality(board.pieceBoards[Piece::Pawn | Piece::White]);
	value.materialValue += 3000 * cardinality(board.pieceBoards[Piece::Knight | Piece::White]);
	value.materialValue += 3000 * cardinality(board.pieceBoards[Piece::Bishop | Piece::White]);
	value.materialValue += 5000 * cardinality(board.pieceBoards[Piece::Rook | Piece::White]);
	value.materialValue += 9000 * cardinality(board.pieceBoards[Piece::Queen | Piece::White]);

	value.materialValue -= 1000 * cardinality(board.pieceBoards[Piece::Pawn | Piece::Black]);
	value.materialValue -= 3000 * cardinality(board.pieceBoards[Piece::Knight | Piece::Black]);
	value.materialValue -= 3000 * cardinality(board.pieceBoards[Piece::Bishop | Piece::Black]);
	value.materialValue -= 5000 * cardinality(board.pieceBoards[Piece::Rook | Piece::Black]);
	value.materialValue -= 9000 * cardinality(board.pieceBoards[Piece::Queen | Piece::Black]);

	value.materialValue += 500;
	return board.blacksTurn ? -value : value;
}

// bool exchangeDeltaValid(Board const& board, Move move, long materialDelta)
// {
// 	//assumed to be a non-quiescent move

// 	if (move.deltaSource == move.deltaTarget)
// 	{
// 		//must be a check or check escape, always valid
// 		return true;
// 	}

// 	// //capture or promotion
// 	// if ((move.deltaSource & Piece::Occupied) == Piece::Pawn)
// 	// {
// 	// 	//promotion or using a pawn to capture
// 	// 	//always worth looking at
// 	// 	return true;
// 	// }

// 	// //capture with non-pawn
// 	// long initialValueGained = linearPiecePositionValue[move.deltaSource ^ move.deltaTarget][move.targetSquare];

	
// }

long captureDelta(Board const& board, Move move)
{
	if (((move.deltaSource ^ move.deltaTarget) & Piece::Team))
		return linearPiecePositionValue[move.deltaSource ^ move.deltaTarget][move.targetSquare];
	else
		return linearPiecePositionValue[move.deltaSource ^ move.deltaTarget][move.targetSquare]
			- linearPiecePositionValue[move.deltaSource][move.sourceSquare];
}
