#pragma once

#include "Value.hpp"

struct HashBoard
{
	ull hash;
	Value value;
	Move bestResponse;

	PlyType searchDepth;
	PlyType rootPly;

	enum NodeType : uc
	{
		PrincipleVariation,
		AllNode,
		CutNode,
		Quiescence_PV,
		Quiescence_All,
		Quiescence_Cut,
		Quiescence_Mask = Quiescence_PV,
		TypeInfo_Mask = CutNode
	} nodeType;
};

class HashTable
{
	HashBoard* table;
	size_t size;
	size_t occupancy;
	size_t mask;
	public:
	
	enum HashOccupancyType : uc
	{
		HashNotPresent = 0,
		HashesNotEqual = 1,
		HashesEqual = 2
	};
	
	HashTable(size_t tableSize)
	:
		table{new HashBoard[1ull << tableSize]{}},
		size(1ull << tableSize),
		mask((1ull << tableSize) - 1)
	{}
	~HashTable(){delete[] table;}

	HashTable(HashTable const&) = delete;
	HashTable& operator=(HashTable const&) = delete;

	HashBoard const& get(HashType hash) const {return table[hash & mask];}
	void set(HashBoard&& board) {//TODO: optimize to move instead of copy? build in place?
		HashBoard& slot = table[board.hash & mask];
		if (slot.hash == 0)
			occupancy++;
		slot = board;
	}

	HashBoard const& nonQuiescence_HandleHash(HashType const& hash,
		HashOccupancyType& existence, Value& alpha, Value& beta,
		std::vector<Move>& moves, PlyType searchDepth, PlyType rootPly) const;

	HashBoard const& quiescence_HandleHash(HashType const& hash,
		HashOccupancyType& existence, Value& alpha, Value& beta,
		std::vector<Move>& moves, PlyType searchDepth, PlyType rootPly) const;

	size_t getSize() const {return size;}
	size_t getOccupancy() const {return occupancy;}

	void reset();
};