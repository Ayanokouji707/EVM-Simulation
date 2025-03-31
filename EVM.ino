#include <Keypad.h>
#include <WiFi.h>
#include <WebServer.h>

// WiFi credentials
const char* ssid = "OnePlus Nord Lite 5g";
const char* password = "M1234567";

// Web server
WebServer server(80);

// Button pins
#define BUTTON1_PIN 18  // GPIO pin for Button A
#define BUTTON2_PIN 19  // GPIO pin for Button B
#define BUTTON3_PIN 21  // GPIO pin for Button C
#define RESET_PIN   22  // GPIO pin for Reset Button

#define KEY_ROW1_PIN 25  // GPIO pin for Keypad Row 1
#define KEY_ROW2_PIN 26  // GPIO pin for Keypad Row 2
#define KEY_ROW3_PIN 27  // GPIO pin for Keypad Row 3
#define KEY_ROW4_PIN 32  // GPIO pin for Keypad Row 4
#define KEY_COL1_PIN 33  // GPIO pin for Keypad Column 1
#define KEY_COL2_PIN 12  // GPIO pin for Keypad Column 2
#define KEY_COL3_PIN 13  // GPIO pin for Keypad Column 3
#define KEY_COL4_PIN 14  // GPIO pin for Keypad Column 4

// Counters and flags for buttons
volatile int button1PressCount = 0;
volatile int button2PressCount = 0;
volatile int button3PressCount = 0;

volatile bool button1Pressed = false;
volatile bool button2Pressed = false;
volatile bool button3Pressed = false;
volatile bool resetPressed = false;
volatile bool resetEnabled = false;  // Flag for enabling reset after password is entered

unsigned long lastPressTime1 = 0;  // Last press time for Button A
unsigned long lastPressTime2 = 0;  // Last press time for Button B
unsigned long lastPressTime3 = 0;  // Last press time for Button C
unsigned long lastResetTime = 0;   // Last press time for Reset Button
const unsigned long debounceDelay = 1000;  // 1-second debounce

// Keypad setup
const byte ROWS = 4;  // Four rows
const byte COLS = 4;  // Four columns
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[4] = {KEY_ROW1_PIN, KEY_ROW2_PIN, KEY_ROW3_PIN, KEY_ROW4_PIN};
byte colPins[4] = {KEY_COL1_PIN, KEY_COL2_PIN, KEY_COL3_PIN, KEY_COL4_PIN};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Password setup
String adminPassword = "6789";         // Predefined admin password for reset
String displayPassword = "1234";      // Predefined password for displaying counts
String inputPassword = "";             // Stores user input
const int maxPasswordLength = 4;       // Maximum password length

// User authentication setup
struct User {
  String id;
  String password;
  bool hasVoted;
};

User users[10] = {
  {"1001", "pass1", false},
  {"1002", "pass2", false},
  {"1003", "pass3", false},
  {"1004", "pass4", false},
  {"1005", "pass5", false},
  {"1006", "pass6", false},
  {"1007", "pass7", false},
  {"1008", "pass8", false},
  {"1009", "pass9", false},
  {"1010", "pass10", false}
};

int authenticatedUserIndex = -1;

// HTML generator for the webpage
String generateLoginPage() {
  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head><title>ESP32 Voting System</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 0; padding: 0; display: flex; justify-content: center; align-items: center; height: 100vh; background: linear-gradient(to bottom, rgba(34, 193, 195, 0.7), rgba(253, 187, 45, 0.7)), url('https://source.unsplash.com/1600x900/?technology,abstract'); background-size: cover; background-position: center; }";
  html += ".login-container { background: rgba(255, 255, 255, 0.9); padding: 40px; border-radius: 20px; box-shadow: 0 8px 20px rgba(0, 0, 0, 0.3); width: 90%; max-width: 400px; text-align: center; transition: transform 0.3s; }";
  html += ".login-container:hover { transform: scale(1.05); }";
  html += ".login-container h1 { margin-bottom: 20px; color: #444; font-size: 28px; background: linear-gradient(to right, #22c1c3, #fdbb2d); -webkit-background-clip: text; color: transparent; }";
  html += "label { font-size: 16px; color: #666; display: block; margin-top: 15px; text-align: left; }";
  html += "input { width: 100%; padding: 12px; margin-top: 5px; border: 1px solid #ddd; border-radius: 8px; font-size: 16px; box-shadow: inset 0 1px 3px rgba(0, 0, 0, 0.1); }";
  html += "input:focus { border-color: #22c1c3; outline: none; box-shadow: 0 0 5px rgba(34, 193, 195, 0.5); }";
  html += "button { background-color: #22c1c3; color: #fff; border: none; padding: 12px 25px; font-size: 18px; border-radius: 8px; cursor: pointer; margin-top: 20px; width: 100%; box-shadow: 0 4px 10px rgba(0, 0, 0, 0.2); transition: background-color 0.3s, transform 0.3s; }";
  html += "button:hover { background-color: #1b9a9c; transform: translateY(-3px); }";
  html += "a { display: block; margin-top: 15px; color: #22c1c3; text-decoration: none; font-size: 14px; }";
  html += "a:hover { text-decoration: underline; }";
  html += "</style></head>";
  html += "<body>";
  html += "<div class='login-container'>";
  html += "<h1>Login to Vote</h1>";
  html += "<form action='/login' method='POST'>";
  html += "<label for='id'>User ID</label>";
  html += "<input type='text' id='id' name='id' placeholder='Enter your User ID' required>";
  html += "<label for='password'>Password</label>";
  html += "<input type='password' id='password' name='password' placeholder='Enter your Password' required>";
  html += "<button type='submit'>Login</button>";
  html += "</form>";
  html += "<a href='/forgot-password'>Forgot Password?</a>";
  html += "</div>";
  html += "</body>";
  html += "</html>";
  return html;
}




