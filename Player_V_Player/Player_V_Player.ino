/*
 * FIXES APPLIED:
 * 1. ✅ Fixed state sync timing
 * 2. ✅ Fixed LED coordinate system
 * 3. ✅ Fixed board state sync
 * 4. ✅ FIXED CAPTURE LOGIC - pieces now get captured!
 * 5. ✅ FIXED LED VISIBILITY - LEDs now show during move selection
 * 6. ✅ TURN-BASED SYSTEM - White and Black alternate turns
 * 7. ✅ CHECK DETECTION - King cannot move into check, cannot capture protected pieces
 * 8. ✅ CHECKMATE DETECTION - Game ends when checkmate occurs 
 * 9. ✅ CASTLING - Kingside and Queenside castling implemented
 * 10. ✅ EN PASSANT - Pawn diagonal capture implemented

 * ✅ Pawn Promotion needs checking // is okay now 
 // Need to fix and polish turn based system on button press,
  move confirmation has been added

  // Need to implement Reset Mode (Two buttons pressed) added, need to check edge cases
  // Polish Texts ( added )

 
 * Uppercase = White pieces (RNBQKP)
 * Lowercase = Black pieces (rnbqkp)
 * Space ' ' = Empty square
 */

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
bool isAIPlaying = false; // false for 1 v 1 gameplay
 
#include <math.h>
#include <FastLED.h>
#include <TM1637Display.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int MUX_S0 = 4;
const int MUX_S1 = 5;
const int MUX_S2 = 6;
const int MUX_S3 = 7;
const int MUX_EN = 14;

const int MUX1_SIG = 15;
const int MUX2_SIG = 16;
const int MUX3_SIG = 17;
const int MUX4_SIG = 18;


// Custom Pawn Icon
byte pawn[8] = {
  B00000, B00100, B01110, B00100, B00100, B01110, B11111, B00000
};

const int sigPins[4] = {MUX1_SIG, MUX2_SIG, MUX3_SIG, MUX4_SIG};

// --- TM1637 Displays ---
TM1637Display display1(21, 20);
TM1637Display display2(39, 38);

// --- LCD Configuration ---
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- Clock Variables ---
int p1Time = 300, p2Time = 300; // Time in seconds (5 minutes)
bool p1Turn = true;
bool gameStarted = false;
unsigned long lastTick = 0;

// --- Display Update Variables ---
unsigned long lastDisplayUpdate = 0;
const int displayUpdateInterval = 3000; // 3 seconds between display cycles
int displayState = 0; // Tracks which info to show



byte rowPatterns[8] = {
  0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
};


bool moveConfirmedByButton = false;
int pendingFromRow = -1, pendingFromCol = -1;
int pendingToRow = -1, pendingToCol = -1;

// ---------------------------
// Global Variables
// ---------------------------
CRGB leds[LED_COUNT];
const CRGB PROMOTION_COLOR = CRGB(255, 215, 0); // Gold
const CRGB MOVE_COLOR = CRGB(0, 255, 0);        // Green for legal moves
const CRGB CAPTURE_COLOR = CRGB(255, 0, 0);     // Red for captures
const CRGB ORIGIN_COLOR = CRGB(100, 100, 255);  // Blue for picked piece

bool sensorState[8][8];
bool sensorPrev[8][8];

char initialBoard[8][8] = {
  {'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R'},  // row 0 (rank 1)
  {'P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'},  // row 1 (rank 2)
  {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},  // row 2 (rank 3)
  {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},  // row 3 (rank 4)
  {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},  // row 4 (rank 5)
  {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},  // row 5 (rank 6)
  {'p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'},  // row 6 (rank 7)
  {'r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'}   // row 7 (rank 8)
};

char board[8][8];



// --- AI Dialogue Arrays ---
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


// NEW: Game state tracking
bool whiteTurn = true;  // White moves first
bool gameOver = false;
char winner = ' ';      // 'w' or 'b'

// NEW: Castling rights
bool whiteKingMoved = false;
bool whiteRookKingsideMoved = false;
bool whiteRookQueensideMoved = false;
bool blackKingMoved = false;
bool blackRookKingsideMoved = false;
bool blackRookQueensideMoved = false;

