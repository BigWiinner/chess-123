#pragma once

#include "Game.h"
#include "Grid.h"
#include "Bitboard.h"

constexpr int pieceSize = 80;
constexpr int WHITE = +1;
constexpr int BLACK = -1;

enum PieceBoard
{
    WHITE_PAWNS,
    WHITE_ROOKS,
    WHITE_BISHOPS,
    WHITE_KNIGHTS,
    WHITE_QUEENS,
    WHITE_KING,
    WHITE_ALL,
    BLACK_PAWNS,
    BLACK_ROOKS,
    BLACK_BISHOPS,
    BLACK_KNIGHTS,
    BLACK_QUEENS,
    BLACK_KING,
    BLACK_ALL,
    OCCUPIED,
    EMPTY,
    e_numBitboards
};

class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;

    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    void bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;

    void stopGame() override;

    Player *checkForWinner() override;
    bool checkForDraw() override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;

    Grid* getGrid() override { return _grid; }

    // AI methods
    void updateAI() override;
    bool gameHasAI() override { return true; }

private:
    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    int ownerColorAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;

    BitboardElement generateKnightBitboardMoves(int square);
    void generateKnightMoves(std::vector<BitMove>& moves, BitboardElement knights, BitboardElement unoccupied);
    void generateBishopMoves(std::vector<BitMove>& moves, BitboardElement bishops, BitboardElement unoccupied, BitboardElement friendlies);
    void generateRookMoves(std::vector<BitMove>& moves, BitboardElement bishops, BitboardElement unoccupied, BitboardElement friendlies);
    void generateQueenMoves(std::vector<BitMove>& moves, BitboardElement bishops, BitboardElement unoccupied, BitboardElement friendlies);
    BitboardElement generateKingBitboardMoves(int square);
    void generateKingMoves(std::vector<BitMove>& moves, BitboardElement king, BitboardElement unoccupied);
    void generatePawnMoves(std::vector<BitMove>& moves, BitboardElement pawns, BitboardElement emptySpaces, BitboardElement enemySpaces, char color);
    void addPawnMovesToList(std::vector<BitMove>& moves, BitboardElement bitboard, int shift);

    std::vector<BitMove> generateAllMoves();
    std::vector<BitMove> generateAllMoves(char* state, int player);

    int evaluate(std::string state);
    int negamax(char* state, int depth, int alpha, int beta, int player);

    Grid* _grid;
    std::vector<BitMove> _moves;
    int _currentPlayer;
    BitboardElement _bitboards[e_numBitboards];
    int _bitboardLookup[128];
    BitboardElement _knightBitboards[64];
    BitboardElement _kingBitboards[64];
};