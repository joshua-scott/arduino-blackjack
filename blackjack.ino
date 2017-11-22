/*

ARDUINO BLACKJACK
TASK REQUIREMENTS:

- On the player's turn, the player can keep his hand as it is (stand) or take more cards from the deck (hit), one at a time, until either the player judges that the hand is strong enough to go up against the dealer's hand and stands, or until it goes over 21, in which case the player immediately loses (busts).
- On the dealer's turn, the dealer takes more cards or stands depending on the value of the hand. Contrary to the player, though, the dealer's action is completely dictated by the rules. The dealer must take more cards if the value of the hand is lower than 17, otherwise the dealer will stand.

- Card values:
  Cards 2-10 are valued at the face value of the card.
  Face cards such as the King, Queen, and Jack are valued at 10 each.
  The Ace card, however, is a special card and which be valued either at 11 or 1.

- You need Arduino, LCD-display and some switches or push buttons to implement this game.
- You should see on the display yours and dealers hand, and Arduino should announce the winner.

Credit to LinhartR from the Arduino forums for the code this was based on (and modified for my needs): https://forum.arduino.cc/index.php?topic=182265.0

 */

#include <Arduino.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(7, 8, 9, 10, 11 , 12);
const byte lcdRowMax = 2;
const byte lcdColMax = 16;
const int twistBtn = 2;
const int stickBtn = 3;

// Create suit symbols character maps.
byte heart[8] = {
    B01010,
    B11111,
    B11111,
    B11111,
    B01110,
    B00100,
    B00000,
    B00000,
};
byte diamond[8] = {
    B00100,
    B01110,
    B11111,
    B11111,
    B01110,
    B00100,
    B00000,
    B00000,
};
byte spade[8] = {
    B00100,
    B01110,
    B11111,
    B11111,
    B00100,
    B01110,
    B00000,
    B00000,
};
byte club[8] = {
    B01110,
    B01110,
    B11111,
    B11111,
    B00100,
    B01110,
    B00000,
    B00000,
};
byte back1[8] = {
    B11111,
    B10110,
    B10101,
    B10110,
    B10101,
    B10110,
    B10000,
    B11111,
};
byte back2[8] = {
    B11111,
    B00001,
    B01111,
    B00101,
    B00101,
    B10101,
    B01001,
    B11111,
};
byte ten[8] = {
    B01000,
    B11000,
    B01000,
    B01010,
    B11101,
    B00101,
    B00101,
    B00010,
};

// Init Deck.
const byte cardIndexMax = 52;
const byte rankIndex = 0;
const byte suitIndex = 1;
byte deck[cardIndexMax][2];
byte cardIndex;

// Init Hand.
const byte handIndexMax = 6;
const byte dealerIndex = 0;
const byte playerIndex = 1;
byte hand[2][handIndexMax][2];
byte playerHandIndex;
byte dealerHandIndex;
boolean dealerHoleCard = true; // Hole Card display state.
byte handTotal[2][2];          // Need two totals per player for Ace duality.

// Setup.
void setup() {
  // Init serial for debugging.
  Serial.begin(9600);

  // Set the internal resistor to be used on the buttons
  pinMode(twistBtn,INPUT_PULLUP);
  pinMode(stickBtn,INPUT_PULLUP);

  // Init card characters.
  lcd.createChar(0, heart);
  lcd.createChar(1, diamond);
  lcd.createChar(2, spade);
  lcd.createChar(3, club);
  lcd.createChar(4, back1);
  lcd.createChar(5, back2);
  lcd.createChar(7, ten);

  // Set up the LCD's number of columns and rows.
  lcd.begin(lcdColMax, lcdRowMax);

  // Create deck.
  cardIndex = 0;
  for (byte suit = 0; suit <= 3; suit++) {
    for (byte rank = 1; rank <= 13; rank++) {
      deck[cardIndex][rankIndex] = rank;
      deck[cardIndex++][suitIndex] = suit;
    }
  }

  // Seed random number generator.
  unsigned long seed = 0, count = 32;
  while (--count) {
    seed = (seed << 1) | (analogRead(0) & 1);
  }
  randomSeed(seed);

  // Display splash.
  displaySplash();
}

