#include "Chess.h"
#include "MagicBitboards.h"
#include <limits>
#include <cmath>

Chess::Chess()
{
    _grid = new Grid(8, 8);

    initMagicBitboards();

    for (int i = 0; i < 64; i++) {
        _knightBitboards[i] = generateKnightBitboardMoves(i);
        _kingBitboards[i] = generateKingBitboardMoves(i);
    }

    for (int i = 0; i < 128; i++) {
        _bitboardLookup[i] = 0;
    }

    _bitboardLookup['P'] = WHITE_PAWNS;
    _bitboardLookup['N'] = WHITE_KNIGHTS;
    _bitboardLookup['R'] = WHITE_ROOKS;
    _bitboardLookup['B'] = WHITE_BISHOPS;
    _bitboardLookup['Q'] = WHITE_QUEENS;
    _bitboardLookup['K'] = WHITE_KING;
    _bitboardLookup['p'] = BLACK_PAWNS;
    _bitboardLookup['n'] = BLACK_KNIGHTS;
    _bitboardLookup['r'] = BLACK_ROOKS;
    _bitboardLookup['b'] = BLACK_BISHOPS;
    _bitboardLookup['q'] = BLACK_QUEENS;
    _bitboardLookup['k'] = BLACK_KING;
    _bitboardLookup['0'] = EMPTY;
}

Chess::~Chess()
{
    cleanupMagicBitboards();
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
    //FENtoBoard("rnbqkbnr/8/8/8/8/8/8/RNBQKBNR");
    _moves = generateAllMoves();

    if (gameHasAI()) {
        setAIPlayer(AI_PLAYER);
    }

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

int Chess::ownerColorAt(int x, int y) const {
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return 0;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return 0;
    }
    int owner = square->bit()->getOwner()->playerNumber();

    if (owner == 1) {
        return BLACK;
    }
    return WHITE;
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
    return s;
}

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

BitboardElement Chess::generateKnightBitboardMoves(int square) {
    BitboardElement bitboard;

    std::pair<int,int> knightMoves[] = { { 2, 1 }, { 2, -1 }, { 1, 2 }, { -1, 2 }, { -2, 1 }, { -2, -1}, { 1, -2 }, { -1, -2 }};

    int rank = square / 8;
    int file = square % 8;

    constexpr uint64_t oneBit = 1;
    for(auto [dr, df] : knightMoves) {
        int r = rank + dr, f = file + df;
        int squareColor = ownerColorAt(f, r);
        if (r >= 0 && r < 8 && f >= 0 && f < 8 && squareColor != _currentPlayer) {
             bitboard |= oneBit << (r * 8 + f);   
        }
    }
    
    return bitboard;
}

void Chess::generateKnightMoves(std::vector<BitMove>& moves, BitboardElement knights, BitboardElement unoccupied) {
    if (knights.getData() == 0) {
        return;
    }

    knights.forEachBit([&] (int fromSquare) {
        BitboardElement canMoveTo = _knightBitboards[fromSquare].getData() & unoccupied.getData();
        canMoveTo.forEachBit([&] (int toSquare) {
            moves.emplace_back(fromSquare, toSquare, Knight);
        });
    });
}

BitboardElement Chess::generateKingBitboardMoves(int square) {
    BitboardElement bitboard;

    std::pair<int,int> kingMoves[] = { { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 }, { 0, -1 }, { 1, -1 } };

    int rank = square / 8;
    int file = square % 8;

    constexpr uint64_t oneBit = 1;
    for(auto [dr, df] : kingMoves) {
        int r = rank + dr, f = file + df;
        int squareColor = ownerColorAt(f, r);
        if (r >= 0 && r < 8 && f >= 0 && f < 8 && squareColor != _currentPlayer) {
             bitboard |= oneBit << (r * 8 + f);   
        }
    }
    
    return bitboard;
}

void Chess::generateKingMoves(std::vector<BitMove>& moves, BitboardElement king, BitboardElement unoccupied) {
    king.forEachBit([&] (int fromSquare) {
        BitboardElement canMoveTo = _kingBitboards[fromSquare].getData() & unoccupied.getData();
        canMoveTo.forEachBit([&] (int toSquare) {
            moves.emplace_back(fromSquare, toSquare, Knight);
        });
    });
}

void Chess::addPawnMovesToList(std::vector<BitMove>& moves, const BitboardElement bitboard, const int shift) {
    if (bitboard.getData() == 0) {
        return;
    }

    bitboard.forEachBit([&] (int toSquare) {
        int fromSquare = toSquare - shift;
        moves.emplace_back(fromSquare, toSquare, Pawn);
    });
}

