#include "Evaluation.hpp"

// #include <iostream>

using namespace std;

string AI_Weights::file;

long AI_Weights::pawnWeights[9];
long AI_Weights::opponentPawnRelativeFileWeights[3];
long AI_Weights::pieceBoardPositionWeights[16][64];


void AI_Weights::initialize(std::string pathToWeights)
{
	if (pathToWeights != "")
	{
		if (loadData(pathToWeights))
		{
			file = pathToWeights;
			return;
		}
	}

	//default values

	//piece values and position on board values

	for (int rank = 0; rank < 8; ++rank)
	{
		for (int file = 0; file < 8; ++file)
		{
			int i = rank * 8 + file;

			pieceBoardPositionWeights[Piece::Pawn | Piece::White][i] = 1000;
			pieceBoardPositionWeights[Piece::Pawn | Piece::Black][i] = -1000;
			switch (rank)
			{
				case 1:
				pieceBoardPositionWeights[Piece::Pawn | Piece::Black][i] = -5000;
				break;

				case 2:
				pieceBoardPositionWeights[Piece::Pawn | Piece::Black][i] = -3000;
				break;

				case 3:
				pieceBoardPositionWeights[Piece::Pawn | Piece::Black][i] = -1200;
				if (file >= 2 && file <= 5)
				{
					pieceBoardPositionWeights[Piece::Pawn | Piece::White][i] = 1100;
				}
				break;

				case 4:
				pieceBoardPositionWeights[Piece::Pawn | Piece::White][i] = 1200;
				if (file >= 2 && file <= 5)
				{
					pieceBoardPositionWeights[Piece::Pawn | Piece::Black][i] = -1100;
				}
				break;

				case 5:
				pieceBoardPositionWeights[Piece::Pawn | Piece::White][i] = 3000;
				break;

				case 6:
				pieceBoardPositionWeights[Piece::Pawn | Piece::White][i] = 5000;
				break;
			}

			pieceBoardPositionWeights[Piece::Bishop | Piece::White][i] = 3250;
			pieceBoardPositionWeights[Piece::Bishop | Piece::Black][i] = -3250;

			pieceBoardPositionWeights[Piece::Knight | Piece::White][i] = 3000;
			pieceBoardPositionWeights[Piece::Knight | Piece::Black][i] = -3000;
			if (rank >= 2 && rank <= 5 && file >= 2 && file <= 5)
			{
				pieceBoardPositionWeights[Piece::Knight | Piece::White][i] += 100;
				pieceBoardPositionWeights[Piece::Knight | Piece::Black][i] += -100;
			}

			pieceBoardPositionWeights[Piece::Rook | Piece::White][i] = 5000;
			pieceBoardPositionWeights[Piece::Rook | Piece::Black][i] = -5000;

			pieceBoardPositionWeights[Piece::Queen | Piece::White][i] = 9000;
			pieceBoardPositionWeights[Piece::Queen | Piece::Black][i] = -9000;

			if (rank == 0)//prefer things out than in
			{
				pieceBoardPositionWeights[Piece::Bishop | Piece::White][i] -= 50;
				pieceBoardPositionWeights[Piece::Knight | Piece::White][i] -= 50;
			}
			else if (rank == 7)
			{
				pieceBoardPositionWeights[Piece::Bishop | Piece::Black][i] -= -50;
				pieceBoardPositionWeights[Piece::Knight | Piece::Black][i] -= -50;
			}

			pieceBoardPositionWeights[Piece::King | Piece::White][i] = 0;
			pieceBoardPositionWeights[Piece::King | Piece::Black][i] = 0;
			if (rank == 0 && (file > 5 || file < 3))
			{
				pieceBoardPositionWeights[Piece::King | Piece::White][i] = 100;
			}
			else if (rank == 7 && (file > 5 || file < 3))
			{
				pieceBoardPositionWeights[Piece::King | Piece::Black][i] = -100;
			}

			//non-piece values
			pieceBoardPositionWeights[Piece::White][i] = 0;
			pieceBoardPositionWeights[Piece::Black][i] = 0;
			pieceBoardPositionWeights[Piece::All][i] = 0;
			pieceBoardPositionWeights[Piece::None][i] = 0;
		}
	}

	//pass pawn weights

	opponentPawnRelativeFileWeights[0] = -500;
	// opponentPawnRelativeFileWeights[0] = -50;
	// opponentPawnRelativeFileWeights[1] = -50;
	// opponentPawnRelativeFileWeights[2] = -50;

	//pawn convolutional weights

	//guessed values
	pawnWeights[0] = 20;//twentieth of a pawn; strong structure
	pawnWeights[1] = -20;//twentieth of a pawn; doubled pawns
	pawnWeights[3] = 0;//straight is neutral

	//constraint value
	pawnWeights[4] = 0;//don't count yourself

	//symmetry values
	pawnWeights[2] = pawnWeights[0];
	pawnWeights[6] = pawnWeights[0];
	pawnWeights[8] = pawnWeights[0];
	pawnWeights[7] = pawnWeights[1];
	pawnWeights[5] = pawnWeights[5];
}