// Main loop.
void loop() {
  // Start a new round.
  dealerHandIndex = 0;
  playerHandIndex = 0;
  dealerHoleCard = true;

  // Clear Hand Totals.
  handTotal[dealerIndex][0] = 0;
  handTotal[dealerIndex][1] = 0;
  handTotal[playerIndex][0] = 0;
  handTotal[playerIndex][1] = 0;

  // Deal initial hand.
  dealCard(playerIndex, playerHandIndex++);
  dealCard(dealerIndex, dealerHandIndex++);
  dealCard(playerIndex, playerHandIndex++);
  dealCard(dealerIndex, dealerHandIndex++);
  displayHands();

  // Dealer Blackjack?
  if (handTotal[dealerIndex][1] == 21) {
    // Yes, Display hole card.
    displayHoleCard(true);

    // Give player the bad news.
    lcd.clear();
    lcd.print("Dealer Blackjack!");

    // Player Blackjack?
    lcd.setCursor(0, 1);
    if (handTotal[playerIndex][1] == 21) {
      // Yes, Push.
      lcd.print("You Push!");
    }
    else {
      // No, Loser.
      lcd.print("You Lose.");
    }
  }

  // Player Blackjack?
  else if (handTotal[playerIndex][1] == 21) {
    // Yes, Show hole card.
    displayHoleCard(true);

    // Give player the good news.
    lcd.clear();
    lcd.print("Player Blackjack!");
    lcd.setCursor(0, 1);
    lcd.print("You Win!");
  }

  // Play it out.
  else {
    // Keep going until players reaches card limit, 21, busts or stays.
    int stick = 0;
    while (playerHandIndex < handIndexMax && handTotal[playerIndex][0] < 21 && !stick) {
      lcd.setCursor(14, 0);
      lcd.print("<S");
      lcd.setCursor(14, 1);
      lcd.print(">T");

      // User presses twist
      if (digitalRead(twistBtn) == LOW) {
        // Prevent further presses
        delay(200);
        // Hit player.
        dealCard(playerIndex, playerHandIndex++);
        displayHands();
      } else if (digitalRead(stickBtn) == LOW) {
        // Prevent further presses
        delay(200);
        // Move on
        stick = 1;
      } else {
        Serial.println("Neither buttons are pressed");
      }
    }

    // Player busted?
    if (handTotal[playerIndex][0] > 21) {
      // Yes, Give player the bad news.
      displayHoleCard(true);
      lcd.clear();
      lcd.print("Busted!");
      lcd.setCursor(0, 1);
      lcd.print("You lose!");
    }
    else {
      // Play out dealer's hand.
      displayHoleCard(false);

      // Dealer must stand on all 17's.
      while (dealerHandIndex < handIndexMax && handTotal[dealerIndex][0] < 17 && handTotal[dealerIndex][1] < 17) {
        // Hit dealer.
        dealCard(dealerIndex, dealerHandIndex++);
        displayHands();
        delay(2500);
      }
      delay(1000);

      // Get player total.
      int playerTotal;
      if (handTotal[playerIndex][1] > 21) {
        playerTotal = handTotal[playerIndex][0];
      }
      else {
        playerTotal = handTotal[playerIndex][1];
      }

      // Get dealer total.
      int dealerTotal;
      if (handTotal[dealerIndex][1] > 21) {
        dealerTotal = handTotal[dealerIndex][0];
      }
      else {
        dealerTotal = handTotal[dealerIndex][1];
      }

      // Dealer busted?
      if (dealerTotal > 21) {
        // Yes, Give player the good news.
        lcd.clear();
        lcd.print("Dealer Busts!");
        lcd.setCursor(0, 1);
        lcd.print("You Win!");
      }

      // Push?
      else if (dealerTotal == playerTotal) {
        // Yes, Give player the news.
        lcd.clear();
        lcd.print("You Push!");
      }

      // Highest hand?
      else {
        if (dealerTotal > playerTotal) {
          // Dealer wins.
          lcd.clear();
          lcd.print("Dealer Wins!");
        }
        else {
          // Player wins.
          lcd.clear();
          lcd.print("You Win!");
        }
      }
    }
  }
  delay(2000);
}

