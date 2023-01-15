#include "Adafruit_ILI9341.h"
#include "TouchScreen.h"

// Pins settings for PyPortal tft-lcd display
#define TFT_D0        34 // Data bit 0 pin (MUST be on PORT byte boundary)
#define TFT_WR        26 // Write-strobe pin (CCL-inverted timer output)
#define TFT_DC        10 // Data/command pin
#define TFT_CS        11 // Chip-select pin
#define TFT_RST       24 // Reset pin
#define TFT_RD         9 // Read-strobe pin
#define TFT_BACKLIGHT 25 // Backlight

// Pyportal display object
Adafruit_ILI9341 tft = Adafruit_ILI9341(tft8bitbus, TFT_D0, TFT_WR, TFT_DC, TFT_CS, TFT_RST, TFT_RD);

// Touchscreen Pins
#define YP A4  // must be an analog pin, use "An" notation!
#define XM A7  // must be an analog pin, use "An" notation!
#define YM A6   // can be a digital pin
#define XP A5   // can be a digital pin

// Touchscreen calibrations
#define X_MIN  160
#define X_MAX  917
#define Y_MIN  947
#define Y_MAX  136

// Touchscreen object
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 3000);

// Screen dimension
int screen_width = 240;
int screen_height = 320;

// Touch tracking
bool touch_active = false;
int release_threshold = 10;
int release_count = 0;

// Game board layout params
int cell_size = 6;
int num_rows = (screen_height - 1) / cell_size;
int num_cols = (screen_width - 1) / cell_size;
int x_width = cell_size * num_cols;
int y_width = cell_size * num_rows;
int x_offset = (screen_width - x_width) / 2;
int y_offset = (screen_height - y_width) / 2;
bool show_grid = true;

// Game board display colors
int cell_color = ILI9341_BLACK;
int background_color = ILI9341_WHITE;
int grid_color = ILI9341_LIGHTGREY;

// Initialize dynamically sized game array
int** game;
int** temp_game;

void setup() {
  Serial.begin(9600);

  // Initialize display
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);

  // Turn on backlight 
  pinMode(TFT_BACKLIGHT, OUTPUT);
  digitalWrite(TFT_BACKLIGHT, HIGH);

  // Initialize game arrays
  initGame();
}


void initGame() {  
  // Create game and temp_game of dynamic size
  // based on number of rows and cols
  game = new int*[num_rows];
  temp_game = new int*[num_rows];
  for(int i = 0; i < num_rows; i++) {
      game[i] = new int[num_cols];
      temp_game[i] = new int[num_cols];
  }

  // Randomize initial pattern
  randomizeGame();
}

void randomizeGame() {
  // Add random pattern to the game
  for (int i=0; i<num_rows; i++) {
    for (int j=0; j<num_cols; j++) {
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
  if (show_grid == true) {
    for (int i=0; i<num_rows+1; i++) {
      tft.drawLine(x_offset, i * cell_size + y_offset, x_width + x_offset, i * cell_size + y_offset, grid_color);
    }
    for (int i=0; i<num_cols+1; i++) {
      tft.drawLine(i * cell_size + x_offset, y_offset, i * cell_size + x_offset, y_width + y_offset, grid_color);
    }
  }

  // Draw game cells
  for (int i=0; i<num_rows; i++) {
    for (int j=0; j<num_cols; j++) {

      // Get cell color
      int color = (game[i][j] == 1) ? cell_color : background_color;

      // Get grid offset
      int grid_offset = (show_grid == true) ? 1 : 0;

      // Draw cell rect
      tft.fillRect(
        j * cell_size + x_offset + grid_offset, 
        i * cell_size + y_offset + grid_offset, 
        cell_size - grid_offset, 
        cell_size - grid_offset, 
        color
      );
    }
  }
}


void loop() {

  // Get touch info
  TSPoint p = ts.getPoint();
  
  // Handle touch detected
  if (p.z > ts.pressureThreshhold) {

    // Get touch coorinates
    int x = map(p.x, X_MIN, X_MAX, 0, 240);
    int y = map(p.y, Y_MIN, Y_MAX, 0, 320);

    // Check if touch was on game board
    x = x - x_offset;
    y = y - y_offset;
    if (x >= 0 && x <= x_width && y >= 0 && y <= y_width) {

      // Activate touched cell
      int row = map(y, 0, y_width, 0, num_rows);
      int col = map(x, 0, x_width, 0, num_cols);
      game[row][col] = 1;

      // Reset release count
      release_count = 0;

      // Set touch as active
      if (touch_active == false) {
        touch_active = true;
        Serial.println("Touch Started"); 
      }
    }
  }

  // Handle touch not detected
  else {
    // Set touch as not active if released
    if (touch_active == true) {
      if (release_count >= release_threshold) {
        touch_active = false;
        Serial.println("Touch Ended"); 
      } else {
        release_count++;
      }
    }
  }    

  // Step game forward if no touch detected
  if (touch_active == false) {
    stepGame();
    delay(50);
  }

  // Draw game board
  drawGame();
}