String generateVotingPage() {
  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head><title>ESP32 Voting System</title>";
  html += "<style>";
  html += "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 0; padding: 0; display: flex; flex-direction: column; justify-content: center; align-items: center; height: 100vh; background: linear-gradient(135deg, #a1c4fd, #c2e9fb); color: #333; text-align: center; }";  // Light gradient background
  html += "h1 { font-size: 40px; margin-bottom: 20px; text-shadow: 2px 2px 6px rgba(0, 0, 0, 0.2); font-weight: bold; }";
  html += "p { font-size: 22px; margin-bottom: 30px; text-shadow: 1px 1px 3px rgba(0, 0, 0, 0.3); font-weight: 300; }";
  
  // Button styling (only letters A, B, C on button)
  html += "button { background: linear-gradient(135deg, #ffb6b9, #ff6a00); color: #fff; border: none; padding: 20px 40px; font-size: 30px; font-weight: bold; border-radius: 50%; cursor: pointer; box-shadow: 0 6px 15px rgba(0, 0, 0, 0.2); transition: transform 0.3s, box-shadow 0.3s, background 0.5s ease; display: flex; align-items: center; justify-content: center; width: 120px; height: 120px; margin-bottom: 20px; }";
  html += "button:hover { transform: translateY(-5px); box-shadow: 0 10px 20px rgba(0, 0, 0, 0.3); background: linear-gradient(135deg, #ff6a00, #ffb6b9); }";
  html += "button:active { transform: translateY(3px); box-shadow: 0 2px 5px rgba(0, 0, 0, 0.2); }";
  
  // Candidate name labels under buttons
  html += ".candidate-label { font-size: 18px; font-weight: 500; color: #333; text-transform: uppercase; margin-top: 10px; letter-spacing: 1px; text-shadow: 1px 1px 3px rgba(0, 0, 0, 0.2); }";

  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<h1>Welcome to ESP32 Voting System</h1>";
  html += "<p>Make your vote count by selecting your choice below:</p>";
  
  // Buttons with only the letters (A, B, C)
  html += "<button onclick='sendRequest(\"/voteA\")'>A</button>";
  html += "<div class='candidate-label'>Candidate A</div>";
  
  html += "<button onclick='sendRequest(\"/voteB\")'>B</button>";
  html += "<div class='candidate-label'>Candidate B</div>";
  
  html += "<button onclick='sendRequest(\"/voteC\")'>C</button>";
  html += "<div class='candidate-label'>Candidate C</div>";

  html += "<script>";
  html += "function sendRequest(path) {";
  html += "  fetch(path).then(response => response.text()).then(data => {";
  html += "    document.body.innerHTML = data;";
  html += "  });";
  html += "}";
  html += "</script>";
  html += "</body>";
  html += "</html>";
  return html;
}

void handleRoot() {
  server.send(200, "text/html", generateLoginPage());
}

void handleLogin() {
  if (server.hasArg("id") && server.hasArg("password")) {
    String id = server.arg("id");
    String password = server.arg("password");
    for (int i = 0; i < 10; i++) {
      if (users[i].id == id && users[i].password == password) {
        if (!users[i].hasVoted) {
          authenticatedUserIndex = i;
          server.send(200, "text/html", generateVotingPage());
          return;
        } else {
          server.send(200, "text/plain", "You have already voted.");
          return;
        }
      }
    }
    server.send(200, "text/plain", "Invalid credentials.");
  } else {
    server.send(200, "text/plain", "Missing credentials.");
  }
}