// NEW: En passant tracking
int enPassantCol = -1;  // Column where en passant is possible (-1 = none)
int enPassantRow = -1;  // Row where the pawn can be captured

// ---------------------------
// Function Prototypes
// ---------------------------
void readSensors();
void fireworkAnimation();
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
void updateCastlingRights(int row, int col);

// ---------------------------
// HELPER: Get LED index from board coordinates
// ---------------------------
int getPixelIndex(int row, int col) {
  return col * NUM_COLS + (7 - row);
}

// ---------------------------
// HELPER: Display current LED state (for debugging)
// ---------------------------
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

// ---------------------------
// NEW: Check if a square is under attack
// ---------------------------
bool isSquareAttacked(int row, int col, char byColor) {
  // Check if square (row, col) is attacked by pieces of color 'byColor'
  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 8; c++) {
      char piece = board[r][c];
      if (piece == ' ') continue;
      
      char pieceColor = (piece >= 'a' && piece <= 'z') ? 'b' : 'w';
      if (pieceColor != byColor) continue;
      
      // Get possible moves for this piece
      int moveCount = 0;
      int moves[30][2];
      
      // Temporarily get moves (we need raw moves, not filtered by check)
      char pieceType = (piece >= 'a' && piece <= 'z') ? piece - 32 : piece;
      
      // Check if this piece can attack the target square
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

// ---------------------------
// NEW: Check if king is in check
// ---------------------------
bool isKingInCheck(char kingColor) {
  // Find king position
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
  
  if (kingRow == -1) return false; // King not found
  
  char enemyColor = (kingColor == 'w') ? 'b' : 'w';
  return isSquareAttacked(kingRow, kingCol, enemyColor);
}

// ---------------------------
// NEW: Check if move would leave king in check
// ---------------------------
bool wouldMoveLeaveKingInCheck(int fromRow, int fromCol, int toRow, int toCol) {
  // Simulate the move
  char piece = board[fromRow][fromCol];
  char capturedPiece = board[toRow][toCol];
  
  board[toRow][toCol] = piece;
  board[fromRow][fromCol] = ' ';
  
  char pieceColor = (piece >= 'a' && piece <= 'z') ? 'b' : 'w';
  bool inCheck = isKingInCheck(pieceColor);
  
  // Undo the move
  board[fromRow][fromCol] = piece;
  board[toRow][toCol] = capturedPiece;
  
  return inCheck;
}

// ---------------------------
// NEW: Check for checkmate
// ---------------------------
bool isCheckmate(char kingColor) {
  if (!isKingInCheck(kingColor)) return false;
  
  // Try all possible moves for this color
  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 8; c++) {
      char piece = board[r][c];
      if (piece == ' ') continue;
      
      char pieceColor = (piece >= 'a' && piece <= 'z') ? 'b' : 'w';
      if (pieceColor != kingColor) continue;
      
      // Get all possible moves
      int moveCount = 0;
      int moves[30][2];
      getPossibleMoves(r, c, moveCount, moves);
      
      // Check if any move gets us out of check
      for (int i = 0; i < moveCount; i++) {
        if (!wouldMoveLeaveKingInCheck(r, c, moves[i][0], moves[i][1])) {
          return false; // Found a legal move
        }
      }
    }
  }
  
  return true; // No legal moves - checkmate
}

// ---------------------------
// NEW: Update castling rights after move
// ---------------------------
void updateCastlingRights(int row, int col) {
  char piece = board[row][col];
  
  if (piece == 'K') whiteKingMoved = true;
  if (piece == 'k') blackKingMoved = true;
  
  if (piece == 'R' && row == 0 && col == 0) whiteRookQueensideMoved = true;
  if (piece == 'R' && row == 0 && col == 7) whiteRookKingsideMoved = true;
  if (piece == 'r' && row == 7 && col == 0) blackRookQueensideMoved = true;
  if (piece == 'r' && row == 7 && col == 7) blackRookKingsideMoved = true;
}

