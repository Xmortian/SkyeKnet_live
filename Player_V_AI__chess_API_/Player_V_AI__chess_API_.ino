/*
 * Chess Board - 1 vs AI Mode
 * Full Integration with chess-api.com
 * 
 * Player = White
 * AI = Black
 */
 
// if lichess not work; Need to implement difficulty matrix, either through website or other methods
// FEN breaks when castling ( works now )
// FEN breaks when es passant present  ( works now )
// Rook does not move when castle with AI BLACK, only king moves two squares, possible error in move generation (Fixed but need checking)
// Need to add AI confirm button ( At very last)
// Add winchance ( .winChance) and show on LED
// Need to fix AI wrong move, detection works for now, but Re move does not.
// Need to check Pawn promotion FEN
// 
#define NUM_ROWS 8
#define NUM_COLS 8
#define LED_COUNT 64
#define BRIGHTNESS 100
#define LED_PIN 19
#define BUZZER_PIN 18 
#define WOKWI_TEST_MODE true

#define BTN_P1 35 
#define BTN_P2 10 

// --- AI Configuration ---
bool isAIPlaying = true; // TRUE for AI mode

#include <math.h>
#include <FastLED.h>
#include <TM1637Display.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// WiFi Credentials (Wokwi)
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* api_url = "https://chess-api.com/v1";

const int MUX_S0 = 4;
const int MUX_S1 = 5;
const int MUX_S2 = 6;
const int MUX_S3 = 7;
const int MUX_EN = 14;

const int MUX1_SIG = 15;
const int MUX2_SIG = 16;
const int MUX3_SIG = 17;
const int MUX4_SIG = 18;

byte pawn[8] = {
  B00000, B00100, B01110, B00100, B00100, B01110, B11111, B00000
};

const int sigPins[4] = {MUX1_SIG, MUX2_SIG, MUX3_SIG, MUX4_SIG};

TM1637Display display1(21, 20);
TM1637Display display2(39, 38);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Clock Variables
int p1Time = 300, p2Time = 300;
bool p1Turn = true;
bool gameStarted = false;
unsigned long lastTick = 0;

// Display Update Variables
unsigned long lastDisplayUpdate = 0;
const int displayUpdateInterval = 3000;
int displayState = 0;

byte rowPatterns[8] = {
  0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
};

bool moveConfirmedByButton = false;

// LED Colors
CRGB leds[LED_COUNT];
const CRGB PROMOTION_COLOR = CRGB(255, 215, 0);
const CRGB MOVE_COLOR = CRGB(0, 255, 0);
const CRGB CAPTURE_COLOR = CRGB(255, 0, 0);
const CRGB ORIGIN_COLOR = CRGB(100, 100, 255);
const CRGB AI_FROM_COLOR = CRGB(255, 255, 0);  // Yellow - AI wants to move from
const CRGB AI_TO_COLOR = CRGB(0, 255, 255);    // Cyan - AI wants to move to

bool sensorState[8][8];
bool sensorPrev[8][8];

