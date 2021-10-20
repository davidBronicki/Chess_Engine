#pragma once

#include "BitDefs.hpp"

#include <vector>

struct EvaluationContext
{
	//uci required parameters and flags
	bool goFlag, stopFlag, quitFlag, debugFlag;
	bool ponderFlag, mateSearchFlag, infiniteFlag;
	int moveTime, wTime, bTime, wInc, bInc, movesToGo, maxDepth, maxNodes;

	//custom parameters
	PlyType depthWalkValue;//initial search depth when deep searching
	PlyType quiescenceSearchDepth;
	uc cores;//number of calculation threads to use
	uc keepNStacks;//keep best N lines

	std::vector<std::vector<Move>> bestMoveStacks;

	std::vector<Move> activeMoves;
	std::vector<HashType> activeHashes;//3 move repetition tracking

	EvaluationContext();

	void resetGoFlags();
};