// ---------------------------
// Test mode: Control board via serial commands
// ---------------------------
void handleSerialCommands() {
  if (!WOKWI_TEST_MODE) return;
  
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    String origCmd = cmd;
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
      cmd.trim();
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
    else if (cmd == "LEDS") {
      showLEDPattern();
    }
    else if (cmd == "HELP") {
      Serial.println("\n=== Wokwi Test Commands ===");
      Serial.println("LIFT e2      - Lift piece from e2");
      Serial.println("PLACE e4     - Place piece on e4");
      Serial.println("MOVE e2 e4   - Complete move");
      Serial.println("BOARD        - Show current board");
      Serial.println("LEDS         - Show LED pattern");
      Serial.println("HELP         - Show this help");
      Serial.println("===========================\n");
    }
  }
}

// ---------------------------
// SETUP
// ---------------------------
void setup() {
  Serial.begin(9600);
  delay(100);
  Wire.begin(8, 9);

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

    // Initialize TM1637 displays
  display1.setBrightness(0x0f);
  display2.setBrightness(0x0f);\


    // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, pawn);

  // 1. Swiping Credits
  scrollCredits(); 


  // 3. Welcome Message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(0);
  lcd.print(" CHESS ENGINE ");
  lcd.setCursor(0, 1);
  lcd.print("   V1.0 READY   ");
  delay(2000);

  // 4. Initial AI Greeting
  if(isAIPlaying) {
    playAIDialogue(aiGreetings[random(0, 2)], true); // true = slow scroll for greeting
  }
  
  // 5. Initialize clock display
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
  
  fireworkAnimation();
  Serial.println("Board ready! System initialized.");
  Serial.println("WHITE's turn to move.");
  Serial.println("Type 'HELP' for available commands.");
}

