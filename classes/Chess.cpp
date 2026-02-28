#include "Chess.h"
#include <limits>
#include <cmath>

Chess::Chess()
{
    _grid = new Grid(8, 8);
}

Chess::~Chess()
{
    delete _grid;
}

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()-128];
    }
    return notation;
}

Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();
    // should possibly be cached from player class?
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;
    _currentPlayer = WHITE;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

    _moves = generateAllMoves();
    startGame();
}

void Chess::FENtoBoard(const std::string& fen) {
    // convert a FEN string to a board
    // FEN is a space delimited string with 6 fields
    // 1: piece placement (from white's perspective)
    // NOT PART OF THIS ASSIGNMENT BUT OTHER THINGS THAT CAN BE IN A FEN STRING
    // ARE BELOW
    // 2: active color (W or B)
    // 3: castling availability (KQkq or -)
    // 4: en passant target square (in algebraic notation, or -)
    // 5: halfmove clock (number of halfmoves since the last capture or pawn advance)

    _grid->forEachSquare([] (ChessSquare* square, int x, int y) {
        square->setBit(nullptr);
    });

    // Go through row first, then column
    // REMINDERS:
    //  - lowercase is black
    //  - uppercase is white
    //  - numbers are number of empty spaces between pieces
    //  - / chars denote moving to the next row
    //  - FEN strings are made with the assumption that white started at the bottom of the board

    // ALSO for column jumping:
    //  https://www.reddit.com/r/Cplusplus/comments/x89lj8/understanding_the_code_hack_of_adding_0_to_char/

    int row = 7;
    int col = 0;
    for (char c : fen) {
        
        // For now, change later:
        if (c == ' ') {
            return;
        }


        if (c == '/') {
            row--;
            col = 0;
        }
        else if (isdigit(c)) {
            col += c - '0';
        }
        else {
            ChessPiece piece = Pawn;
            char upperC = toupper(c);
            if (upperC == 'R') {
                piece = Rook;
            }
            else if (upperC == 'N') {
                piece = Knight;
            }
            else if (upperC == 'B') {
                piece = Bishop;
            }
            else if (upperC == 'Q') {
                piece = Queen;
            }
            else if (upperC == 'K') {
                piece = King;
            }

            Bit* bit = PieceForPlayer(isupper(c) ? 0 : 1, piece);
            ChessSquare* square = _grid->getSquare(col, row);
            bit->setPosition(square->getPosition());
            square->setBit(bit);
            bit->setGameTag(isupper(c) ? piece : (piece + 128));
            col++;
        }
    }
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    // need to implement friendly/unfriendly in bit so for now this hack
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor != currentPlayer) return false;
    ChessSquare* square = (ChessSquare *)& src;
    int squareIndex = square->getSquareIndex();

    for (auto move : _moves) {
        if (move.from == squareIndex) {
            return true;
        }
    }

    return false;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessSquare* squareSrc = (ChessSquare*)&src;
    ChessSquare* squareDst = (ChessSquare*)&dst;

    int squareIndexSrc = squareSrc->getSquareIndex();
    int squareIndexDst = squareDst->getSquareIndex();

    for (auto move : _moves) {
        if (move.from == squareIndexSrc && move.to == squareIndexDst) {
            return true;
        }
    }
    return false;
}

void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) {
    _currentPlayer = (_currentPlayer == WHITE ? BLACK : WHITE);
    _moves = generateAllMoves();
    clearBoardHighlights();
    endTurn();
}

void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

Player* Chess::ownerAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return nullptr;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return nullptr;
    }
    return square->bit()->getOwner();
}

Player* Chess::checkForWinner()
{
    return nullptr;
}

bool Chess::checkForDraw()
{
    return false;
}

std::string Chess::initialStateString()
{
    return stateString();
}

std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
            s += pieceNotation( x, y );
        }
    );
    return s;}

void Chess::setStateString(const std::string &s)
{
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, Pawn));
        } else {
            square->setBit(nullptr);
        }
    });
}

void Chess::generateKnightMoves(std::vector<BitMove>& moves, std::string& state) {
    char knightPiece = _currentPlayer == WHITE ? 'N' : 'n';

    std::pair<int,int> knightMoves[] = { { 2, 1 }, { 2, -1 }, { 1, 2 }, { -1, 2 }, { -2, 1 }, { -2, -1}, { 1, -2 }, { -1, -2 }};

    int index = 0;
    for (char square : state) {
        if (square == knightPiece) {
            int rank = index / 8;
            int file = index % 8;

            for(auto [dr, df] : knightMoves) {
                int r = rank + dr, f = file + df;
                if (r >= 0 && r < 8 && f >= 0 && f < 8 ) {
                    moves.emplace_back(index, r*8+f, Knight);
                }
            }
        }
        index++;
    }
}

std::vector<BitMove> Chess::generateAllMoves() {
    std::vector<BitMove> moves;
    moves.reserve(32);

    std::string state = stateString();
    std::cout << state << std::endl;

    generateKnightMoves(moves, state);

    return moves;
}
