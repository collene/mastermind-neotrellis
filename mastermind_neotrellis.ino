#include "Adafruit_NeoTrellis.h"

#define RED_BUTTON_PIN 3
#define GREEN_BUTTON_PIN 2

#define NUM_ROWS 8    // Y
#define NUM_COLS 8    // X
#define TOP_LEFT_ADDRESS 0x2F
#define TOP_RIGHT_ADDRESS 0x30
#define BOTTOM_LEFT_ADDRESS 0x31
#define BOTTOM_RIGHT_ADDRESS 0x32

Adafruit_NeoTrellis TOP_LEFT = Adafruit_NeoTrellis(TOP_LEFT_ADDRESS);
Adafruit_NeoTrellis TOP_RIGHT = Adafruit_NeoTrellis(TOP_RIGHT_ADDRESS);
Adafruit_NeoTrellis BOTTOM_LEFT = Adafruit_NeoTrellis(BOTTOM_LEFT_ADDRESS);
Adafruit_NeoTrellis BOTTOM_RIGHT = Adafruit_NeoTrellis(BOTTOM_RIGHT_ADDRESS);

Adafruit_NeoTrellis trellisArray[NUM_ROWS / 4][NUM_COLS / 4] = {
  {TOP_LEFT   , TOP_RIGHT   },
  {BOTTOM_LEFT, BOTTOM_RIGHT}
};

Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis *)trellisArray, NUM_ROWS / 4, NUM_COLS / 4);

struct guess {
  int choices[4];
  int hints[4];
};

int numberGuesses = 0;
#define MAX_NUMBER_GUESSES NUM_ROWS
#define NUMBER_GUESS_CHOICES (NUM_COLS / 2)
guess guesses[MAX_NUMBER_GUESSES];

uint32_t RED = seesaw_NeoPixel::Color(16, 0, 0);
uint32_t GREEN = seesaw_NeoPixel::Color(0, 16, 0);
uint32_t BLUE = seesaw_NeoPixel::Color(0, 0, 16);
uint32_t ORANGE = seesaw_NeoPixel::Color(16, 6, 0);
uint32_t YELLOW = seesaw_NeoPixel::Color(16, 16, 0);
uint32_t MAGENTA = seesaw_NeoPixel::Color(16, 0, 16);
uint32_t CYAN = seesaw_NeoPixel::Color(0, 16, 16);
uint32_t WHITE = seesaw_NeoPixel::Color(16, 16, 16);
uint32_t CLEAR = seesaw_NeoPixel::Color(0, 0, 0);

#define NO_PLAY 0
#define WRONG_COLOR 1
#define RIGHT_COLOR_RIGHT_SPOT 2
#define RIGHT_COLOR_WRONG_SPOT 3
#define NOT_FOUND -1
#define DEFAULT_SELECTED_INDEX 1
#define CLEAR_SELECTED_INDEX 0
#define NUMBER_CHOICES_COLORS 9
const uint32_t choicesColors[NUMBER_CHOICES_COLORS] = {CLEAR, RED, GREEN, BLUE, ORANGE, YELLOW, MAGENTA, CYAN, WHITE};
const uint32_t hintsColors[4] = {CLEAR, CLEAR, GREEN, BLUE};

int computerColors[NUMBER_GUESS_CHOICES];

TrellisCallback trellisButtonPressed(keyEvent event) {
  int buttonNumber = event.bit.NUM;   

  if(event.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
    int row = buttonNumber / NUM_COLS;
    int col = buttonNumber - (row * NUM_COLS);
    if(isValidButtonPress(row, col)) {    // only change the current guess's selections      
      guesses[row].choices[col] = changeSelectedColor(row, col);
      updateTrellis();
    }    
  }
  return 0;
}
bool isValidButtonPress(int row, int col) {
  return row == numberGuesses && col < NUMBER_GUESS_CHOICES;
}
int changeSelectedColor(int row, int col) {
  int newColor = guesses[row].choices[col] + 1;
  if(newColor >= NUMBER_CHOICES_COLORS) {
    newColor = CLEAR_SELECTED_INDEX;
  }
  return newColor;
}

