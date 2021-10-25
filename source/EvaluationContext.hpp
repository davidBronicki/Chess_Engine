#pragma once

#include "BitDefs.hpp"

#include <vector>

template <typename T>
class UpdateableData
{
	bool dataChanged;
	T data;

	public:

	UpdateableData(T const& init) :
		data(init), dataChanged(true){}

	UpdateableData() :
		dataChanged(false){}

	void set(T&& newData)
	{
		dataChanged = true;
		data = newData;
	}

	bool changed() const
	{
		return dataChanged;
	}

	T const& get()
	{
		dataChanged = false;
		return data;
	}
};

struct PV_Data
{
	std::vector<Move> moves;
	Value value;
	int specificDepth;
	int specificSelectiveDepth;
};

struct EvaluationContext
{
	//uci required parameters and flags
	bool goFlag, stopFlag, quitFlag, debugFlag;
	bool ponderFlag, mateSearchFlag, infiniteFlag;
	int moveTime, wTime, bTime, wInc, bInc, movesToGo, maxDepth, maxNodes;

	//custom parameters
	PlyType depthWalkValue;//initial search depth when deep searching
	PlyType quiescenceSearchDepth;
	PlyType searchDepth;
	uc cores;//number of calculation threads to use
	uc keepNStacks;//keep best N lines

	// int nodesReached;

	UpdateableData<int> depth;
	UpdateableData<int> selectiveDepth;
	int nodesReached;
	int timeSpent;//milliseconds
	std::vector<UpdateableData<PV_Data>> pvLines;


	// std::vector<std::vector<Move>> bestMoveStacks;

	std::vector<Move> activeMoves;
	std::vector<HashType> activeHashes;//3 move repetition tracking

	EvaluationContext();

	void resetGoFlags();
};