void Chess::generatePawnMoves(std::vector<BitMove>& moves, const BitboardElement pawns, const BitboardElement emptySpaces, const BitboardElement enemySpaces, char color) {
    if (pawns.getData() == 0) {
        return;
    }

    constexpr uint64_t NotAFile = 0xFEFEFEFEFEFEFEFEULL;
    constexpr uint64_t NotHFile = 0x7F7F7F7F7F7F7F7FULL;
    constexpr uint64_t Rank3 = 0x0000000000FF0000ULL;
    constexpr uint64_t Rank6 = 0x0000FF0000000000ULL;

    BitboardElement singleMove = (color == WHITE) ? (pawns.getData() << 8) & emptySpaces.getData() : (pawns.getData() >> 8) & emptySpaces.getData();
    BitboardElement doubleMove = (color == WHITE) ? ((singleMove.getData() & Rank3) << 8) & emptySpaces.getData() : ((singleMove.getData() & Rank6) >> 8) & emptySpaces.getData();

    BitboardElement captureLeft = (color == WHITE) ? ((pawns.getData() & NotAFile) << 7) & enemySpaces.getData() : ((pawns.getData() & NotAFile) >> 9) & enemySpaces.getData();
    BitboardElement captureRight = (color == WHITE) ? ((pawns.getData() & NotHFile) << 9) & enemySpaces.getData() : ((pawns.getData() & NotHFile) >> 7) & enemySpaces.getData();
    
    int singleShift = color == WHITE ? 8 : -8;
    int doubleShift = color == WHITE ? 16 : -16;
    
    int captureLeftShift = color == WHITE ? 7 : -9;
    int captureRightShift = color == WHITE ? 9 : -7;

    addPawnMovesToList(moves, singleMove, singleShift);
    addPawnMovesToList(moves, doubleMove, doubleShift);
    
    addPawnMovesToList(moves, captureLeft, captureLeftShift);
    addPawnMovesToList(moves, captureRight, captureRightShift);
}

void Chess::generateRookMoves(std::vector<BitMove>& moves, BitboardElement rooks, BitboardElement occupied, BitboardElement friendlies) {
    if (rooks.getData() == 0) {
        return;
    }

    rooks.forEachBit([&] (int fromSquare) {
        BitboardElement attacks = BitboardElement(getRookAttacks(fromSquare, occupied.getData()) & ~friendlies.getData());
        attacks.forEachBit([&] (int toSquare) {
            moves.emplace_back(fromSquare, toSquare, Rook);
        });
    });
}

void Chess::generateBishopMoves(std::vector<BitMove>& moves, BitboardElement bishops, BitboardElement occupied, BitboardElement friendlies) {
    if (bishops.getData() == 0) {
        return;
    }

    bishops.forEachBit([&] (int fromSquare) {
        BitboardElement attacks = BitboardElement(getBishopAttacks(fromSquare, occupied.getData()) & ~friendlies.getData());
        attacks.forEachBit([&] (int toSquare) {
            moves.emplace_back(fromSquare, toSquare, Bishop);
        });
    });
}

void Chess::generateQueenMoves(std::vector<BitMove>& moves, BitboardElement queens, BitboardElement occupied, BitboardElement friendlies) {
    if (queens.getData() == 0) {
        return;
    }

    queens.forEachBit([&] (int fromSquare) {
        BitboardElement bishopAttacks = BitboardElement(getBishopAttacks(fromSquare, occupied.getData()) & ~friendlies.getData());
        bishopAttacks.forEachBit([&] (int toSquare) {
            moves.emplace_back(fromSquare, toSquare, Queen);
        });
        BitboardElement rookAttacks = BitboardElement(getRookAttacks(fromSquare, occupied.getData()) & ~friendlies.getData());
        rookAttacks.forEachBit([&] (int toSquare) {
            moves.emplace_back(fromSquare, toSquare, Queen);
        });
    });
}

std::vector<BitMove> Chess::generateAllMoves() {
    // PERSONAL REMINDERS:
    // ULL stands for unsigned long long, from my understanding its size is 64 bits and cannot be negative
    // << is bit shift left, >> bit shift right (the last bit is dropped)

    std::vector<BitMove> moves;
    moves.reserve(64);

    std::string state = stateString();

    for (int i = 0; i < e_numBitboards; i++) {
        _bitboards[i] = 0;
    }

    for (int i = 0; i < 64; i++) {
        int bitIndex = _bitboardLookup[state[i]];
        _bitboards[bitIndex] |= 1ULL << i;
        if (state[i] != '0') {
            _bitboards[OCCUPIED] |= 1ULL << i;
            _bitboards[isupper(state[i]) ? WHITE_ALL : BLACK_ALL] |= 1ULL << i;
        }
    }

    int bitIndex = _currentPlayer == WHITE ? WHITE_PAWNS : BLACK_PAWNS;
    int allyPieces = _currentPlayer == WHITE ? WHITE_ALL : BLACK_ALL;
    int enemyPieces = _currentPlayer == WHITE ? BLACK_ALL : WHITE_ALL;
    generateKnightMoves(moves, _bitboards[WHITE_KNIGHTS + bitIndex], ~_bitboards[allyPieces].getData());
    generateBishopMoves(moves, _bitboards[WHITE_BISHOPS + bitIndex], _bitboards[OCCUPIED], _bitboards[allyPieces].getData());
    generateRookMoves(moves, _bitboards[WHITE_ROOKS + bitIndex], _bitboards[OCCUPIED], _bitboards[allyPieces].getData());
    generateQueenMoves(moves, _bitboards[WHITE_QUEENS + bitIndex], _bitboards[OCCUPIED], _bitboards[allyPieces].getData());
    generateKingMoves(moves, _bitboards[WHITE_KING + bitIndex], ~_bitboards[allyPieces].getData());
    generatePawnMoves(moves, _bitboards[WHITE_PAWNS + bitIndex], _bitboards[EMPTY], _bitboards[enemyPieces], _currentPlayer);

    return moves;
}

