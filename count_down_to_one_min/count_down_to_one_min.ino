/*
 * 60-Second Accelerating Countdown
 *
 * This code is for an ACTIVE buzzer (one that beeps with just VCC and GND)
 * It plays an accelerating "beep" rhythm for a 1-minute countdown.
 *
 * Connect Buzzer (+) to Pin 9
 * Connect Buzzer (-) to GND
 */

#define BUZZER_PIN 9

void setup() {
  // Set the buzzer pin as an output
  pinMode(BUZZER_PIN, OUTPUT);
  Serial.begin(9600); // Optional: for seeing text
}

void loop() {
  
  Serial.println("60 Second Countdown Started!");

  // --- Phase 1: Slow ---
  // (30 seconds: 1 beep every 2 seconds = 15 beeps)
  Serial.println("Phase 1: Slow (30s remaining)");
  for (int i = 0; i < 15; i++) {
    beep(100, 1900); // 100ms ON, 1900ms OFF (2000ms total)
  }

  // --- Phase 2: Medium ---
  // (20 seconds: 1 beep per second = 20 beeps)
  Serial.println("Phase 2: Medium (30s remaining)");
  for (int i = 0; i < 20; i++) { 
    beep(100, 900); // 100ms ON, 900ms OFF (1000ms total)
  }

  // --- Phase 3: Fast ---
  // (10 seconds: 2 beeps per second = 20 beeps)
  Serial.println("Phase 3: Fast! (10s remaining)");
  for (int i = 0; i < 20; i++) { 
    beep(100, 400); // 100ms ON, 400ms OFF (500ms total)
  }

  // --- Phase 4: Time's Up! ---
  Serial.println("TIME'S UP!");
  digitalWrite(BUZZER_PIN, HIGH);
  delay(3000); // Long 3-second solid beep
  digitalWrite(BUZZER_PIN, LOW);

  // Wait for 5 seconds before starting the countdown again
  delay(5000);
}

// This is a helper function to play one beep
// 'duration' = how long the beep is ON
// 'p' = how long the beep is OFF after
void beep(int duration, int p) {
  digitalWrite(BUZZER_PIN, HIGH); // Turn buzzer ON
  delay(duration);                // Hold for the note duration
  digitalWrite(BUZZER_PIN, LOW);  // Turn buzzer OFF
  delay(p);                       // Hold for the pause
}
