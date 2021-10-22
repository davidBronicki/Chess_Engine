#pragma once

#include "Value.hpp"
#include "BitDefs.hpp"

#include <string>
#include <regex>
#include <memory>
#include <thread>

// #include "HashTable.hpp"

std::vector<std::string> tokenize(
	const std::string& str,
	const std::regex& re);

BoardSquare algebraicToIndex(std::string square);
std::string indexToAlgebraic(BoardSquare index);
std::string moveToAlgebraic(Move move);

struct HashTable;
struct EvaluationContext;
struct Board;

class Engine
{
	static bool globalsReady;

	static void calculationLoop(Engine* engine);

	static void initializeGlobals();

	bool good();

	//state variables
	std::unique_ptr<Board> board;
	std::unique_ptr<HashTable> hashTable;
	std::unique_ptr<EvaluationContext> context;

	std::thread calculationThread;

	//io variables
	std::regex interfaceParsingTokens;
	std::regex fenParsingTokens;
	void handleString(std::string inputLine);

	//parameters set when io calls go. need to be reset when stopping
	// void resetGoFlags();
	//need to work with a new move stack
	// void resetIntermediateValues();

	void initPos();
	void initPos(std::string positionString);
	Move buildMoveFromFragments(BoardSquare startIndex, BoardSquare endIndex, char promotion);

	bool threeMoveRepetition();
	bool advance(Move move);
	// uc nonQuiescentAdvance(Move move);
	void back();
	Value hashEval(Move move);

	Value quiescenceSearch(
		Value alpha, Value beta, PlyType searchDepth, PlyType rootPly);

	Value mainSearch(
		Value alpha, Value beta, PlyType searchDepth, PlyType rootPly);

	std::vector<std::tuple<Value, Move>> rootSearch(PlyType searchDepth);

	public:

	Engine();
	~Engine();//forces destructer to not be inline so that it happens when unique pointed objects are defined.
	void run();
};