char initialBoard[8][8] = {
  {'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R'},
  {'P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'},
  {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
  {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
  {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
  {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
  {'p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'},
  {'r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'}
};

char board[8][8];

// AI Move State
String aiMoveFrom = "";
String aiMoveTo = "";
bool waitingForAIMove = false;
bool aiMoveInProgress = false;

const char* aiGreetings[] = {
  "Hello! I am SkyeKnet! Your personal ChessCare Companion.(๑>ᴗ<๑)",
  "FINALLY A WORTHY OPPONENT, OUR BATTLE WILL BE LEGENDARY.(˵•̀ᴗ•́˵)"
};

const char* aiRandomLines[] = {
  "My dream is to dethrone Stockfish someday (ง •̀_•́)ง",
  "Castle all you want! Your king is mine to take! ᕙ(⇀‸↼‶)ᕗ",
  "Sacrifice the kinggggggggggggg! ୧(๑•̀ᗝ•́)૭",
  "Let me cook! ψ(｀∇´)ψ",
  "My circuits are throbbing, THROBBING!! (ง ‵□′)ง",
  "Is that your best move? How cute. (¬‿¬)",
  "Calculated. 10,000 moves ahead... ( •̀_•́ )✧",
  "Error 404: Your defense not found. ┐(︶▽︶)┌"
};

// Game state tracking
bool whiteTurn = true;
bool gameOver = false;
char winner = ' ';

// Castling rights
bool whiteKingMoved = false;
bool whiteRookKingsideMoved = false;
bool whiteRookQueensideMoved = false;
bool blackKingMoved = false;
bool blackRookKingsideMoved = false;
bool blackRookQueensideMoved = false;

// En passant tracking
int enPassantCol = -1;
int enPassantRow = -1;

// Move history for FEN generation
String moveHistory = "";

// Function Prototypes
void readSensors();
void fireworkAnimation();
void startGameAnimation();
void captureAnimation();
void promotionAnimation(int col);
void checkForPromotion(int targetRow, int targetCol, char piece);
void getPossibleMoves(int row, int col, int &moveCount, int moves[][2]);
int getPixelIndex(int row, int col);
void handleSerialCommands();
void showLEDPattern();
bool isSquareAttacked(int row, int col, char byColor);
bool isKingInCheck(char kingColor);
bool isCheckmate(char kingColor);
bool wouldMoveLeaveKingInCheck(int fromRow, int fromCol, int toRow, int toCol);
void updateCastlingRights(int fromRow, int fromCol, char piece);
String generateFEN();
void getAIMove();
void showAIMove();
void executeAIMove();
void handlePlayerMove(int row, int col);
void updateClock();
void displayClockInfo();
void scrollCredits();
void playAIDialogue(const char* text, bool slowScroll);
void displayStaticInfo(const char* top, const char* bottom);
int countPieces(char color);
int calculateMaterial();

int getPixelIndex(int row, int col) {
  return col * NUM_COLS + (7 - row);
}

void showLEDPattern() {
  Serial.println("\n=== LED Display (Green=move, Red=capture, Blue=origin) ===");
  for (int row = 7; row >= 0; row--) {
    Serial.print(row + 1);
    Serial.print(" | ");
    for (int col = 0; col < 8; col++) {
      int idx = getPixelIndex(row, col);
      if (leds[idx].r > 100 && leds[idx].g < 50 && leds[idx].b < 50) {
        Serial.print("R "); // Red (capture)
      } else if (leds[idx].g > 100 && leds[idx].r < 50 && leds[idx].b < 50) {
        Serial.print("G "); // Green (move)
      } else if (leds[idx].b > 100 && leds[idx].r < 150 && leds[idx].g < 150) {
        Serial.print("B "); // Blue (origin)
      } else if (leds[idx].r > 0 || leds[idx].g > 0 || leds[idx].b > 0) {
        Serial.print("* "); // Some other color
      } else {
        Serial.print(". "); // Off
      }
    }
    Serial.println();
  }
  Serial.println("  +----------------");
  Serial.println("    a b c d e f g h\n");
}

// Generate FEN string from current board state
String generateFEN() {
  String fen = "";
  
  // 1. Board position
  for (int row = 7; row >= 0; row--) {
    int emptyCount = 0;
    for (int col = 0; col < 8; col++) {
      char piece = board[row][col];
      if (piece == ' ') {
        emptyCount++;
      } else {
        if (emptyCount > 0) {
          fen += String(emptyCount);
          emptyCount = 0;
        }
        fen += piece;
      }
    }
    if (emptyCount > 0) fen += String(emptyCount);
    if (row > 0) fen += "/";
  }
  
  // 2. Active color
  fen += whiteTurn ? " w " : " b ";
  
  // 3. Castling availability
  String castling = "";
  if (!whiteKingMoved) {
    if (!whiteRookKingsideMoved) castling += "K";
    if (!whiteRookQueensideMoved) castling += "Q";
  }
  if (!blackKingMoved) {
    if (!blackRookKingsideMoved) castling += "k";
    if (!blackRookQueensideMoved) castling += "q";
  }
  if (castling == "") castling = "-";
  fen += castling;
  
// 4. En passant target square
fen += " ";
if (enPassantCol != -1 && enPassantRow != -1) {
  bool epUsable = false;
  int leftCol  = enPassantCol - 1;
  int rightCol = enPassantCol + 1;

  if (whiteTurn) {
    // Black just double-pushed a pawn — check if a WHITE pawn sits beside it
    // The black pawn that just moved is at enPassantRow+1 (one past the ep square)
    int blackPawnRow = enPassantRow + 1;
    if (leftCol  >= 0 && board[blackPawnRow][leftCol]  == 'P') epUsable = true;
    if (rightCol <  8 && board[blackPawnRow][rightCol] == 'P') epUsable = true;
  } else {
    // White just double-pushed a pawn — check if a BLACK pawn sits beside it
    // The white pawn that just moved is at enPassantRow-1
    int whitePawnRow = enPassantRow - 1;
    if (leftCol  >= 0 && board[whitePawnRow][leftCol]  == 'p') epUsable = true;
    if (rightCol <  8 && board[whitePawnRow][rightCol] == 'p') epUsable = true;
  }

  if (epUsable) {
    fen += (char)('a' + enPassantCol);
    fen += String(enPassantRow + 1);
  } else {
    fen += "-";
  }
} else {
  fen += "-";
}
  
  // 5. Halfmove clock and 6. Fullmove number
  fen += " 0 1";
  
  return fen;
}

// Get AI move from chess-api.com
void getAIMove() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    displayStaticInfo("AI Error:", "No WiFi");
    return;
  }

  displayStaticInfo("SkyeKnet is", "Thinking...");

  HTTPClient http;
  http.begin(api_url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(10000);

  // Generate FEN FIRST, then build request
  String fen = generateFEN();
  Serial.println("Sending FEN to SkyeKnet: " + fen);

  StaticJsonDocument<256> requestDoc;
  requestDoc["fen"] = fen;

  String requestBody;
  serializeJson(requestDoc, requestBody);

  int httpResponseCode = http.POST(requestBody);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Raw API Response: " + response);

    DynamicJsonDocument responseDoc(2048);
    DeserializationError error = deserializeJson(responseDoc, response);

    if (!error) {
      JsonObject bestMove;

      if (responseDoc.is<JsonArray>()) {
        bestMove = responseDoc[0];
      } else if (responseDoc.is<JsonObject>()) {
        bestMove = responseDoc.as<JsonObject>();
      }

      if (!bestMove.isNull() && bestMove.containsKey("from")) {
        aiMoveFrom = bestMove["from"].as<String>();
        aiMoveTo = bestMove["to"].as<String>();

        Serial.print("Target Locked: ");
        Serial.print(aiMoveFrom);
        Serial.print(" -> ");
        Serial.println(aiMoveTo);

        showAIMove();
        waitingForAIMove = true;

        if (random(0, 3) == 1) {
          playAIDialogue(aiRandomLines[random(0, 8)], false);
        }
      } else {
        Serial.println("JSON structure unexpected or move not found.");
        displayStaticInfo("AI Error", "Move Missing");
      }
    } else {
      Serial.print("JSON Deserialization failed: ");
      Serial.println(error.c_str());
      displayStaticInfo("AI Error", "Parse Fail");
    }
  } else {
    Serial.print("HTTP Request failed. Error code: ");
    Serial.println(httpResponseCode);
    if (httpResponseCode == -1) displayStaticInfo("AI Error", "Timeout/Link");
    else displayStaticInfo("AI Error", "API Down");
  }

  http.end();
}

// Show AI's intended move on LEDs
void showAIMove() {
  if (aiMoveFrom.length() < 2 || aiMoveTo.length() < 2) return;
  
  int fromRow = aiMoveFrom[1] - '1';
  int fromCol = aiMoveFrom[0] - 'a';
  int toRow = aiMoveTo[1] - '1';
  int toCol = aiMoveTo[0] - 'a';
  
  FastLED.clear();
  leds[getPixelIndex(fromRow, fromCol)] = AI_FROM_COLOR;
  leds[getPixelIndex(toRow, toCol)] = AI_TO_COLOR;
  FastLED.show();
  
  char moveText[17];
  sprintf(moveText, "%s -> %s", aiMoveFrom.c_str(), aiMoveTo.c_str());
  displayStaticInfo("AI Move:", moveText);
}

// Execute AI move on the board
void executeAIMove() {
  if (aiMoveFrom.length() < 2 || aiMoveTo.length() < 2) return;
  
  int fromRow = aiMoveFrom[1] - '1';
  int fromCol = aiMoveFrom[0] - 'a';
  int toRow = aiMoveTo[1] - '1';
  int toCol = aiMoveTo[0] - 'a';
  
  char piece = board[fromRow][fromCol];
  
  // Check for capture
  if (board[toRow][toCol] != ' ') {
    Serial.println("AI captures!");
    captureAnimation();
  }
  
  // Move piece
  board[toRow][toCol] = piece;
  board[fromRow][fromCol] = ' ';

  if ((piece == 'K' || piece == 'k') && abs(toCol - fromCol) == 2) {
    int rookRow = (piece == 'K') ? 0 : 7;
    if (toCol == 6) { // Kingside
      board[rookRow][5] = board[rookRow][7];
      board[rookRow][7] = ' ';
      sensorState[rookRow][5] = true;  sensorPrev[rookRow][5] = true;
      sensorState[rookRow][7] = false; sensorPrev[rookRow][7] = false;
      Serial.println("AI castled kingside - rook moved h->f");
    } else { // Queenside
      board[rookRow][3] = board[rookRow][0];
      board[rookRow][0] = ' ';
      sensorState[rookRow][3] = true;  sensorPrev[rookRow][3] = true;
      sensorState[rookRow][0] = false; sensorPrev[rookRow][0] = false;
      Serial.println("AI castled queenside - rook moved a->d");
    }
  }

  
  // Update castling rights
  updateCastlingRights(fromRow, fromCol, piece); 
  
  // Check for promotion
  checkForPromotion(toRow, toCol, piece);
  
  // Update sensor states
  sensorPrev[fromRow][fromCol] = false;
  sensorPrev[toRow][toCol] = true;
  sensorState[fromRow][fromCol] = false;
  sensorState[toRow][toCol] = true;
  
  // Switch turns
  whiteTurn = !whiteTurn;
  p1Turn = !p1Turn;
  
  // Check for checkmate
  if (isKingInCheck('w')) {
    if (isCheckmate('w')) {
      Serial.println("\n*** CHECKMATE! AI WINS! ***");
      gameOver = true;
      winner = 'b';
      displayStaticInfo("CHECKMATE!", "AI Wins!");
      
      for (int i = 0; i < 5; i++) {
        fill_solid(leds, LED_COUNT, CRGB::Purple);
        FastLED.show();
        delay(300);
        FastLED.clear();
        FastLED.show();
        delay(300);
      }
    } else {
      Serial.println("CHECK!");
      displayStaticInfo("WARNING!", "You're in Check!");
    }
  }
  
  waitingForAIMove = false;
  aiMoveInProgress = false;
  aiMoveFrom = "";
  aiMoveTo = "";
  
  FastLED.clear();
  FastLED.show();
}

// Handle player move - extracted from loop() to fix goto-over-declarations bug
void handlePlayerMove(int row, int col) {
  char piece = board[row][col];

  if (piece == ' ') {
    sensorPrev[row][col] = sensorState[row][col];
    return;
  }

  char pieceColor = (piece >= 'a' && piece <= 'z') ? 'b' : 'w';
  if (pieceColor != 'w') {
    Serial.println("That's not your piece!");
    sensorState[row][col] = true;
    sensorPrev[row][col] = true;
    return;
  }

  Serial.print("Piece lifted from ");
  Serial.print((char)('a' + col));
  Serial.println(row + 1);
  Serial.print("Piece type: ");
  Serial.println(piece);

  if (!gameStarted) {
    Serial.println("Game not started! Press your button to begin.");
    sensorState[row][col] = true;
    sensorPrev[row][col] = true;
    FastLED.clear();
    FastLED.show();
    return;
  }

  if (p1Time == 0 || p2Time == 0) {
    Serial.println("Game over - time expired!");
    sensorState[row][col] = true;
    sensorPrev[row][col] = true;
    FastLED.clear();
    FastLED.show();
    if (!gameOver) {
      gameOver = true;
      winner = (p1Time == 0) ? 'b' : 'w';
    }
    return;
  }

  int moveCount = 0;
  int moves[30][2];
  getPossibleMoves(row, col, moveCount, moves);

  int legalMoveCount = 0;
  int legalMoves[30][2];
  for (int i = 0; i < moveCount; i++) {
    if (!wouldMoveLeaveKingInCheck(row, col, moves[i][0], moves[i][1])) {
      legalMoves[legalMoveCount][0] = moves[i][0];
      legalMoves[legalMoveCount][1] = moves[i][1];
      legalMoveCount++;
    }
  }

  Serial.print("Possible moves: ");
  Serial.println(legalMoveCount);

  FastLED.clear();
  leds[getPixelIndex(row, col)] = ORIGIN_COLOR;

  for (int i = 0; i < legalMoveCount; i++) {
    int r = legalMoves[i][0];
    int c = legalMoves[i][1];
    leds[getPixelIndex(r, c)] = (board[r][c] == ' ') ? MOVE_COLOR : CAPTURE_COLOR;
    Serial.print("  -> ");
    Serial.print((char)('a' + c));
    Serial.print(r + 1);
    Serial.println((board[r][c] == ' ') ? " (move)" : " (CAPTURE)");
  }
  FastLED.show();

  if (WOKWI_TEST_MODE) showLEDPattern();

  int targetRow = -1, targetCol = -1;
  bool piecePlaced = false;
  bool pieceReturnedEarly = false;

  unsigned long startWait = millis();
  const unsigned long waitTimeout = 30000;

  while (!piecePlaced && (millis() - startWait < waitTimeout)) {
    handleSerialCommands();
    if (!WOKWI_TEST_MODE) readSensors();

    if (sensorState[row][col] && !pieceReturnedEarly) {
      pieceReturnedEarly = true;
      Serial.println("Piece returned to original square.");
      break;
    }

    for (int r2 = 0; r2 < 8; r2++) {
      for (int c2 = 0; c2 < 8; c2++) {
        if (sensorState[r2][c2] && !sensorPrev[r2][c2] && !(r2 == row && c2 == col)) {
          targetRow = r2;
          targetCol = c2;
          piecePlaced = true;
          break;
        }
      }
      if (piecePlaced) break;
    }

    FastLED.show();
    delay(10);
  }

  if (pieceReturnedEarly) {
    sensorPrev[row][col] = true;
    FastLED.clear();
    FastLED.show();
    return;
  }

  if (!piecePlaced) {
    Serial.println("Move timeout - piece not placed!");
    FastLED.clear();
    FastLED.show();
    sensorState[row][col] = true;
    sensorPrev[row][col] = true;
    return;
  }

  Serial.print("Piece placed at ");
  Serial.print((char)('a' + targetCol));
  Serial.println(targetRow + 1);

  if (targetRow == row && targetCol == col) {
    Serial.println("Piece returned to original square.");
    sensorPrev[row][col] = true;
    FastLED.clear();
    FastLED.show();
    return;
  }

  bool legal = false;
  for (int i = 0; i < legalMoveCount; i++) {
    if (legalMoves[i][0] == targetRow && legalMoves[i][1] == targetCol) {
      legal = true;
      break;
    }
  }

  if (!legal) {
    Serial.println("Illegal move! Reverting state...");
    sensorState[row][col] = true;
    sensorState[targetRow][targetCol] = (board[targetRow][targetCol] != ' ');
    sensorPrev[row][col] = true;
    sensorPrev[targetRow][targetCol] = (board[targetRow][targetCol] != ' ');
    FastLED.clear();
    FastLED.show();
    return;
  }

  // Button confirmation
  Serial.println("Press your button to confirm move!");
  moveConfirmedByButton = false;
  unsigned long confirmWait = millis();

  while (!moveConfirmedByButton && (millis() - confirmWait < 10000)) {
    unsigned long now = millis();
    if (gameStarted && (now - lastTick >= 1000)) {
      lastTick = now;
      if (p1Turn && p1Time > 0) p1Time--;
      else if (!p1Turn && p2Time > 0) p2Time--;
      updateClock();
    }

    if (digitalRead(BTN_P1) == LOW) {
      moveConfirmedByButton = true;

      if (!gameStarted) {
        gameStarted = true;
        lastTick = millis();
        p1Turn = true;
        Serial.println("Game clock started! Your clock running.");
        displayStaticInfo("GAME START!", "Good Luck!");
        startGameAnimation();
        delay(500);
      }

      delay(200);
    }
    delay(10);
  }

  if (!moveConfirmedByButton) {
    Serial.println("Move timeout - no button confirmation!");
    sensorState[row][col] = true;
    sensorState[targetRow][targetCol] = (board[targetRow][targetCol] != ' ');
    sensorPrev[row][col] = true;
    sensorPrev[targetRow][targetCol] = (board[targetRow][targetCol] != ' ');
    FastLED.clear();
    FastLED.show();
    return;
  }

  Serial.println("Move confirmed! Executing...");

  bool isCapture = (board[targetRow][targetCol] != ' ');
  if (isCapture) {
    Serial.print("CAPTURE! ");
    Serial.print(piece);
    Serial.print(" takes ");
    Serial.println(board[targetRow][targetCol]);
    captureAnimation();
  }

  // Handle castling
  if ((piece == 'K' || piece == 'k') && abs(targetCol - col) == 2) {
    Serial.println("CASTLING!");
    int rookRow = (piece == 'K') ? 0 : 7;
    if (targetCol == 6) {
      board[rookRow][5] = board[rookRow][7];
      board[rookRow][7] = ' ';
      Serial.println("Kingside castling - please move the rook from h to f");
    } else {
      board[rookRow][3] = board[rookRow][0];
      board[rookRow][0] = ' ';
      Serial.println("Queenside castling - please move the rook from a to d");
    }
  }

  // Handle en passant
  if ((piece == 'P' || piece == 'p') && targetCol != col && board[targetRow][targetCol] == ' ') {
    int captureRow = (piece == 'P') ? targetRow - 1 : targetRow + 1;
    Serial.print("EN PASSANT! Captured pawn at ");
    Serial.print((char)('a' + targetCol));
    Serial.println(captureRow + 1);
    board[captureRow][targetCol] = ' ';
    captureAnimation();
  }

  // Update board
  board[targetRow][targetCol] = piece;
  board[row][col] = ' ';

  // Track en passant possibility
  enPassantCol = -1;
  enPassantRow = -1;
  if ((piece == 'P' || piece == 'p') && abs(targetRow - row) == 2) {
    enPassantCol = col;
    enPassantRow = (row + targetRow) / 2;
  }

  updateCastlingRights(row, col, piece);
  checkForPromotion(targetRow, targetCol, piece);

  sensorPrev[row][col] = false;
  sensorPrev[targetRow][targetCol] = true;

  whiteTurn = !whiteTurn;
  if (gameStarted) p1Turn = !p1Turn;

  // Check for check/checkmate
  char enemyColor = whiteTurn ? 'w' : 'b';
  if (isKingInCheck(enemyColor)) {
    if (isCheckmate(enemyColor)) {
      Serial.println("\n*** CHECKMATE! ***");
      Serial.print(whiteTurn ? "AI" : "YOU");
      Serial.println(" WIN!");
      gameOver = true;
      winner = whiteTurn ? 'b' : 'w';

      for (int i = 0; i < 5; i++) {
        fill_solid(leds, LED_COUNT, (winner == 'w') ? CRGB::White : CRGB::Purple);
        FastLED.show();
        delay(300);
        FastLED.clear();
        FastLED.show();
        delay(300);
      }
    } else {
      Serial.println("CHECK!");
    }
  }

  FastLED.clear();
  FastLED.show();

  if (!gameOver) {
    Serial.println("Your move complete. AI is thinking...");
    displayStaticInfo("AI Thinking...", "Please wait");
    getAIMove();
  }
}

bool isSquareAttacked(int row, int col, char byColor) {
  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 8; c++) {
      char piece = board[r][c];
      if (piece == ' ') continue;
      
      char pieceColor = (piece >= 'a' && piece <= 'z') ? 'b' : 'w';
      if (pieceColor != byColor) continue;
      
      char pieceType = (piece >= 'a' && piece <= 'z') ? piece - 32 : piece;
      
      switch(pieceType) {
        case 'P': {
          int direction = (pieceColor == 'w') ? 1 : -1;
          int captureColumns[] = {c-1, c+1};
          for (int i = 0; i < 2; i++) {
            if (r + direction == row && captureColumns[i] == col) {
              return true;
            }
          }
          break;
        }
        case 'N': {
          int knightMoves[8][2] = {{2,1}, {1,2}, {-1,2}, {-2,1},
                                    {-2,-1}, {-1,-2}, {1,-2}, {2,-1}};
          for (int i = 0; i < 8; i++) {
            if (r + knightMoves[i][0] == row && c + knightMoves[i][1] == col) {
              return true;
            }
          }
          break;
        }
        case 'K': {
          int kingMoves[8][2] = {{1,0}, {-1,0}, {0,1}, {0,-1},
                                  {1,1}, {1,-1}, {-1,1}, {-1,-1}};
          for (int i = 0; i < 8; i++) {
            if (r + kingMoves[i][0] == row && c + kingMoves[i][1] == col) {
              return true;
            }
          }
          break;
        }
        case 'R': {
          int directions[4][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
          for (int d = 0; d < 4; d++) {
            for (int step = 1; step < 8; step++) {
              int newRow = r + step * directions[d][0];
              int newCol = c + step * directions[d][1];
              if (newRow < 0 || newRow >= 8 || newCol < 0 || newCol >= 8) break;
              if (newRow == row && newCol == col) return true;
              if (board[newRow][newCol] != ' ') break;
            }
          }
          break;
        }
        case 'B': {
          int directions[4][2] = {{1,1}, {1,-1}, {-1,1}, {-1,-1}};
          for (int d = 0; d < 4; d++) {
            for (int step = 1; step < 8; step++) {
              int newRow = r + step * directions[d][0];
              int newCol = c + step * directions[d][1];
              if (newRow < 0 || newRow >= 8 || newCol < 0 || newCol >= 8) break;
              if (newRow == row && newCol == col) return true;
              if (board[newRow][newCol] != ' ') break;
            }
          }
          break;
        }
        case 'Q': {
          int directions[8][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}, 
                                   {1,1}, {1,-1}, {-1,1}, {-1,-1}};
          for (int d = 0; d < 8; d++) {
            for (int step = 1; step < 8; step++) {
              int newRow = r + step * directions[d][0];
              int newCol = c + step * directions[d][1];
              if (newRow < 0 || newRow >= 8 || newCol < 0 || newCol >= 8) break;
              if (newRow == row && newCol == col) return true;
              if (board[newRow][newCol] != ' ') break;
            }
          }
          break;
        }
      }
    }
  }
  return false;
}