void setup() {
  Serial.println("Setting up");
  Serial.begin(9600);
  randomSeed(analogRead(0));

  Serial.println("Starting trellis...");
  if(!trellis.begin()) {
    Serial.println("Failed to start trellis.  Can not continue");
    while(true);
  }
  for(int i = 0; i < NUM_ROWS; i++) {
    for(int j = 0; j < NUM_COLS; j++) {
      trellis.activateKey(j, i, SEESAW_KEYPAD_EDGE_RISING, true);
      trellis.registerCallback(j, i, trellisButtonPressed);      
    }
  }

  Serial.println("Registering buttons");
  pinMode(RED_BUTTON_PIN, INPUT_PULLUP);
  pinMode(GREEN_BUTTON_PIN, INPUT_PULLUP);

  Serial.println("Setup complete!");
  startGame();  
}

void clearGuesses() {
  numberGuesses = 0;
  for(int i = 0; i < MAX_NUMBER_GUESSES; i++) {
    for(int j = 0; j < NUMBER_GUESS_CHOICES; j++) {
      guesses[i].choices[j] = (i == 0) ? DEFAULT_SELECTED_INDEX : CLEAR_SELECTED_INDEX;    // start the game out with the first line all set to RED
      guesses[i].hints[j] = NO_PLAY;
    }    
  }  
}
void startGame() {
  Serial.println("Starting game!");
  clearGuesses();
  Serial.println("Selecting random colors...(no peeking!)");
  for(int j = 0; j < NUMBER_GUESS_CHOICES; j++) {
    computerColors[j] = random(DEFAULT_SELECTED_INDEX, NUMBER_CHOICES_COLORS);
  }
  updateTrellis();
  Serial.println("Choose your first guess!");
}
void updateTrellis() {  
  updateTrellisWithChoicesOnLastLine(false);
}
void updateTrellisWithChoicesOnLastLine(bool useChoicesForLastLine) {  
  for(int i = 0; i < MAX_NUMBER_GUESSES; i++) {
    for(int j = 0; j < NUMBER_GUESS_CHOICES; j++) {
      trellis.setPixelColor(j, i, choicesColors[guesses[i].choices[j]]);     
      if(useChoicesForLastLine && (i == (MAX_NUMBER_GUESSES - 1))) {
        trellis.setPixelColor(j + NUMBER_GUESS_CHOICES, i, choicesColors[guesses[i].hints[j]]); 
      } else {
        trellis.setPixelColor(j + NUMBER_GUESS_CHOICES, i, hintsColors[guesses[i].hints[j]]);  
      }      
    }
  }
  trellis.show();
  delay(50);      
}

void loop() {
  trellis.read();
  if(digitalRead(RED_BUTTON_PIN) == LOW) {
    while(digitalRead(RED_BUTTON_PIN) == LOW) { 
      // do nothing while button is still being pressed
    }
    clearChoices();
  } else if(digitalRead(GREEN_BUTTON_PIN) == LOW) {
    while(digitalRead(GREEN_BUTTON_PIN) == LOW) {
      // do nothing while button is still being pressed
    }
    confirmChoices();
  }  
  delay(20);
}

void clearChoices() {
  // default to previous guess
  Serial.println("Red button pressed!  Clearing current guess");
  for(int j = 0; j < NUMBER_GUESS_CHOICES; j++) {    
    guesses[numberGuesses].choices[j] = (numberGuesses == 0) ? DEFAULT_SELECTED_INDEX : guesses[numberGuesses - 1].choices[j];
  }
  updateTrellis();  
}