void AI_Weights::addEntropy()
{
	//TODO
}

bool AI_Weights::loadData(std::string pathToWeights)
{
	//TODO
	return false;
}
bool AI_Weights::saveData()
{
	//TODO
	return false;
}
bool AI_Weights::saveData(std::string pathToWeights)
{
	//TODO
	return false;
}


long AI_Weights::whitePawnValue(PieceType const* pieceBoard, int index)
{
	long output = pieceValue(pieceBoard, index);

	static constexpr PieceType pawnType = Piece::Pawn | Piece::White;
	static constexpr PieceType opponentPawnType = Piece::Pawn | Piece::Black;

	int rank = index / 8;
	int file = index % 8;

	bool leftOccupied = false;
	bool middleOccupied = false;
	bool rightOccupied = false;

	switch (file)
	{
		case 0://on left edge
		for (int r = - 1; r <= 1; ++r)
		{
			output += pawnWeights[3 * r + 1 + 3] * (pieceBoard[index + 8 * r] == pawnType);
			output += pawnWeights[3 * r + 2 + 3] * (pieceBoard[index + 8 * r + 1] == pawnType);
		}
		for (int r = rank + 1; r < 8; ++r)
		{
			middleOccupied = middleOccupied || pieceBoard[8 * r + file] == opponentPawnType;
			rightOccupied = rightOccupied || pieceBoard[8 * r + file + 1] == opponentPawnType;
		}
		break;

		case 7:
		for (int r = - 1; r <= 1; ++r)
		{
			output += pawnWeights[3 * r + 0 + 3] * (pieceBoard[index + 8 * r - 1] == pawnType);
			output += pawnWeights[3 * r + 1 + 3] * (pieceBoard[index + 8 * r] == pawnType);
		}
		for (int r = rank + 1; r < 8; ++r)
		{
			leftOccupied = leftOccupied || pieceBoard[8 * r + file - 1] == opponentPawnType;
			middleOccupied = middleOccupied || pieceBoard[8 * r + file] == opponentPawnType;
		}
		break;

		default:
		for (int r = - 1; r <= 1; ++r)
		{
			output += pawnWeights[3 * r + 0 + 3] * (pieceBoard[index + 8 * r - 1] == pawnType);
			output += pawnWeights[3 * r + 1 + 3] * (pieceBoard[index + 8 * r] == pawnType);
			output += pawnWeights[3 * r + 2 + 3] * (pieceBoard[index + 8 * r + 1] == pawnType);
		}
		for (int r = rank + 1; r < 8; ++r)
		{
			leftOccupied = leftOccupied || pieceBoard[8 * r + file - 1] == opponentPawnType;
			middleOccupied = middleOccupied || pieceBoard[8 * r + file] == opponentPawnType;
			rightOccupied = rightOccupied || pieceBoard[8 * r + file + 1] == opponentPawnType;
		}
		break;
	}

	output += (leftOccupied || middleOccupied || rightOccupied) * opponentPawnRelativeFileWeights[0];
	// output += leftOccupied * opponentPawnRelativeFileWeights[0];
	// output += middleOccupied * opponentPawnRelativeFileWeights[1];
	// output += rightOccupied * opponentPawnRelativeFileWeights[2];

	return output;
}
long AI_Weights::blackPawnValue(PieceType const* pieceBoard, int index)
{
	long output = pieceValue(pieceBoard, index);

	static constexpr PieceType pawnType = Piece::Pawn | Piece::Black;
	static constexpr PieceType opponentPawnType = Piece::Pawn | Piece::White;

	int rank = index / 8;
	int file = index % 8;

	bool leftOccupied = false;
	bool middleOccupied = false;
	bool rightOccupied = false;

	switch (file)
	{
		case 0://on left edge
		for (int r = - 1; r <= 1; ++r)
		{
			output += pawnWeights[3 * (1 - r) + 1 + 3] * (pieceBoard[index + 8 * r] == pawnType);
			output += pawnWeights[3 * (1 - r) + 2 + 3] * (pieceBoard[index + 8 * r + 1] == pawnType);
		}
		for (int r = rank - 1; r >= 0; --r)
		{
			middleOccupied = middleOccupied || pieceBoard[8 * r + file] == opponentPawnType;
			rightOccupied = rightOccupied || pieceBoard[8 * r + file + 1] == opponentPawnType;
		}
		break;

		case 7:
		for (int r = - 1; r <= 1; ++r)
		{
			output += pawnWeights[3 * (1 - r) + 0 + 3] * (pieceBoard[index + 8 * r - 1] == pawnType);
			output += pawnWeights[3 * (1 - r) + 1 + 3] * (pieceBoard[index + 8 * r] == pawnType);
		}
		for (int r = rank - 1; r >= 0; --r)
		{
			leftOccupied = leftOccupied || pieceBoard[8 * r + file - 1] == opponentPawnType;
			middleOccupied = middleOccupied || pieceBoard[8 * r + file] == opponentPawnType;
		}
		break;

		default:
		for (int r = - 1; r <= 1; ++r)
		{
			output += pawnWeights[3 * (1 - r) + 0 + 3] * (pieceBoard[index + 8 * r - 1] == pawnType);
			output += pawnWeights[3 * (1 - r) + 1 + 3] * (pieceBoard[index + 8 * r] == pawnType);
			output += pawnWeights[3 * (1 - r) + 2 + 3] * (pieceBoard[index + 8 * r + 1] == pawnType);
		}
		for (int r = rank - 1; r >= 0; --r)
		{
			leftOccupied = leftOccupied || pieceBoard[8 * r + file - 1] == opponentPawnType;
			middleOccupied = middleOccupied || pieceBoard[8 * r + file] == opponentPawnType;
			rightOccupied = rightOccupied || pieceBoard[8 * r + file + 1] == opponentPawnType;
		}
		break;
	}

	output -= (leftOccupied || middleOccupied || rightOccupied) * opponentPawnRelativeFileWeights[0];

	// output -= leftOccupied * opponentPawnRelativeFileWeights[0];
	// output -= middleOccupied * opponentPawnRelativeFileWeights[1];
	// output -= rightOccupied * opponentPawnRelativeFileWeights[2];

	return output;
}