bool isKingInCheck(char kingColor) {
  int kingRow = -1, kingCol = -1;
  char kingPiece = (kingColor == 'w') ? 'K' : 'k';
  
  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 8; c++) {
      if (board[r][c] == kingPiece) {
        kingRow = r;
        kingCol = c;
        break;
      }
    }
    if (kingRow != -1) break;
  }
  
  if (kingRow == -1) return false;
  
  char enemyColor = (kingColor == 'w') ? 'b' : 'w';
  return isSquareAttacked(kingRow, kingCol, enemyColor);
}

bool wouldMoveLeaveKingInCheck(int fromRow, int fromCol, int toRow, int toCol) {
  char piece = board[fromRow][fromCol];
  char capturedPiece = board[toRow][toCol];
  
  board[toRow][toCol] = piece;
  board[fromRow][fromCol] = ' ';
  
  char pieceColor = (piece >= 'a' && piece <= 'z') ? 'b' : 'w';
  bool inCheck = isKingInCheck(pieceColor);
  
  board[fromRow][fromCol] = piece;
  board[toRow][toCol] = capturedPiece;
  
  return inCheck;
}

bool isCheckmate(char kingColor) {
  if (!isKingInCheck(kingColor)) return false;
  
  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 8; c++) {
      char piece = board[r][c];
      if (piece == ' ') continue;
      
      char pieceColor = (piece >= 'a' && piece <= 'z') ? 'b' : 'w';
      if (pieceColor != kingColor) continue;
      
      int moveCount = 0;
      int moves[30][2];
      getPossibleMoves(r, c, moveCount, moves);
      
      for (int i = 0; i < moveCount; i++) {
        if (!wouldMoveLeaveKingInCheck(r, c, moves[i][0], moves[i][1])) {
          return false;
        }
      }
    }
  }
  
  return true;
}

