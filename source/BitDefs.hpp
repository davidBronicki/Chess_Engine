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

	Team	 =	0b0001,//mask to get team
	Sliding	 =	0b1000,//mask to check if slidy
	Occupied =	0b1110,//mask to see if square is occupied

	IndexAll =	0b1000,//index labelling the "all pieces" bit board
	IndexNone=	0b1001};//index labelling the "no pieces" bit board
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

namespace
{
	ull leftMoveMask[8]	=
	{
		0xffffffffffffffffull,
		0x7f7f7f7f7f7f7f7full,
		0x3f3f3f3f3f3f3f3full,
		0x1f1f1f1f1f1f1f1full,
		0x0f0f0f0f0f0f0f0full,
		0x0707070707070707ull,
		0x0303030303030303ull,
		0x0101010101010101ull
	};
	ull rightMoveMask[8] =
	{
		0xffffffffffffffffull,
		0xfefefefefefefefeull,
		0xfcfcfcfcfcfcfcfcull,
		0xf8f8f8f8f8f8f8f8ull,
		0xf0f0f0f0f0f0f0f0ull,
		0xe0e0e0e0e0e0e0e0ull,
		0xc0c0c0c0c0c0c0c0ull,
		0x8080808080808080ull
	};
}
