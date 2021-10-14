#pragma once

typedef unsigned char uc;
typedef unsigned short us;
typedef unsigned long ul;
typedef unsigned long long ull;


namespace Piece
{
	enum names : uc{
	White	 =	0b0000,
	Black	 =	0b0001,
	Pawn	 =	0b0010,
	Knight	 =	0b0100,
	King	 =	0b0110,
	Rook	 =	0b1010,
	Bishop	 =	0b1100,
	Queen	 =	0b1110,

	Team	 =	0b0001,
	Sliding	 =	0b1000,
	Occupied =	0b1110,
	IndexAll =	0b1000,
	IndexNone=	0b1001};
}

namespace Extra
{
	enum names : uc{
	White_Short			=	0b00010000,
	White_King			=	0b00010000,

	White_Long			=	0b00100000,
	White_Queen			=	0b00100000,

	Black_Short			=	0b01000000,
	Black_King			=	0b01000000,

	Black_Long			=	0b10000000,
	Black_Queen			=	0b10000000,

	CastleInfo			=	0b11110000,

	EnPassantAvailable	=	0b00001000,
	EnPassantFile		=	0b00000111,
	EnPassantInfo		=	0b00001111};
}

namespace MoveType
{
	enum names : uc{
	Normal,
	EnPassant,
	PromoQueen,
	PromoRook,
	PromoBishop,
	PromoKnight,
	WhiteShort,
	BlackShort,
	WhiteLong,
	BlackLong,
	NullMove};
}