void updateCastlingRights(int fromRow, int fromCol, char piece) {
  if (piece == 'K') whiteKingMoved = true;
  if (piece == 'k') blackKingMoved = true;

  if (piece == 'R' && fromRow == 0 && fromCol == 0) whiteRookQueensideMoved = true;
  if (piece == 'R' && fromRow == 0 && fromCol == 7) whiteRookKingsideMoved = true;
  if (piece == 'r' && fromRow == 7 && fromCol == 0) blackRookQueensideMoved = true;
  if (piece == 'r' && fromRow == 7 && fromCol == 7) blackRookKingsideMoved = true;
}

void handleSerialCommands() {
  if (!WOKWI_TEST_MODE) return;
  
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();
    
    if (cmd.startsWith("LIFT ")) {
      String square = cmd.substring(5);
      square.trim();
      square.toLowerCase();
      int col = square.charAt(0) - 'a';
      int row = square.charAt(1) - '1';
      
      if (row >= 0 && row < 8 && col >= 0 && col < 8) {
        Serial.print("Lifting piece from ");
        Serial.println(square);
        sensorState[row][col] = false;
      }
    }
    else if (cmd.startsWith("PLACE ")) {
      String square = cmd.substring(6);
      square.trim();
      square.toLowerCase();
      int col = square.charAt(0) - 'a';
      int row = square.charAt(1) - '1';
      
      if (row >= 0 && row < 8 && col >= 0 && col < 8) {
        Serial.print("Placing piece on ");
        Serial.println(square);
        sensorPrev[row][col] = sensorState[row][col];
        sensorState[row][col] = true;
      }
    }
    else if (cmd.startsWith("MOVE ")) {
      int firstSpace = cmd.indexOf(' ');
      int secondSpace = cmd.indexOf(' ', firstSpace + 1);
      
      String from = cmd.substring(firstSpace + 1, secondSpace);
      String to = cmd.substring(secondSpace + 1);
      
      from.trim();
      to.trim();
      from.toLowerCase();
      to.toLowerCase();
      
      if (from.length() >= 2 && to.length() >= 2) {
        int fromCol = from.charAt(0) - 'a';
        int fromRow = from.charAt(1) - '1';
        int toCol = to.charAt(0) - 'a';
        int toRow = to.charAt(1) - '1';
        
        Serial.print("Moving piece: ");
        Serial.print(from);
        Serial.print(" -> ");
        Serial.println(to);
        
        for (int r = 0; r < 8; r++) {
          for (int c = 0; c < 8; c++) {
            sensorPrev[r][c] = sensorState[r][c];
          }
        }
        
        sensorState[fromRow][fromCol] = false;
        delay(150);
        
        sensorPrev[toRow][toCol] = false;
        sensorState[toRow][toCol] = true;
      }
    }
    else if (cmd == "BOARD") {
      Serial.println("\n=== Current Board State ===");
      for (int row = 7; row >= 0; row--) {
        Serial.print(row + 1);
        Serial.print(" | ");
        for (int col = 0; col < 8; col++) {
          char piece = board[row][col];
          Serial.print(piece == ' ' ? '.' : piece);
          Serial.print(" ");
        }
        Serial.println();
      }
      Serial.println("  +----------------");
      Serial.println("    a b c d e f g h");
      Serial.print("Turn: ");
      Serial.println(whiteTurn ? "WHITE" : "BLACK");
      Serial.println();
    }
    else if (cmd == "FEN") {
      String fen = generateFEN();
      Serial.println("FEN: " + fen);
      
      Serial.println("\n=== Board State ===");
      for (int row = 7; row >= 0; row--) {
        Serial.print(row + 1);
        Serial.print(" | ");
        for (int col = 0; col < 8; col++) {
          char piece = board[row][col];
          Serial.print(piece == ' ' ? '.' : piece);
          Serial.print(" ");
        }
        Serial.println();
      }
      Serial.println("  +----------------");
      Serial.println("    a b c d e f g h\n");
    }
    else if (cmd == "AI") {
      getAIMove();
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Wire.begin(8, 9);

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.println("IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi Failed!");
  }

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, LED_COUNT);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  pinMode(MUX_S0, OUTPUT);
  pinMode(MUX_S1, OUTPUT);
  pinMode(MUX_S2, OUTPUT);
  pinMode(MUX_S3, OUTPUT);
  pinMode(MUX_EN, OUTPUT);
  pinMode(BTN_P1, INPUT_PULLUP);
  pinMode(BTN_P2, INPUT_PULLUP);
  
  digitalWrite(MUX_EN, LOW);

  display1.setBrightness(0x0f);
  display2.setBrightness(0x0f);

  lcd.init();
  lcd.backlight();
  lcd.createChar(0, pawn);

  scrollCredits();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(0);
  lcd.print(" CHESS ENGINE ");
  lcd.setCursor(0, 1);
  lcd.print("  AI MODE v1.0  ");
  delay(2000);

  if (isAIPlaying) {
    playAIDialogue(aiGreetings[random(0, 2)], true);
  }
  
  updateClock();

  for (int i = 0; i < 4; i++) {
    pinMode(sigPins[i], INPUT_PULLUP);
  }

  for (int row = 0; row < 8; row++){
    for (int col = 0; col < 8; col++){
      board[row][col] = initialBoard[row][col];
    }
  }

  Serial.println("Simulating initial board setup...");
  for (int row = 0; row < 8; row++){
    for (int col = 0; col < 8; col++){
      if (initialBoard[row][col] != ' ') {
        sensorState[row][col] = true;
        sensorPrev[row][col] = true;
      } else {
        sensorState[row][col] = false;
        sensorPrev[row][col] = false;
      }
    }
  }
  
  Serial.println("Playing startup animation...");
  fireworkAnimation();
  delay(500);
  
  Serial.println("Board ready! AI mode initialized.");
  Serial.println("WHITE's turn to move (YOU).");
  Serial.println("Type 'FEN' to see board state, 'AI' to test AI move");
}