long AI_Weights::pieceValue(PieceType const* pieceBoard, int index)
{
	return pieceBoardPositionWeights[pieceBoard[index]][index];
}


long long AI_Weights::evaulate(Board const& board)
{
	long long output = 0;
	for (int i = 0; i < 64; ++i)
	{
		// if ((board.fullBoard[i] & Piece::Pawn) == Piece::Pawn)
		// {
		// 	if ((board.fullBoard[i] & Piece::Team) == Piece::Black)
		// 	{
		// 		output += blackPawnValue(board.fullBoard, i);
		// 	}
		// 	else
		// 	{
		// 		output += whitePawnValue(board.fullBoard, i);
		// 	}
		// }
		output += pieceValue(board.fullBoard, i);
	}

	// output += 1000 * cardinality(board.pieceBoards[Piece::Pawn | Piece::White]);
	// output += 3000 * cardinality(board.pieceBoards[Piece::Knight | Piece::White]);
	// output += 3000 * cardinality(board.pieceBoards[Piece::Bishop | Piece::White]);
	// output += 5000 * cardinality(board.pieceBoards[Piece::Rook | Piece::White]);
	// output += 9000 * cardinality(board.pieceBoards[Piece::Queen | Piece::White]);

	// output -= 1000 * cardinality(board.pieceBoards[Piece::Pawn | Piece::Black]);
	// output -= 3000 * cardinality(board.pieceBoards[Piece::Knight | Piece::Black]);
	// output -= 3000 * cardinality(board.pieceBoards[Piece::Bishop | Piece::Black]);
	// output -= 5000 * cardinality(board.pieceBoards[Piece::Rook | Piece::Black]);
	// output -= 9000 * cardinality(board.pieceBoards[Piece::Queen | Piece::Black]);

	output += 500;

	// cout << output << endl;

	return output;
}