bool isValidSelection(guess guessToCheck) {
  for(int j = 0; j < NUMBER_GUESS_CHOICES; j++) {
    if(guessToCheck.choices[j] == CLEAR_SELECTED_INDEX) {
      return false;
    }
  }
  return true;
}
int getExistingGuessWithSelection(guess guessToCheck) {  
  for(int i = 0; i < numberGuesses; i++) {    
    int count = 0;
    for(int j = 0; j < NUMBER_GUESS_CHOICES; j++) {
      if(guessToCheck.choices[j] == guesses[i].choices[j]) {
        count++;
      }
    }
    if(count == NUMBER_GUESS_CHOICES) {
      return i;
    }
  }
  return NOT_FOUND;
}
void blinkGuess(int guessToBlink, int numBlinks, bool blinkChoice, bool blinkHint) {  
  for(int i = 0; i < numBlinks * 2; i++) {    // multiply by 2 to turn off then back on
    for(int j = 0; j < NUMBER_GUESS_CHOICES; j++) {
      if(i % 2 == 0) {
        if(blinkChoice) {
          trellis.setPixelColor(j, guessToBlink, CLEAR);          
        }
        if(blinkHint) {
          trellis.setPixelColor(j + NUMBER_GUESS_CHOICES, guessToBlink, CLEAR);            
        }
        
      } else {        
        if(blinkChoice) {
          trellis.setPixelColor(j, guessToBlink, choicesColors[guesses[guessToBlink].choices[j]]);                      
        }
        if(blinkHint) {
          trellis.setPixelColor(j + NUMBER_GUESS_CHOICES, guessToBlink, choicesColors[guesses[guessToBlink].hints[j]]);  
        }        
      }      
    }
    trellis.show();
    delay(500);
  }
}
void confirmChoices() {
  Serial.println("Green button pressed!  Confirming current guess");
  /*Serial.print("Guess #");
  Serial.println(numberGuesses);*/
  if(!isValidSelection(guesses[numberGuesses])) {
    Serial.println("Please select a color for each column!");
    return;
  }
  int existingGuess = getExistingGuessWithSelection(guesses[numberGuesses]);  
  if(existingGuess != NOT_FOUND) {
    Serial.println("You've already played that selection before!");
    blinkGuess(existingGuess, 4, true, false);
    return;
  }
  updateHints();   
  
  bool isWin = checkLastGuess();  
  numberGuesses ++; 
  
  if(isWin) {
    gameOver(true);
  } else if(numberGuesses >= MAX_NUMBER_GUESSES) {
    gameOver(false);
  } else {
    for(int j = 0; j < NUMBER_GUESS_CHOICES; j++) {
      guesses[numberGuesses].choices[j] = guesses[numberGuesses - 1].choices[j];
    }
  
    updateTrellis();
    Serial.println("Enter your next guess!");  
  }
}

void updateHints() {  
  bool correctGuess[NUMBER_GUESS_CHOICES];
  bool usedGuess[NUMBER_GUESS_CHOICES];
  int currentHintIndex = 0;

  for(int i = 0; i < NUMBER_GUESS_CHOICES; i++) {
    correctGuess[i] = false;
    usedGuess[i] = false;
  }

  // find all the "correct" guesses
  for(int i = 0; i < NUMBER_GUESS_CHOICES; i++) {
    /*Serial.print("current selection: ");
    Serial.print(guesses[numberGuesses].choices[i]);
    Serial.print(", compared to computer selection: ");
    Serial.println(computerColors[i]);*/
    if(guesses[numberGuesses].choices[i] == computerColors[i]) {
      Serial.println("One answer is the right color and right spot!");
      guesses[numberGuesses].hints[currentHintIndex] = RIGHT_COLOR_RIGHT_SPOT;
      correctGuess[i] = true;
      currentHintIndex++;
    }
  }

  // now find all the "correct color, wrong place" guesses
  for(int i = 0; i < NUMBER_GUESS_CHOICES; i++) {
    if(!correctGuess[i]) {
      // take the computer's color and see if we can find the color in the guess that isn't used and isn't a correct guess already
      for(int j = 0; j < NUMBER_GUESS_CHOICES; j++) {
        if(j != i && !usedGuess[j] && !correctGuess[j]) { // don't use ourselves, or used guesses or correct guesses
          if(computerColors[i] == guesses[numberGuesses].choices[j]) {
            Serial.println("One answer is the right color but wrong spot!");
            guesses[numberGuesses].hints[currentHintIndex] = RIGHT_COLOR_WRONG_SPOT;
            usedGuess[j] = true;
            currentHintIndex++;
            break;
          }
        }
      }
    }
  }
}
bool checkLastGuess() {
  for(int j = 0; j < NUMBER_GUESS_CHOICES; j++) {
    if(guesses[numberGuesses].choices[j] != computerColors[j]) {
      return false;
    }
  }
  return true;
}

void gameOver(bool isWin) {  
  Serial.println("Game is over!");
  if(isWin) {
    Serial.println("You won!");
    updateTrellis();    
    delay(500);
    blinkGuess(numberGuesses - 1, 9, true, true);
  } else {
    Serial.println("Sorry, you lost :(");
    // update the trellis to show the "correct" answer on the last hint
    for(int i = 0; i < NUMBER_GUESS_CHOICES; i++) {
      guesses[MAX_NUMBER_GUESSES - 1].hints[i] = computerColors[i];      
    }
    updateTrellisWithChoicesOnLastLine(true);
    delay(500);
    blinkGuess(MAX_NUMBER_GUESSES - 1, 9, false, true);
  }  
  startGame();
}