void loop() {
  unsigned long currentMillis = millis();
  
  // 1. Handle button presses for clock control AND move confirmation
  if (digitalRead(BTN_P1) == LOW) {
    delay(50);
    if (whiteTurn) {
      moveConfirmedByButton = true;
    }
    if (!gameStarted) {
      gameStarted = true;
      lastTick = millis();
    }
  }

  // 2. Update clock every second
  if (gameStarted && (currentMillis - lastTick >= 1000)) {
    lastTick = currentMillis;
    if (p1Turn && p1Time > 0) p1Time--;
    else if (!p1Turn && p2Time > 0) p2Time--;
    updateClock();
  }

  // Reset system - both buttons held for 3 seconds
  static unsigned long bothButtonsPressedTime = 0;
  static bool resetInProgress = false;

  if (digitalRead(BTN_P1) == LOW && digitalRead(BTN_P2) == LOW) {
    if (!resetInProgress) {
      bothButtonsPressedTime = millis();
      resetInProgress = true;
      Serial.println("Hold both buttons to reset...");
      displayStaticInfo("RESET MODE", "Hold 3 seconds");
    }
    
    if (millis() - bothButtonsPressedTime >= 3000) {
      Serial.println("\n*** RESETTING GAME ***");
      
      for (int row = 0; row < 8; row++){
        for (int col = 0; col < 8; col++){
          board[row][col] = initialBoard[row][col];
          if (initialBoard[row][col] != ' ') {
            sensorState[row][col] = true;
            sensorPrev[row][col] = true;
          } else {
            sensorState[row][col] = false;
            sensorPrev[row][col] = false;
          }
        }
      }
      
      whiteTurn = true;
      gameOver = false;
      winner = ' ';
      gameStarted = false;
      p1Turn = true;
      
      waitingForAIMove = false;
      aiMoveInProgress = false;
      aiMoveFrom = "";
      aiMoveTo = "";
      
      p1Time = 300;
      p2Time = 300;
      updateClock();
      
      whiteKingMoved = false;
      whiteRookKingsideMoved = false;
      whiteRookQueensideMoved = false;
      blackKingMoved = false;
      blackRookKingsideMoved = false;
      blackRookQueensideMoved = false;
      
      enPassantCol = -1;
      enPassantRow = -1;
      
      FastLED.clear();
      FastLED.show();
      
      displayStaticInfo("GAME RESET!", "Ready to Play");
      delay(2000);
      displayStaticInfo("Game Status:", "Press to Start");
      
      Serial.println("Game reset complete. YOUR turn to move.");
      Serial.println("Please ensure all pieces are in starting positions.");
      
      resetInProgress = false;
      bothButtonsPressedTime = 0;
      
      while (digitalRead(BTN_P1) == LOW || digitalRead(BTN_P2) == LOW) {
        delay(50);
      }
      delay(200);
    }
  } else {
    resetInProgress = false;
    bothButtonsPressedTime = 0;
  }

  // 3. Cycle through display information
  if (currentMillis - lastDisplayUpdate >= displayUpdateInterval) {
    lastDisplayUpdate = currentMillis;
    
    switch(displayState) {
      case 0: {
        if (!gameOver && !waitingForAIMove) {
          int whitePieces = countPieces('w');
          int blackPieces = countPieces('b');
          char line2[17];
          sprintf(line2, "You:%d AI:%d", whitePieces, blackPieces);
          displayStaticInfo(whiteTurn ? "> YOUR TURN" : "> AI THINKING", line2);
        } else if (waitingForAIMove) {
          char moveText[17];
          sprintf(moveText, "%s -> %s", aiMoveFrom.c_str(), aiMoveTo.c_str());
          displayStaticInfo("AI Move:", moveText);
        }
        break;
      }
      case 1: {
        if (!gameOver && !waitingForAIMove) {
          if (isAIPlaying && (random(0, 5) == 1)) {
            playAIDialogue(aiRandomLines[random(0, 8)], false);
          } else {
            int materialDiff = calculateMaterial();
            char line2[17];
            if (materialDiff > 0) sprintf(line2, "You +%d", materialDiff);
            else if (materialDiff < 0) sprintf(line2, "AI +%d", -materialDiff);
            else sprintf(line2, "Equal");
            displayStaticInfo("Material:", line2);
          }
        }
        break;
      }
      case 2: {
        if (!gameOver) displayClockInfo();
        break;
      }
      case 3: {
        if (gameOver) {
          displayStaticInfo("CHECKMATE!", winner == 'w' ? "You Win!" : "AI Wins!");
        } else if (!gameStarted) {
          displayStaticInfo("Game Status:", "Press to Start");
        } else if (isKingInCheck('w')) {
          displayStaticInfo("WARNING!", "You're in Check!");
        } else if (isKingInCheck('b')) {
          displayStaticInfo("AI Status:", "AI in Check!");
        } else if (p1Time == 0) {
          displayStaticInfo("TIME UP!", "AI Wins!");
          gameOver = true;
          winner = 'b';
        } else if (p2Time == 0) {
          displayStaticInfo("TIME UP!", "You Win!");
          gameOver = true;
          winner = 'w';
        } else {
          displayStaticInfo("Game Status:", "In Progress");
        }
        break;
      }
    }
    
    displayState = (displayState + 1) % 4;
  }

  handleSerialCommands();
  
  if (!WOKWI_TEST_MODE) readSensors();

  if (gameOver) {
    delay(100);
    return;
  }

// AI MOVE EXECUTION PHASE
if (waitingForAIMove && !whiteTurn) {
  if (aiMoveFrom.length() < 2 || aiMoveTo.length() < 2) {
    Serial.println("Invalid AI move data!");
    waitingForAIMove = false;
    return;
  }

  // Ground truth - never changes during this AI move
  int fromRow = aiMoveTo[1] - '1'; // intentionally unused
  int fromCol = aiMoveFrom[0] - 'a';
  fromRow     = aiMoveFrom[1] - '1';
  int toRow   = aiMoveTo[1]   - '1';
  int toCol   = aiMoveTo[0]   - 'a';

  // Persist lifted square across loop iterations
  static int savedLiftedRow = -1;
  static int savedLiftedCol = -1;

  // Always show AI move on LEDs
  FastLED.clear();
  leds[getPixelIndex(fromRow, fromCol)] = AI_FROM_COLOR;
  leds[getPixelIndex(toRow, toCol)]     = AI_TO_COLOR;
  FastLED.show();

  // Detect lift this iteration
  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 8; c++) {
      if (sensorPrev[r][c] && !sensorState[r][c]) {
        if (!aiMoveInProgress) {
          savedLiftedRow = r;
          savedLiftedCol = c;
          aiMoveInProgress = true;
          Serial.print("Piece lifted from: ");
          Serial.print((char)('a' + c));
          Serial.println(r + 1);
        }
      }
    }
  }

  // Detect placement this iteration
  if (aiMoveInProgress) {
    for (int r = 0; r < 8; r++) {
      for (int c = 0; c < 8; c++) {
        if (!sensorPrev[r][c] && sensorState[r][c]) {

          // Placed back on origin = cancelled
          if (r == savedLiftedRow && c == savedLiftedCol) {
            Serial.println("Piece returned. Try again.");
            aiMoveInProgress = false;
            savedLiftedRow = -1;
            savedLiftedCol = -1;
          }
          // Correct full move
          else if (r == toRow && c == toCol &&
                   savedLiftedRow == fromRow && savedLiftedCol == fromCol) {
            Serial.println("AI move executed correctly!");
            executeAIMove();
            savedLiftedRow = -1;
            savedLiftedCol = -1;
            if (!gameOver) Serial.println("Your turn!");
          }
          // Wrong move
          else {
            Serial.print("WRONG MOVE! Expected: ");
            Serial.print((char)('a' + fromCol)); Serial.print(fromRow + 1);
            Serial.print(" -> ");
            Serial.print((char)('a' + toCol)); Serial.println(toRow + 1);

            char expectedMove[17];
            sprintf(expectedMove, "%c%d -> %c%d",
              ('a' + fromCol), (fromRow + 1),
              ('a' + toCol),   (toRow + 1));
            displayStaticInfo("WRONG MOVE!", expectedMove);

            for (int flash = 0; flash < 4; flash++) {
              fill_solid(leds, LED_COUNT, CRGB::Red);
              FastLED.show();
              delay(250);
              FastLED.clear();
              FastLED.show();
              delay(250);
            }

            // Reset so user can try again from scratch
            aiMoveInProgress = false;
            savedLiftedRow = -1;
            savedLiftedCol = -1;

            // Re-show correct AI move
            FastLED.clear();
            leds[getPixelIndex(fromRow, fromCol)] = AI_FROM_COLOR;
            leds[getPixelIndex(toRow, toCol)]     = AI_TO_COLOR;
            FastLED.show();

            char moveText[17];
            sprintf(moveText, "%c%d -> %c%d",
              ('a' + fromCol), (fromRow + 1),
              ('a' + toCol),   (toRow + 1));
            displayStaticInfo("AI Move:", moveText);
          }
        }
      }
    }
  }

  for (int r = 0; r < 8; r++)
    for (int c = 0; c < 8; c++)
      sensorPrev[r][c] = sensorState[r][c];

  delay(50);
  return;
}
  // PLAYER MOVE PHASE (WHITE)
  if (whiteTurn && !waitingForAIMove) {
    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        if (sensorPrev[row][col] && !sensorState[row][col]) {
          handlePlayerMove(row, col);
          goto endLoop;
        }
      }
    }
  }
  
  endLoop:
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      sensorPrev[row][col] = sensorState[row][col];
    }
  }
  
  delay(50);
}