long AI_Weights::captureDelta(Board const& board, Move move)
{
	// if (((move.deltaSource ^ move.deltaTarget) & Piece::Team))
	// 	return pieceBoardPositionWeights[move.deltaSource ^ move.deltaTarget][move.targetSquare];
	// else
	// 	return pieceBoardPositionWeights[move.deltaSource ^ move.deltaTarget][move.targetSquare]
	// 		- pieceBoardPositionWeights[move.deltaSource][move.sourceSquare];
	return pieceBoardPositionWeights[move.deltaSource ^ move.deltaTarget][move.targetSquare]
		- pieceBoardPositionWeights[move.deltaSource][move.sourceSquare];
}

// void evaluationInitialize(std::string pathToWeights)
// {
// 	if (!weights.loadData(pathToWeights))
// 	{
// 		weights.buildDefaultValues();
// 	}
// 	// for (int i = 0; i < 64; ++i)
// 	// {
// 	// 	linearPiecePositionValue[Piece::Pawn | Piece::Black][i] = -1000;
// 	// 	linearPiecePositionValue[Piece::Bishop | Piece::Black][i] = -3000;
// 	// 	linearPiecePositionValue[Piece::Knight | Piece::Black][i] = -3000;
// 	// 	linearPiecePositionValue[Piece::Rook | Piece::Black][i] = -5000;
// 	// 	linearPiecePositionValue[Piece::Queen | Piece::Black][i] = -9000;

// 	// 	linearPiecePositionValue[Piece::Pawn | Piece::White][i] = 1000;
// 	// 	linearPiecePositionValue[Piece::Bishop | Piece::White][i] = 3000;
// 	// 	linearPiecePositionValue[Piece::Knight | Piece::White][i] = 3000;
// 	// 	linearPiecePositionValue[Piece::Rook | Piece::White][i] = 5000;
// 	// 	linearPiecePositionValue[Piece::Queen | Piece::White][i] = 9000;

// 	// 	linearPiecePositionValue[Piece::King | Piece::Black][i] = 0;
// 	// 	linearPiecePositionValue[Piece::King | Piece::White][i] = 0;
// 	// 	linearPiecePositionValue[Piece::Black][i] = 0;
// 	// 	linearPiecePositionValue[Piece::White][i] = 0;
// 	// 	linearPiecePositionValue[Piece::All][i] = 0;
// 	// 	linearPiecePositionValue[Piece::None][i] = 0;
// 	// }
// }

// int pawnStructureScore(BitBoard whitePawnBoard, BitBoard blackPawnBoard)
// {
// 	return 0;
// }

// Value evaluate(Board const& board)
// {
// 	Value value;

// 	for (int i = 0; i < 64; ++i)
// 	{
// 		PieceType piece(board.fullBoard[i]);
// 		value.materialValue += linearPiecePositionValue[piece][i];
// 	}
// 	// value.materialValue += 1000 * cardinality(board.pieceBoards[Piece::Pawn | Piece::White]);
// 	// value.materialValue += 3000 * cardinality(board.pieceBoards[Piece::Knight | Piece::White]);
// 	// value.materialValue += 3000 * cardinality(board.pieceBoards[Piece::Bishop | Piece::White]);
// 	// value.materialValue += 5000 * cardinality(board.pieceBoards[Piece::Rook | Piece::White]);
// 	// value.materialValue += 9000 * cardinality(board.pieceBoards[Piece::Queen | Piece::White]);

// 	// value.materialValue -= 1000 * cardinality(board.pieceBoards[Piece::Pawn | Piece::Black]);
// 	// value.materialValue -= 3000 * cardinality(board.pieceBoards[Piece::Knight | Piece::Black]);
// 	// value.materialValue -= 3000 * cardinality(board.pieceBoards[Piece::Bishop | Piece::Black]);
// 	// value.materialValue -= 5000 * cardinality(board.pieceBoards[Piece::Rook | Piece::Black]);
// 	// value.materialValue -= 9000 * cardinality(board.pieceBoards[Piece::Queen | Piece::Black]);

// 	value.materialValue += 500;
// 	return board.blacksTurn ? -value : value;
// }

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
