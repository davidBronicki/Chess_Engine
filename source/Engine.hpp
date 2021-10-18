#pragma once

#include <string>
#include <regex>
#include <memory>
#include <thread>

#include "Board.hpp"
#include "EvalTypes.hpp"

std::vector<std::string> tokenize(
	const std::string& str,
	const std::regex& re);

uc algebraicToIndex(std::string square);
std::string indexToAlgebraic(uc index);
std::string moveToAlgebraic(Move move);

struct HashBoard
{
	ull hash;
	Value eval;
	Move bestResponse;

	short totalPlyDepth;
	int uses;
	short lastUseSearch;

	enum CutType : uc
	{
		leaf,
		alphaCut,
		betaCut
	} cutType;
};

class HashTable
{
	HashBoard* table;
	size_t size;
	size_t occupancy;
	public:
	HashTable(size_t tableSize)
	:
		table{new HashBoard[tableSize]{}},
		size(tableSize)
	{}
	~HashTable(){delete[] table;}

	HashTable(HashTable const&) = delete;
	HashTable& operator=(HashTable const&) = delete;

	HashBoard const& get(ull hash) const {return table[hash % size];}
	void set(HashBoard board) {
		HashBoard& slot = table[board.hash % size];
		if (slot.hash == 0)
			occupancy++;
		slot = board;
	}

	size_t getSize() {return size;}
	size_t getOccupancy() {return occupancy;}
};

// class MoveStack
// {
// 	std::vector<Move> moves;
// 	// HashBoard currentEval;
// 	public:
// 	MoveStack(){};
// 	void push(Move move){moves.push_back(move);}
// 	Move pop(){Move output = moves.back(); moves.pop_back(); return output;}
// 	Move const& top() const{return moves.back();}

// 	Move& operator[](size_t i){return moves[i];}
// 	size_t size(){return moves.size()}
// };

class Engine
{
	static bool globalsReady;

	static void calculationLoop(Engine* engine);

	static std::tuple<Value, std::vector<Move>> quiescenceSearch(Engine* engine,
		Value alpha, Value beta, short searchDepth);

	static std::tuple<Value, std::vector<Move>> nonQuiescenceSearch(Engine* engine,
		Value alpha, Value beta, short searchDepth);

	public:
	static void initializeGlobals();

	private:
	
	//uci required parameters and flags
	bool goFlag, stopFlag, quitFlag, debugFlag;
	bool ponderFlag, mateSearchFlag, infiniteFlag;
	int moveTime, wTime, bTime, wInc, bInc, movesToGo, maxDepth, maxNodes;

	inline bool good(){return !stopFlag && !quitFlag;}

	//custom parameters
	short keepNStacks;//keep best N lines
	short depthWalkValue;//initial search depth when deep searching
	short quiescenceSearchDepth;
	short cores;//number of calculation threads to use

	//state variables
	std::shared_ptr<Board> board;
	HashTable hashTable;
	short searchIter;

	std::vector<std::vector<Move>> bestMoveStacks;

	std::vector<Move> activeMoves;
	std::vector<Hash> activeHashes;//3 move repetition tracking

	std::thread calculationThread;

	//io variables
	std::regex interfaceParsingTokens;
	std::regex fenParsingTokens;
	void handleString(std::string inputLine);

	//parameters set when io calls go. need to be reset when stopping
	void resetGoFlags();
	//need to work with a new move stack
	void resetIntermediateValues();

	void initPos();
	void initPos(std::string positionString);
	Move buildMoveFromFragments(uc startIndex, uc endIndex, char promotion);

	bool threeMoveRepetition();
	bool advance(Move move);
	char nonQuiescentAdvance(Move move);
	void back();

	public:

	Engine();
	void run();
};