void readSensors() {
  if (WOKWI_TEST_MODE) return;
  digitalWrite(MUX_EN, LOW);
  for (int ch = 0; ch < 16; ch++) {
    digitalWrite(MUX_S0, bitRead(ch, 0));
    digitalWrite(MUX_S1, bitRead(ch, 1));
    digitalWrite(MUX_S2, bitRead(ch, 2));
    digitalWrite(MUX_S3, bitRead(ch, 3));
    delayMicroseconds(20);
    for (int m = 0; m < 4; m++) {
      int val = digitalRead(sigPins[m]);
      int buttonID = (m * 16) + ch;
      int r = buttonID / 8;
      int c = buttonID % 8;
      sensorState[r][c] = (val == LOW);
    }
  }
}

void fireworkAnimation() {
  CRGB colors[] = {CRGB::Red, CRGB::Orange, CRGB::Yellow, CRGB::Green, CRGB::Blue, CRGB::Purple};
  int colorIndex = 0;
  float centerX = 3.5;
  float centerY = 3.5;
  for (float radius = 0; radius <= 6; radius += 0.3) {
    FastLED.clear();
    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        float dist = sqrt(pow(col - centerX, 2) + pow(row - centerY, 2));
        if (dist <= radius) {
          leds[getPixelIndex(row, col)] = colors[colorIndex % 6];
        }
      }
    }
    FastLED.show();
    colorIndex++;
    delay(50);
  }
  delay(200);
  for (float radius = 6; radius >= 0; radius -= 0.3) {
    FastLED.clear();
    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        float dist = sqrt(pow(col - centerX, 2) + pow(row - centerY, 2));
        if (dist <= radius) {
          leds[getPixelIndex(row, col)] = colors[(colorIndex + 3) % 6];
        }
      }
    }
    FastLED.show();
    colorIndex++;
    delay(50);
  }
  FastLED.clear();
  FastLED.show();
}