// ---------------------------
// MAIN LOOP - WITH CHESS RULES
// ---------------------------
void loop() {
  unsigned long currentMillis = millis();
  
  // 1. Handle button presses for clock control AND move confirmation
  if (digitalRead(BTN_P1) == LOW) {
    delay(50); // Debounce
    if (whiteTurn) {
      moveConfirmedByButton = true; // White confirms their move
    }
    if (!gameStarted) {
      gameStarted = true;
      lastTick = millis();
    }
  }

  if (digitalRead(BTN_P2) == LOW) {
    delay(50); // Debounce
    if (!whiteTurn) {
      moveConfirmedByButton = true; // Black confirms their move
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

// NEW: Reset system - both buttons held for 3 seconds
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
    // RESET EVERYTHING
    Serial.println("\n*** RESETTING GAME ***");
    
    // Reset board
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
    
    // Reset game state
    whiteTurn = true;
    gameOver = false;
    winner = ' ';
    gameStarted = false;
    p1Turn = true;
    
    // Reset clocks
    p1Time = 300;
    p2Time = 300;
    updateClock();
    
    // Reset castling rights
    whiteKingMoved = false;
    whiteRookKingsideMoved = false;
    whiteRookQueensideMoved = false;
    blackKingMoved = false;
    blackRookKingsideMoved = false;
    blackRookQueensideMoved = false;
    
    // Reset en passant
    enPassantCol = -1;
    enPassantRow = -1;
    
    // Clear LEDs
    FastLED.clear();
    FastLED.show();
    
    // Reset display
    displayStaticInfo("GAME RESET!", "Ready to Play");
    delay(2000);
    displayStaticInfo("Game Status:", "Press to Start");
    
    Serial.println("Game reset complete. WHITE's turn to move.");
    Serial.println("Please ensure all pieces are in starting positions.");
    
    resetInProgress = false;
    bothButtonsPressedTime = 0;
    
    // Wait for buttons to be released
    while (digitalRead(BTN_P1) == LOW || digitalRead(BTN_P2) == LOW) {
      delay(50);
    }
    delay(200); // Extra debounce
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
        // Show current turn with piece count
        if (!gameOver) {
        int whitePieces = countPieces('w');
        int blackPieces = countPieces('b');
        char line2[17];
        sprintf(line2, "W:%d B:%d", whitePieces, blackPieces);
        displayStaticInfo(whiteTurn ? "> WHITE's TURN" : "> BLACK's TURN", line2);
        break;
        }
      }

      case 1: {
        // Random AI interjection (20% chance) OR show material
        if (!gameOver) {
        if(isAIPlaying && (random(0, 5) == 1)) {
          playAIDialogue(aiRandomLines[random(0, 8)], false); // false = fast scroll
        } else {
          // Show material advantage
          int materialDiff = calculateMaterial();
          char line2[17];
          if (materialDiff > 0) {
            sprintf(line2, "White +%d", materialDiff);
          } else if (materialDiff < 0) {
            sprintf(line2, "Black +%d", -materialDiff);
          } else {
            sprintf(line2, "Equal");
          }
          displayStaticInfo("Material:", line2);
        }
        break;
        }
      }
      
      case 2: {
        // Show time remaining
        if (!gameOver) {
        displayClockInfo();
        break;
        }
      }
      
      case 3: {
        // Show game status
        if (gameOver) {
          // Checkmate happened
          displayStaticInfo("CHECKMATE!", winner == 'w' ? "White Wins!" : "Black Wins!");
        } else if (!gameStarted) {
          displayStaticInfo("Game Status:", "Press to Start");
        } else if (isKingInCheck('w')) {
          displayStaticInfo("WARNING!", "White in Check!");
        } else if (isKingInCheck('b')) {
          displayStaticInfo("WARNING!", "Black in Check!");
        } else if (p1Time == 0) {
          displayStaticInfo("TIME UP!", "Black Wins!");
          gameOver = true;
          winner = 'b';
        } else if (p2Time == 0) {
          displayStaticInfo("TIME UP!", "White Wins!");
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

  // ORIGINAL CHESS GAME LOGIC STARTS HERE
  handleSerialCommands();
  
  if (!WOKWI_TEST_MODE) {
    readSensors();
  }

  if (gameOver) {
    // Game ended - just wait
    delay(100);
    return;
  }

  // Look for piece pickup
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      if (sensorPrev[row][col] && !sensorState[row][col]) {
        char piece = board[row][col];
        
        if (piece == ' ') {
          sensorPrev[row][col] = sensorState[row][col];
          continue;
        }

        // NEW: Check if it's the right player's turn
        char pieceColor = (piece >= 'a' && piece <= 'z') ? 'b' : 'w';
        if ((whiteTurn && pieceColor != 'w') || (!whiteTurn && pieceColor != 'b')) {
          Serial.println("It's not your turn!");
          sensorState[row][col] = true; // Force piece back
          sensorPrev[row][col] = true;
          continue;
        }

        Serial.print("Piece lifted from ");
        Serial.print((char)('a' + col)); 
        Serial.println(row + 1);
        Serial.print("Piece type: ");
        Serial.println(piece);
        
        // NEW: Don't allow moves before game starts
        if (!gameStarted) {
          Serial.println("Game not started! WHITE must press their button to begin.");
          sensorState[row][col] = true; // Force piece back
          sensorPrev[row][col] = true;
          FastLED.clear();
          FastLED.show();
          continue;
        }

        // NEW: Don't allow moves if time ran out
          if (p1Time == 0 || p2Time == 0) {
            Serial.println("Game over - time expired!");
            sensorState[row][col] = true; // Force piece back
            sensorPrev[row][col] = true;
            FastLED.clear();
            FastLED.show();
            if (!gameOver) {
              gameOver = true;
              winner = (p1Time == 0) ? 'b' : 'w';
            }
            continue;
          }

        int moveCount = 0;
        int moves[30][2];
        getPossibleMoves(row, col, moveCount, moves);
        
        // NEW: Filter out moves that would leave king in check
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
        int currentPixelIndex = getPixelIndex(row, col);
        leds[currentPixelIndex] = ORIGIN_COLOR;
        
        for (int i = 0; i < legalMoveCount; i++) {
          int r = legalMoves[i][0]; 
          int c = legalMoves[i][1];
          int movePixelIndex = getPixelIndex(r, c);
          
          if (board[r][c] == ' ') {
            leds[movePixelIndex] = MOVE_COLOR;
            Serial.print("  -> ");
            Serial.print((char)('a' + c));
            Serial.print(r + 1);
            Serial.println(" (move)");
          } else {
            leds[movePixelIndex] = CAPTURE_COLOR;
            Serial.print("  -> ");
            Serial.print((char)('a' + c));
            Serial.print(r + 1);
            Serial.println(" (CAPTURE)");
          }
        }
        FastLED.show();
        
        if (WOKWI_TEST_MODE) {
          showLEDPattern();
        }
        
        int targetRow = -1, targetCol = -1;
        bool piecePlaced = false;
        bool pieceReturnedEarly = false;  // NEW FLAG

        
        unsigned long startWait = millis();
        const unsigned long waitTimeout = 30000;

        while (!piecePlaced && (millis() - startWait < waitTimeout)) {
          handleSerialCommands();
          if (!WOKWI_TEST_MODE) { 
            readSensors(); 
          }

        // Check if piece was returned to origin
        if (sensorState[row][col] && !pieceReturnedEarly) {  // Changed condition
          pieceReturnedEarly = true;
          Serial.println("Piece returned to original square.");
          break;
        }
          
          for (int r2 = 0; r2 < 8; r2++) {
            for (int c2 = 0; c2 < 8; c2++) {
              // Look for NEW piece placement (not at origin)
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

      // NEW: Handle early return FIRST
      if (pieceReturnedEarly) {
        sensorPrev[row][col] = true;
        FastLED.clear();
        FastLED.show();
        continue;  // Allow lifting another piece
      }

        
      if (!piecePlaced) {
        Serial.println("Move timeout - piece not placed!");
        FastLED.clear();
        FastLED.show();
        sensorState[row][col] = true;
        sensorPrev[row][col] = true;
        continue;
      }
        Serial.print("Piece placed at ");
        Serial.print((char)('a' + targetCol));
        Serial.println(targetRow + 1);

        // Validate move first
        if (targetRow == row && targetCol == col) {
          Serial.println("Piece returned to original square.");
          sensorPrev[row][col] = true;
        } 
        else {
          bool legal = false;
          for(int i = 0; i < legalMoveCount; i++) {
            if(legalMoves[i][0] == targetRow && legalMoves[i][1] == targetCol) {
              legal = true;
              break;
            }
          }

          if (legal) {
            // NEW: Wait for button confirmation BEFORE executing move
// NEW: Wait for button confirmation BEFORE executing move
Serial.print(whiteTurn ? "WHITE" : "BLACK");
Serial.println(" - Press your button to confirm move!");

moveConfirmedByButton = false;
unsigned long confirmWait = millis();

while (!moveConfirmedByButton && (millis() - confirmWait < 10000)) {
  // Update clock while waiting for confirmation
  unsigned long currentMillis = millis();
  if (gameStarted && (currentMillis - lastTick >= 1000)) {
    lastTick = currentMillis;
    if (p1Turn && p1Time > 0) p1Time--;
    else if (!p1Turn && p2Time > 0) p2Time--;
    updateClock();
  }
  
  if (digitalRead(whiteTurn ? BTN_P1 : BTN_P2) == LOW) {
    moveConfirmedByButton = true;
    
    // Start game clock on first button press (White's first move)
    if (!gameStarted) {
      gameStarted = true;
      lastTick = millis();
      p1Turn = true;  // Start White's clock
      Serial.println("Game clock started! White's clock running.");
    }
    
    delay(200); // Debounce
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
              goto endLoop;
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
            
            // NEW: Handle castling
            if ((piece == 'K' || piece == 'k') && abs(targetCol - col) == 2) {
              Serial.println("CASTLING!");
              // Move rook too
              if (targetCol == 6) { // Kingside
                int rookRow = (piece == 'K') ? 0 : 7;
                board[rookRow][5] = board[rookRow][7];
                board[rookRow][7] = ' ';
                Serial.println("Kingside castling - please move the rook from h to f");
              } else { // Queenside
                int rookRow = (piece == 'K') ? 0 : 7;
                board[rookRow][3] = board[rookRow][0];
                board[rookRow][0] = ' ';
                Serial.println("Queenside castling - please move the rook from a to d");
              }
            }
            
            // NEW: Handle en passant
            if ((piece == 'P' || piece == 'p') && targetCol != col && board[targetRow][targetCol] == ' ') {
              // En passant capture
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
            
            // NEW: Track en passant possibility
            enPassantCol = -1;
            if ((piece == 'P' || piece == 'p') && abs(targetRow - row) == 2) {
              enPassantCol = col;
              enPassantRow = (row + targetRow) / 2;
            }
            
            // Update castling rights
            updateCastlingRights(row, col);
            
            checkForPromotion(targetRow, targetCol, piece);
            
            sensorPrev[row][col] = false;
            sensorPrev[targetRow][targetCol] = true;
            
            // NEW: Switch turns AND sync with clock
// NEW: Switch turns AND sync with clock
whiteTurn = !whiteTurn;

// Only switch clock after game has started
if (gameStarted) {
  p1Turn = !p1Turn;  // Toggle clock between players
}
            
            // NEW: Check for check/checkmate
            char enemyColor = whiteTurn ? 'w' : 'b';
            if (isKingInCheck(enemyColor)) {
              if (isCheckmate(enemyColor)) {
                Serial.println("\n*** CHECKMATE! ***");
                Serial.print(whiteTurn ? "BLACK" : "WHITE");
                Serial.println(" WINS!");
                gameOver = true;
                winner = whiteTurn ? 'b' : 'w';
                
                // Victory animation
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
            
            if (!gameOver) {
              Serial.print(whiteTurn ? "WHITE" : "BLACK");
              Serial.println("'s turn to move.");
            }
            
          } else {
            Serial.println("Illegal move! Reverting state...");
            sensorState[row][col] = true;
            sensorState[targetRow][targetCol] = (board[targetRow][targetCol] != ' ');
            sensorPrev[row][col] = true;
            sensorPrev[targetRow][targetCol] = (board[targetRow][targetCol] != ' ');
          }
        }
        
        FastLED.clear();
        FastLED.show();
        
        goto endLoop;
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

// ---------------------------
// FUNCTIONS
// ---------------------------

void readSensors() {
  if(WOKWI_TEST_MODE) return;

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
  float centerX = 3.5; 
  float centerY = 3.5;
  for (float radius = 0; radius < 6; radius += 0.5) {
    FastLED.clear();
    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        float dist = sqrt(pow(col - centerX, 2) + pow(row - centerY, 2));
        if (fabs(dist - radius) < 0.5) {
          leds[getPixelIndex(row, col)] = CRGB::White;
        }
      }
    }
    FastLED.show();
    delay(100);
  }
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
    Serial.print(isWhite ? "White" : "Black");
    Serial.print(" pawn promoted to Queen at ");
    Serial.print((char)('a' + targetCol));
    Serial.println(isWhite ? "8" : "1");
    
    promotionAnimation(targetCol);
    board[targetRow][targetCol] = isWhite ? 'Q' : 'q';
    
    Serial.println("Please replace the pawn with a queen piece");
    int pixelIndex = getPixelIndex(targetRow, targetCol);

    while (sensorState[targetRow][targetCol]) {
      leds[pixelIndex] = PROMOTION_COLOR;
      FastLED.show();
      delay(250);
      leds[pixelIndex] = CRGB::Black;
      FastLED.show();
      delay(250);
      if (WOKWI_TEST_MODE) {
        handleSerialCommands();  // ← ADD THIS
      } else {
        readSensors();
      }
    }

    Serial.println("Pawn removed, please place a queen");

    while (!sensorState[targetRow][targetCol]) {
      leds[pixelIndex] = PROMOTION_COLOR;
      FastLED.show();
      delay(250);
      leds[pixelIndex] = CRGB::Black;
      FastLED.show();
      delay(250);
      if (WOKWI_TEST_MODE) {
        handleSerialCommands();  // ← ADD THIS
      } else {
        readSensors();
      }
    }
    
    Serial.println("Queen placed, promotion complete");
    
    for (int i = 0; i < 3; i++) {
      leds[pixelIndex] = PROMOTION_COLOR;
      FastLED.show();
      delay(100);
      leds[pixelIndex] = CRGB::Black;
      FastLED.show();
      delay(100);
    }
  }
}

void getPossibleMoves(int row, int col, int &moveCount, int moves[][2]) {
  moveCount = 0;
  char piece = board[row][col];
  
  if (piece == ' ') {
    return;
  }
  
  char pieceColor = (piece >= 'a' && piece <= 'z') ? 'b' : 'w';
  char pieceType = (piece >= 'a' && piece <= 'z') ? piece - 32 : piece;

  switch(pieceType) {
    case 'P': { // Pawn
      int direction = (pieceColor == 'w') ? 1 : -1;
      
      // One square forward
      if (row + direction >= 0 && row + direction < 8 && board[row + direction][col] == ' ') {
        moves[moveCount][0] = row + direction;
        moves[moveCount][1] = col;
        moveCount++;
        
        // Two squares from start
        if ((pieceColor == 'w' && row == 1) || (pieceColor == 'b' && row == 6)) {
          if (board[row + 2*direction][col] == ' ') {
            moves[moveCount][0] = row + 2*direction;
            moves[moveCount][1] = col;
            moveCount++;
          }
        }
      }
      
      // Diagonal captures
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
          
          // NEW: En passant
          if (newCol == enPassantCol && newRow == enPassantRow) {
            moves[moveCount][0] = newRow;
            moves[moveCount][1] = newCol;
            moveCount++;
          }
        }
      }
      break;
    }
    
    case 'R': { // Rook
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
    
    case 'N': { // Knight
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
    
    case 'B': { // Bishop
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
    
    case 'Q': { // Queen
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
    
    case 'K': { // King
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
      
      // NEW: Castling
      if (pieceColor == 'w' && !whiteKingMoved && !isKingInCheck('w')) {
        // Kingside
        if (!whiteRookKingsideMoved && 
            board[0][5] == ' ' && board[0][6] == ' ' &&
            !isSquareAttacked(0, 5, 'b') && !isSquareAttacked(0, 6, 'b')) {
          moves[moveCount][0] = 0;
          moves[moveCount][1] = 6;
          moveCount++;
        }
        // Queenside
        if (!whiteRookQueensideMoved && 
            board[0][1] == ' ' && board[0][2] == ' ' && board[0][3] == ' ' &&
            !isSquareAttacked(0, 2, 'b') && !isSquareAttacked(0, 3, 'b')) {
          moves[moveCount][0] = 0;
          moves[moveCount][1] = 2;
          moveCount++;
        }
      }
      
      if (pieceColor == 'b' && !blackKingMoved && !isKingInCheck('b')) {
        // Kingside
        if (!blackRookKingsideMoved && 
            board[7][5] == ' ' && board[7][6] == ' ' &&
            !isSquareAttacked(7, 5, 'w') && !isSquareAttacked(7, 6, 'w')) {
          moves[moveCount][0] = 7;
          moves[moveCount][1] = 6;
          moveCount++;
        }
        // Queenside
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
// --- HELPER FUNCTIONS ---

// Update TM1637 clock displays
void updateClock() {
  display1.showNumberDecEx(p1Time / 60 * 100 + p1Time % 60, 0x40, true);
  display2.showNumberDecEx(p2Time / 60 * 100 + p2Time % 60, 0x40, true);
}

// Display clock times on LCD
void displayClockInfo() {
  char line1[17], line2[17];
  sprintf(line1, "White: %02d:%02d", p1Time / 60, p1Time % 60);
  sprintf(line2, "Black: %02d:%02d", p2Time / 60, p2Time % 60);
  
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 0);
  lcd.print(line1);
  
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

// Optimized scroll to prevent blinking
void scrollCredits() {
  String line1 = "    Made by     ";
  String line2 = "      Moutmayen Nafis & Abid Elhan      "; 
  
  lcd.setCursor(0, 0);
  lcd.print(line1);

  for (int pos = 0; pos < line2.length() - 16; pos++) {
    lcd.setCursor(0, 1);
    lcd.print(line2.substring(pos, pos + 16));
    delay(200); 
  }
}

// Function for AI to talk - with speed control
void playAIDialogue(const char* text, bool slowScroll) {
  String message = String(text);
  String paddedMessage = "                " + message + "                ";
  
  int scrollDelay = slowScroll ? 100 : 15; // Adjust speed
  int scrollStep = slowScroll ? 1 : 3;     // Skip characters for fast scroll!
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("[SkyeKnet AI]");
  
  for (int pos = 0; pos < paddedMessage.length() - 16; pos += scrollStep) {
    lcd.setCursor(0, 1);
    lcd.print(paddedMessage.substring(pos, pos + 16));
    delay(scrollDelay);
  }
  delay(300); // Brief pause at end
  lcd.clear();
}
// Function to update screen without unnecessary clears
void displayStaticInfo(const char* top, const char* bottom) {
  lcd.setCursor(0, 0);
  lcd.print("                "); // Clear line 1 manually
  lcd.setCursor(0, 0);
  lcd.print(top);
  
  lcd.setCursor(0, 1);
  lcd.print("                "); // Clear line 2 manually
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
