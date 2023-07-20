#include "Adafruit_ILI9341.h"
#include "TouchScreen.h"

// Pins settings for PyPortal tft-lcd display
#define TFT_D0 34         // Data bit 0 pin (MUST be on PORT byte boundary)
#define TFT_WR 26         // Write-strobe pin (CCL-inverted timer output)
#define TFT_DC 10         // Data/command pin
#define TFT_CS 11         // Chip-select pin
#define TFT_RST 24        // Reset pin
#define TFT_RD 9          // Read-strobe pin
#define TFT_BACKLIGHT 25  // Backlight

// Pyportal display object
Adafruit_ILI9341 tft = Adafruit_ILI9341(tft8bitbus, TFT_D0, TFT_WR, TFT_DC, TFT_CS, TFT_RST, TFT_RD);

// Screen dimensions
int screen_width = 320;
int screen_height = 240;

// Game board size
int cell_size = 10;
int num_rows = (screen_height - 1) / cell_size;
int num_cols = (screen_width - 1) / cell_size;

// Create dynamically sized game array
int** game;
int** temp_game;

// Game board dimensions and offsets
int board_width = cell_size * num_cols;
int board_height = cell_size * num_rows;
int x_offset = (screen_width - board_width) / 2;
int y_offset = (screen_height - board_height) / 2;

// Game board display colors
int live_color = ILI9341_BLACK;
int dead_color = ILI9341_WHITE;
int grid_color = ILI9341_LIGHTGREY;

// Touchscreen Pins
#define YD A7
#define XR A6
#define YU A5
#define XL A4

// Touchscreen object
TouchScreen ts = TouchScreen(XL, YD, XR, YU, 300);

// Touchscreen calibrations
#define TS_MINX 133
#define TS_MINY 840
#define TS_MAXX 927
#define TS_MAXY 140

// Touch tracking
bool touch_active = false;
int release_threshold = 10;
int release_count = 0;

const int analogOutPin = 25;

void setup() {

  // Initialize serial communication
  Serial.begin(9600);

  // Initialize display
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);

  // Turn on display backlight
  pinMode(TFT_BACKLIGHT, OUTPUT);
  digitalWrite(TFT_BACKLIGHT, HIGH);

  // Initialize game
  initGame();

  analogWrite(analogOutPin, 15);
}


void initGame() {

  // Create game and temp_game of dynamic size
  // based on number of rows and cols
  game = new int*[num_rows];
  temp_game = new int*[num_rows];
  for (int i = 0; i < num_rows; i++) {
    game[i] = new int[num_cols];
    temp_game[i] = new int[num_cols];
  }

  // Randomize initial cell activtion
  for (int i = 0; i < num_rows; i++) {
    for (int j = 0; j < num_cols; j++) {
      game[i][j] = random(2);
    }
  }
}


void stepGame() {

  // Reset temp game array
  for (int i = 0; i < num_rows; i++) {
    for (int j = 0; j < num_cols; j++) {
      temp_game[i][j] = 0;
    }
  }

  // Compute temp game updates
  for (int i = 0; i < num_rows; i++) {
    for (int j = 0; j < num_cols; j++) {
      int neighbors = countNeighbors(i, j);
      if (game[i][j] == 1) {
        if (neighbors < 2 || neighbors > 3) {
          temp_game[i][j] = 0;
        } else {
          temp_game[i][j] = 1;
        }
      } else {
        if (neighbors == 3) {
          temp_game[i][j] = 1;
        } else {
          temp_game[i][j] = 0;
        }
      }
    }
  }

  // Copy updates to game array
  for (int i = 0; i < num_rows; i++) {
    for (int j = 0; j < num_cols; j++) {
      game[i][j] = temp_game[i][j];
    }
  }
}


int countNeighbors(int x, int y) {

  // Return number of neighbors for input x,y position,
  // calculated using edge-wrapping
  int count = 0;
  for (int i = -1; i <= 1; i++) {
    for (int j = -1; j <= 1; j++) {
      if (i == 0 && j == 0) {
        continue;
      }
      int newX = (x + i + num_rows) % num_rows;
      int newY = (y + j + num_cols) % num_cols;
      count += game[newX][newY];
    }
  }
  return count;
}


void drawGame() {

  // Draw game grid
  for (int i = 0; i < num_rows + 1; i++) {
    tft.drawLine(
      x_offset, 
      i * cell_size + y_offset, 
      x_offset + board_width, 
      i * cell_size + y_offset,
      grid_color
    );
  }
  for (int i = 0; i < num_cols + 1; i++) {
    tft.drawLine(
      i * cell_size + x_offset, 
      y_offset, 
      i * cell_size + x_offset, 
      y_offset + board_height, 
      grid_color
    );
  }

  // Draw game cells
  for (int i = 0; i < num_rows; i++) {
    for (int j = 0; j < num_cols; j++) {

      // Get cell color
      int color = (game[i][j] == 1) ? live_color : dead_color;

      // Draw cell rect
      tft.fillRect(
        j * cell_size + x_offset + 1,
        i * cell_size + y_offset + 1,
        cell_size - 1,
        cell_size - 1,
        color
      );
    }
  }
}


void loop() {

  // Get touch point
  TSPoint p = ts.getPoint();



  // Handle touch event
  if (p.z > 0) {

    // Calculate touch pixel coordinates
    int x = map(p.x, TS_MINX, TS_MAXX, 0, screen_width);
    int y = map(p.y, TS_MINY, TS_MAXY, 0, screen_height);

    // Check if touch was on game board
    x = x - x_offset;
    y = y - y_offset;
    if (x >= 0 && x <= board_width && y >= 0 && y <= board_height) {

      // Calculate touched cell position
      int row = map(y, 0, board_height, 0, num_rows);
      int col = map(x, 0, board_width, 0, num_cols);

      // Activate touched cell
      game[row][col] = 1;

      // Set touch as active
      if (touch_active == false) {
        touch_active = true;
        //Serial.println("Touch Started");
      }

      // Reset release count
      release_count = 0;
    }
  }

  // Handle no touch event
  else {

    // Set touch as not active if released
    if (touch_active == true) {
      if (release_count >= release_threshold) {
        touch_active = false;
        //Serial.println("Touch Ended");
      } else {
        release_count++;
      }
    }
  }

  // Step game forward if no touch detected
  if (touch_active == false) {
    stepGame();
    delay(100);
  }

  // Draw game on display
  drawGame();
}