void startGameAnimation() {
  for (int i = 0; i < LED_COUNT; i++) {
    leds[i] = CRGB::Green;
    FastLED.show();
    delay(15);
  }
  delay(300);
  FastLED.clear();
  FastLED.show();
  delay(100);
  fill_solid(leds, LED_COUNT, CRGB::Green);
  FastLED.show();
  delay(200);
  FastLED.clear();
  FastLED.show();
}

void captureAnimation() {
  for (int pulse = 0; pulse < 3; pulse++) {
    fill_solid(leds, LED_COUNT, (pulse % 2 == 0) ? CRGB::Red : CRGB::Orange);
    FastLED.show();
    delay(150);
  }
  FastLED.clear();
  FastLED.show();
}

void promotionAnimation(int col) {
  for (int step = 0; step < 16; step++) {
    for (int row = 0; row < 8; row++) {
      int idx = getPixelIndex(row, col);
      leds[idx] = ((step + row) % 8 < 4) ? PROMOTION_COLOR : CRGB::Black;
    }
    FastLED.show();
    delay(100);
  }
}

void checkForPromotion(int targetRow, int targetCol, char piece) {
  if ((piece == 'P' && targetRow == 7) || (piece == 'p' && targetRow == 0)) {
    bool isWhite = (piece == 'P');
    promotionAnimation(targetCol);
    board[targetRow][targetCol] = isWhite ? 'Q' : 'q';
    Serial.println("Pawn promoted to Queen!");
  }
}