std::vector<BitMove> Chess::generateAllMoves(char* state, int player) {
    std::vector<BitMove> moves;
    moves.reserve(64);

    for (int i = 0; i < e_numBitboards; i++) {
        _bitboards[i] = 0;
    }

    for (int i = 0; i < 64; i++) {
        int bitIndex = _bitboardLookup[state[i]];
        _bitboards[bitIndex] |= 1ULL << i;
        if (state[i] != '0') {
            _bitboards[OCCUPIED] |= 1ULL << i;
            _bitboards[isupper(state[i]) ? WHITE_ALL : BLACK_ALL] |= 1ULL << i;
        }
    }

    int bitIndex = player == WHITE ? WHITE_PAWNS : BLACK_PAWNS;
    int allyPieces = player == WHITE ? WHITE_ALL : BLACK_ALL;
    int enemyPieces = player == WHITE ? BLACK_ALL : WHITE_ALL;
    generateKnightMoves(moves, _bitboards[WHITE_KNIGHTS + bitIndex], ~_bitboards[allyPieces].getData());
    generateBishopMoves(moves, _bitboards[WHITE_BISHOPS + bitIndex], _bitboards[OCCUPIED], _bitboards[allyPieces].getData());
    generateRookMoves(moves, _bitboards[WHITE_ROOKS + bitIndex], _bitboards[OCCUPIED], _bitboards[allyPieces].getData());
    generateQueenMoves(moves, _bitboards[WHITE_QUEENS + bitIndex], _bitboards[OCCUPIED], _bitboards[allyPieces].getData());
    generateKingMoves(moves, _bitboards[WHITE_KING + bitIndex], ~_bitboards[allyPieces].getData());
    generatePawnMoves(moves, _bitboards[WHITE_PAWNS + bitIndex], _bitboards[EMPTY], _bitboards[enemyPieces], _currentPlayer);

    return moves;
}

const std::map<char, int> pieceEval = {
    {'0', 0},
    {'P', 100}, {'N', 300}, {'B', 300}, {'R', 500}, {'Q', 1000}, {'K', 2000},
    {'p', -100}, {'n', -300}, {'b', -300}, {'r', -500}, {'q', -1000}, {'k', -2000}
}; 

int Chess::evaluate(const std::string state) {
    int value = 0;
    for (char c : state) {
        value += pieceEval.at(c);
    }
    return value;
}

int Chess::negamax(char* state, int depth, int alpha, int beta, int player) {
    if (depth == 0) {
        int score = evaluate(state);
        return player * score;
    }

    int bestScore = -999999999;
    std::vector<BitMove> negaMoves = generateAllMoves(state, player == 1 ? WHITE : BLACK);
    for (auto move : negaMoves) {
        int srcSquare = move.from;
        int dstSquare = move.to;

        char temp = state[dstSquare];


        state[dstSquare] = state[srcSquare];
        state[srcSquare] = '0';

        int score = -negamax(state, depth - 1, -beta, -alpha, -player);

        state[srcSquare] = state[dstSquare];
        state[dstSquare] = temp;

        if (score > bestScore) {
            bestScore = score;
        }
        if (score > alpha) {
            alpha = score;
        }
        if (alpha >= beta) break;
    }




    return bestScore;
}

void Chess::updateAI() {
    if (!gameHasAI()) return;
    if (_moves.size() == 0) return; // No moves remain

    constexpr int infinity = 999999999;
    int bestScore = -infinity;
    BitMove bestMove;
    std::string initState = stateString();
    char startState[65];

    int depth = 4;

    for (auto move : _moves) {
        strcpy(&startState[0], initState.c_str());

        int srcSquare = move.from;
        int dstSquare = move.to;


        startState[dstSquare] = startState[srcSquare];
        startState[srcSquare] = '0';

        int score = -negamax(startState, depth, -infinity, infinity, 1);

        startState[srcSquare] = startState[dstSquare];
        startState[dstSquare] = '0';

        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
    }

    int srcSquare = bestMove.from;
    int dstSquare = bestMove.to;
    // srcSquare and dstSquare are a number between 0 and 63 representing which spot on the board they are
    //  but, does not specify its rank and file (x,y)
    // square&7 and square/8 get the proper x,y coordinates
    BitHolder& src = getHolderAt(srcSquare&7, srcSquare/8);
    BitHolder& dst = getHolderAt(dstSquare&7, dstSquare/8);
    Bit* bit = src.bit();
    dst.dropBitAtPoint(bit, ImVec2(0,0)); // As defined in ChessSquare.cpp
    src.setBit(nullptr);
    bitMovedFromTo(*bit, src, dst);
}