void handleVote(String choice) {
  if (authenticatedUserIndex != -1 && !users[authenticatedUserIndex].hasVoted) {
    if (choice == "A") button1PressCount++;
    else if (choice == "B") button2PressCount++;
    else if (choice == "C") button3PressCount++;

    users[authenticatedUserIndex].hasVoted = true;
    authenticatedUserIndex = -1; // Reset authentication
    server.send(200, "text/plain", "Your vote for " + choice + " has been recorded. Thank you for voting!\nPlease refresh the page to login again.");
  } else {
    server.send(200, "text/plain", "You have already voted or are not authenticated.");
  }
}

void handleVoteA() {
  handleVote("A");
}

void handleVoteB() {
  handleVote("B");
}

void handleVoteC() {
  handleVote("C");
}


void IRAM_ATTR handleButton1Press() {
  unsigned long currentTime = millis();
  if (currentTime - lastPressTime1 > debounceDelay) {
    button1PressCount++;
    button1Pressed = true;
    lastPressTime1 = currentTime;
  }
}

void IRAM_ATTR handleButton2Press() {
  unsigned long currentTime = millis();
  if (currentTime - lastPressTime2 > debounceDelay) {
    button2PressCount++;
    button2Pressed = true;
    lastPressTime2 = currentTime;
  }
}

void IRAM_ATTR handleButton3Press() {
  unsigned long currentTime = millis();
  if (currentTime - lastPressTime3 > debounceDelay) {
    button3PressCount++;
    button3Pressed = true;
    lastPressTime3 = currentTime;
  }
}

void IRAM_ATTR handleResetPress() {
  unsigned long currentTime = millis();
  if (currentTime - lastResetTime > debounceDelay) {
    if (resetEnabled) {  // Check if reset is enabled
      button1PressCount = 0;
      button2PressCount = 0;
      button3PressCount = 0;
      Serial.print("\nA: ");
      Serial.print(button1PressCount);
      Serial.print(" B: ");
      Serial.print(button2PressCount);
      Serial.print(" C: ");
      Serial.println(button3PressCount);
      resetEnabled = false;  // Disable reset after execution
    } else {
      Serial.println("Reset not enabled.");
    }
    resetPressed = true;
    lastResetTime = currentTime;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("System Initialized");

  // Configure button pins as input with pull-up resistors
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);
  pinMode(RESET_PIN, INPUT_PULLUP);

  pinMode(KEY_ROW1_PIN, OUTPUT);
  pinMode(KEY_ROW2_PIN, OUTPUT);
  pinMode(KEY_ROW3_PIN, OUTPUT);
  pinMode(KEY_ROW4_PIN, OUTPUT);

  // Initialize keypad column pins as INPUT with pull-up resistors
  pinMode(KEY_COL1_PIN, INPUT_PULLUP);
  pinMode(KEY_COL2_PIN, INPUT_PULLUP);
  pinMode(KEY_COL3_PIN, INPUT_PULLUP);
  pinMode(KEY_COL4_PIN, INPUT_PULLUP);

  // Attach interrupts for button presses
  attachInterrupt(digitalPinToInterrupt(BUTTON1_PIN), handleButton1Press, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON2_PIN), handleButton2Press, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON3_PIN), handleButton3Press, FALLING);
  attachInterrupt(digitalPinToInterrupt(RESET_PIN), handleResetPress, FALLING);

   // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

server.on("/", handleRoot);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/voteA", handleVoteA);
  server.on("/voteB", handleVoteB);
  server.on("/voteC", handleVoteC);

  server.begin();
  Serial.println("Web server started!");
}

void loop() {
    server.handleClient();
  // Keypad input
  char key = keypad.getKey();
  if (key) {
    // Check if the entered password is correct
    if (key == '#') {  // Submit password
      if (inputPassword == adminPassword) {
        // Admin password (6789) entered: enable reset button
        resetEnabled = true;
        Serial.println("\nReset Button Activated.");
      } else if (inputPassword == displayPassword) {
        // Display password (1234) entered: show counts
        Serial.print("\nA: ");
        Serial.print(button1PressCount);
        Serial.print(" B: ");
        Serial.print(button2PressCount);
        Serial.print(" C: ");
        Serial.println(button3PressCount);
      } else {
        Serial.println("\nIncorrect Password!");
      }
      inputPassword = "";  // Reset input after validation
    } else if (key == '*') {  // Clear input
      inputPassword = "";  // Reset input
      Serial.println("\nInput Cleared.");
    } else {  // Add key to password input
      if (inputPassword.length() < maxPasswordLength) {
        inputPassword += key;
        Serial.print("*");  // Mask input
      } else {
        Serial.println("\nMaximum length reached!");
      }
    }
  }
}