void getPossibleMoves(int row, int col, int &moveCount, int moves[][2]) {
  moveCount = 0;
  char piece = board[row][col];
  
  if (piece == ' ') return;
  
  char pieceColor = (piece >= 'a' && piece <= 'z') ? 'b' : 'w';
  char pieceType = (piece >= 'a' && piece <= 'z') ? piece - 32 : piece;

  switch(pieceType) {
    case 'P': {
      int direction = (pieceColor == 'w') ? 1 : -1;
      
      if (row + direction >= 0 && row + direction < 8 && board[row + direction][col] == ' ') {
        moves[moveCount][0] = row + direction;
        moves[moveCount][1] = col;
        moveCount++;
        
        if ((pieceColor == 'w' && row == 1) || (pieceColor == 'b' && row == 6)) {
          if (board[row + 2*direction][col] == ' ') {
            moves[moveCount][0] = row + 2*direction;
            moves[moveCount][1] = col;
            moveCount++;
          }
        }
      }
      
      int captureColumns[] = {col-1, col+1};
      for (int i = 0; i < 2; i++) {
        int newRow = row + direction;
        int newCol = captureColumns[i];
        if (newCol >= 0 && newCol < 8 && newRow >= 0 && newRow < 8) {
          char targetPiece = board[newRow][newCol];
          if (targetPiece != ' ' && 
              ((pieceColor == 'w' && targetPiece >= 'a' && targetPiece <= 'z') || 
               (pieceColor == 'b' && targetPiece >= 'A' && targetPiece <= 'Z'))) {
            moves[moveCount][0] = newRow;
            moves[moveCount][1] = newCol;
            moveCount++;
          }
          
          if (newCol == enPassantCol && newRow == enPassantRow) {
            moves[moveCount][0] = newRow;
            moves[moveCount][1] = newCol;
            moveCount++;
          }
        }
      }
      break;
    }
    
    case 'R': {
      int directions[4][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
      for (int d = 0; d < 4; d++) {
        for (int step = 1; step < 8; step++) {
          int newRow = row + step * directions[d][0];
          int newCol = col + step * directions[d][1];
          if (newRow < 0 || newRow >= 8 || newCol < 0 || newCol >= 8) break;
          char targetPiece = board[newRow][newCol];
          if (targetPiece == ' ') {
            moves[moveCount][0] = newRow;
            moves[moveCount][1] = newCol;
            moveCount++;
          } else {
            if ((pieceColor == 'w' && targetPiece >= 'a' && targetPiece <= 'z') ||
                (pieceColor == 'b' && targetPiece >= 'A' && targetPiece <= 'Z')) {
              moves[moveCount][0] = newRow;
              moves[moveCount][1] = newCol;
              moveCount++;
            }
            break;
          }
        }
      }
      break;
    }
    
    case 'N': {
      int knightMoves[8][2] = {{2,1}, {1,2}, {-1,2}, {-2,1},
                                {-2,-1}, {-1,-2}, {1,-2}, {2,-1}};
      for (int i = 0; i < 8; i++) {
        int newRow = row + knightMoves[i][0];
        int newCol = col + knightMoves[i][1];
        if (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
          char targetPiece = board[newRow][newCol];
          if (targetPiece == ' ' || 
              ((pieceColor == 'w' && targetPiece >= 'a' && targetPiece <= 'z') ||
               (pieceColor == 'b' && targetPiece >= 'A' && targetPiece <= 'Z'))) {
            moves[moveCount][0] = newRow;
            moves[moveCount][1] = newCol;
            moveCount++;
          }
        }
      }
      break;
    }
    
    case 'B': {
      int directions[4][2] = {{1,1}, {1,-1}, {-1,1}, {-1,-1}};
      for (int d = 0; d < 4; d++) {
        for (int step = 1; step < 8; step++) {
          int newRow = row + step * directions[d][0];
          int newCol = col + step * directions[d][1];
          if (newRow < 0 || newRow >= 8 || newCol < 0 || newCol >= 8) break;
          char targetPiece = board[newRow][newCol];
          if (targetPiece == ' ') {
            moves[moveCount][0] = newRow;
            moves[moveCount][1] = newCol;
            moveCount++;
          } else {
            if ((pieceColor == 'w' && targetPiece >= 'a' && targetPiece <= 'z') ||
                (pieceColor == 'b' && targetPiece >= 'A' && targetPiece <= 'Z')) {
              moves[moveCount][0] = newRow;
              moves[moveCount][1] = newCol;
              moveCount++;
            }
            break;
          }
        }
      }
      break;
    }
    
    case 'Q': {
      int directions[8][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}, 
                               {1,1}, {1,-1}, {-1,1}, {-1,-1}};
      for (int d = 0; d < 8; d++) {
        for (int step = 1; step < 8; step++) {
          int newRow = row + step * directions[d][0];
          int newCol = col + step * directions[d][1];
          if (newRow < 0 || newRow >= 8 || newCol < 0 || newCol >= 8) break;
          char targetPiece = board[newRow][newCol];
          if (targetPiece == ' ') {
            moves[moveCount][0] = newRow;
            moves[moveCount][1] = newCol;
            moveCount++;
          } else {
            if ((pieceColor == 'w' && targetPiece >= 'a' && targetPiece <= 'z') ||
                (pieceColor == 'b' && targetPiece >= 'A' && targetPiece <= 'Z')) {
              moves[moveCount][0] = newRow;
              moves[moveCount][1] = newCol;
              moveCount++;
            }
            break;
          }
        }
      }
      break;
    }
    
    case 'K': {
      int kingMoves[8][2] = {{1,0}, {-1,0}, {0,1}, {0,-1},
                              {1,1}, {1,-1}, {-1,1}, {-1,-1}};
      for (int i = 0; i < 8; i++) {
        int newRow = row + kingMoves[i][0];
        int newCol = col + kingMoves[i][1];
        if (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
          char targetPiece = board[newRow][newCol];
          if (targetPiece == ' ' || 
              ((pieceColor == 'w' && targetPiece >= 'a' && targetPiece <= 'z') ||
               (pieceColor == 'b' && targetPiece >= 'A' && targetPiece <= 'Z'))) {
            moves[moveCount][0] = newRow;
            moves[moveCount][1] = newCol;
            moveCount++;
          }
        }
      }
      
      if (pieceColor == 'w' && !whiteKingMoved && !isKingInCheck('w')) {
        if (!whiteRookKingsideMoved && 
            board[0][5] == ' ' && board[0][6] == ' ' &&
            !isSquareAttacked(0, 5, 'b') && !isSquareAttacked(0, 6, 'b')) {
          moves[moveCount][0] = 0;
          moves[moveCount][1] = 6;
          moveCount++;
        }
        if (!whiteRookQueensideMoved && 
            board[0][1] == ' ' && board[0][2] == ' ' && board[0][3] == ' ' &&
            !isSquareAttacked(0, 2, 'b') && !isSquareAttacked(0, 3, 'b')) {
          moves[moveCount][0] = 0;
          moves[moveCount][1] = 2;
          moveCount++;
        }
      }
      
      if (pieceColor == 'b' && !blackKingMoved && !isKingInCheck('b')) {
        if (!blackRookKingsideMoved && 
            board[7][5] == ' ' && board[7][6] == ' ' &&
            !isSquareAttacked(7, 5, 'w') && !isSquareAttacked(7, 6, 'w')) {
          moves[moveCount][0] = 7;
          moves[moveCount][1] = 6;
          moveCount++;
        }
        if (!blackRookQueensideMoved && 
            board[7][1] == ' ' && board[7][2] == ' ' && board[7][3] == ' ' &&
            !isSquareAttacked(7, 2, 'w') && !isSquareAttacked(7, 3, 'w')) {
          moves[moveCount][0] = 7;
          moves[moveCount][1] = 2;
          moveCount++;
        }
      }
      break;
    }
  }
}

void updateClock() {
  display1.showNumberDecEx(p1Time / 60 * 100 + p1Time % 60, 0x40, true);
  display2.showNumberDecEx(p2Time / 60 * 100 + p2Time % 60, 0x40, true);
}

void displayClockInfo() {
  char line1[17], line2[17];
  sprintf(line1, "You: %02d:%02d", p1Time / 60, p1Time % 60);
  sprintf(line2, "AI: %02d:%02d", p2Time / 60, p2Time % 60);
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

void scrollCredits() {
  String line1 = "    Made by     ";
  String line2 = "      Moutmayen Nafis & Abid Elhan      ";
  lcd.setCursor(0, 0);
  lcd.print(line1);
  for (int pos = 0; pos < (int)line2.length() - 16; pos++) {
    lcd.setCursor(0, 1);
    lcd.print(line2.substring(pos, pos + 16));
    delay(200);
  }
}

void playAIDialogue(const char* text, bool slowScroll) {
  String message = String(text);
  String paddedMessage = "                " + message + "                ";
  int scrollDelay = slowScroll ? 100 : 15;
  int scrollStep = slowScroll ? 1 : 3;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("[SkyeKnet AI]");
  for (int pos = 0; pos < (int)paddedMessage.length() - 16; pos += scrollStep) {
    lcd.setCursor(0, 1);
    lcd.print(paddedMessage.substring(pos, pos + 16));
    delay(scrollDelay);
  }
  delay(300);
  lcd.clear();
}

void displayStaticInfo(const char* top, const char* bottom) {
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 0);
  lcd.print(top);
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print(bottom);
}

int countPieces(char color) {
  int count = 0;
  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 8; c++) {
      char piece = board[r][c];
      if (piece == ' ') continue;
      char pieceColor = (piece >= 'a' && piece <= 'z') ? 'b' : 'w';
      if (pieceColor == color) count++;
    }
  }
  return count;
}

int calculateMaterial() {
  int whiteMaterial = 0, blackMaterial = 0;
  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 8; c++) {
      char piece = board[r][c];
      if (piece == ' ') continue;
      
      int value = 0;
      char p = (piece >= 'a' && piece <= 'z') ? piece - 32 : piece;
      if (p == 'P') value = 1;
      else if (p == 'N' || p == 'B') value = 3;
      else if (p == 'R') value = 5;
      else if (p == 'Q') value = 9;
      
      if (piece >= 'a' && piece <= 'z') blackMaterial += value;
      else whiteMaterial += value;
    }
  }
  return whiteMaterial - blackMaterial;
}