// Deal a card.
void dealCard(byte player, byte card) {
  // Any cards left?
  if (cardIndex >= cardIndexMax) {
    // No, Shuffle the deck.
    lcd.clear();
    lcd.print("Shuffling");
    byte cardTemp[2];
    for (byte cardShuffle = 0; cardShuffle < cardIndexMax; cardShuffle++) {
      byte cardRandom = random(0, 51);
      cardTemp[rankIndex] = deck[cardShuffle][rankIndex];
      cardTemp[suitIndex] = deck[cardShuffle][suitIndex];
      deck[cardShuffle][rankIndex] = deck[cardRandom][rankIndex];
      deck[cardShuffle][suitIndex] = deck[cardRandom][suitIndex];
      deck[cardRandom][rankIndex] = cardTemp[rankIndex];
      deck[cardRandom][suitIndex] = cardTemp[suitIndex];
    }
    cardIndex = 0;
    delay(2000);
    displayHands();
  }

  // Deal card.
  hand[player][card][rankIndex] = deck[cardIndex][rankIndex];
  hand[player][card][suitIndex] = deck[cardIndex++][suitIndex];

  // Update hand totals.
  handTotal[player][0] += min(hand[player][card][rankIndex], 10);
  handTotal[player][1] = handTotal[player][0];
  for (byte cardCount = 0; cardCount <= card; cardCount++) {
    // Found an Ace?
    if (hand[player][cardCount][rankIndex] == 1) {
      // Yes
      handTotal[player][1] += 10;
      // Only count one Ace.
      cardCount = card + 1;
    }
  }
  Serial.println("Player #" + String(player) + "\tCard #" + String(card) + "\tCard: " + String(hand[player][card][rankIndex]) + "-" + String(hand[player][card][suitIndex]));
}

// Display hands.
void displayHands() {
  // Display dealer's hand.
  lcd.clear();
  lcd.print("D:");
  displayHand(dealerIndex, dealerHandIndex);

  // Display player's hand.
  lcd.setCursor(0, 1);
  lcd.print("P:");
  displayHand(playerIndex, playerHandIndex);
}

// Display hand.
void displayHand(byte displayHandIndex, byte displayHandIndexMax) {
  // Display cards.
  for (byte card = 0; card < displayHandIndexMax; card++) {
    // Dealer Hole Card?
    if (displayHandIndex == dealerIndex && card == 0 && dealerHoleCard) {
      // Display card back.
      lcd.write((uint8_t)4);
      lcd.write((uint8_t)5);
    }
    else {
      // Display card rank.
      switch (hand[displayHandIndex][card][rankIndex]) {
      case 1:
        lcd.print("A");
        break;
      case 10:
        lcd.write((uint8_t)7);
        break;
      case 11:
        lcd.print("J");
        break;
      case 12:
        lcd.print("Q");
        break;
      case 13:
        lcd.print("K");
        break;
      default:
        lcd.print(hand[displayHandIndex][card][rankIndex]);
      }
      // Display card suit.
      lcd.write((uint8_t)hand[displayHandIndex][card][suitIndex]);
    }
  }
}

// Display Hole Card
void displayHoleCard(boolean pause) {
  delay(2000 * pause);
  dealerHoleCard = false;
  displayHands();
  delay(2000);
}

// Clear LCD single row.
// Leaves cursor at the beginning of the cleared row.
void lcdClearRow(byte row) {
  if (row >= 0 && row < lcdRowMax) {
    lcd.setCursor(0, row);
    for (int x = 0; x < lcdColMax; x++) {
      lcd.print(" ");
    }
    lcd.setCursor(0, row);
  }
}

// Display splash.
void displaySplash() {
  lcd.clear();
  lcd.setCursor(3, 1);
  lcd.write((uint8_t)4);
  lcd.write((uint8_t)5);
  delay(1000);
  lcd.setCursor(7, 1);
  lcd.write((uint8_t)4);
  lcd.write((uint8_t)5);
  delay(1000);
  lcd.setCursor(3, 1);
  lcd.print("J");
  lcd.write((uint8_t)1);
  delay(1000);
  lcd.setCursor(7, 1);
  lcd.print("A");
  lcd.write((uint8_t)2);
  delay(1000);
  lcd.setCursor(0, 0);
  lcd.print("Blackjack!");

  // Logarithmic blink. (Pretty fancy huh?)
  for (byte x = 15; x < 30; x++) {
    if (x % 2) {
      lcd.display();
    }
    else {
      lcd.noDisplay();
    }
    delay(pow(31 - x, 2) + 200);
  }
  delay(1000);
}