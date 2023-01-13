#include "Adafruit_ILI9341.h"

// Pin settings for PyPortal
#define TFT_D0        34 // Data bit 0 pin (MUST be on PORT byte boundary)
#define TFT_WR        26 // Write-strobe pin (CCL-inverted timer output)
#define TFT_DC        10 // Data/command pin
#define TFT_CS        11 // Chip-select pin
#define TFT_RST       24 // Reset pin
#define TFT_RD         9 // Read-strobe pin
#define TFT_BACKLIGHT 25 // Backlight

// Pyportal display object
Adafruit_ILI9341 tft = Adafruit_ILI9341(tft8bitbus, TFT_D0, TFT_WR, TFT_DC, TFT_CS, TFT_RST, TFT_RD);

// Screen dimension
int screen_width = 240;
int screen_height = 320;
int count = 0;

// Board layout params
int cell_size = 5;
int num_rows = (screen_height - 1) / cell_size;
int num_cols = (screen_width - 1) / cell_size;
int x_width = cell_size * num_cols;
int y_width = cell_size * num_rows;
int x_offset = (screen_width - x_width) / 2;
int y_offset = (screen_height - y_width) / 2;
bool show_grid = false;

// Board display colors
int cell_color = ILI9341_WHITE;
int background_color = ILI9341_BLACK;
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
  // Reset temp game
  for (int i = 0; i < num_rows; i++) {
    for (int j = 0; j < num_cols; j++) {
      temp_game[i][j] = 0;
    }
  }

  // Compute temp updates
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

  // Copy updates to game
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
  // Draw grid
  if (show_grid == true) {
    for (int i=0; i<num_rows+1; i++) {
      tft.drawLine(x_offset, i * cell_size + y_offset, x_width + x_offset, i * cell_size + y_offset, grid_color);
    }
    for (int i=0; i<num_cols+1; i++) {
      tft.drawLine(i * cell_size + x_offset, y_offset, i * cell_size + x_offset, y_width + y_offset, grid_color);
    }
  }

  // Draw cells
  for (int i=0; i<num_rows; i++) {
    for (int j=0; j<num_cols; j++) {
      // Get cell color
      int cell_value = game[i][j];
      int color;
      if (cell_value == 1) {
        color = cell_color;
      } else {
        color = background_color;
      }
      // Draw cell rect
      int grid_offset;
      if (show_grid == true) {
        grid_offset = 1;
      } else {
        grid_offset = 0;
      }      
      tft.fillRect(j * cell_size + x_offset + grid_offset, i * cell_size + y_offset + grid_offset, cell_size - grid_offset, cell_size - grid_offset, color);
    }
  }
}


void loop() {
  stepGame();
  drawGame();
  delay(10);
  count++;
  Serial.println(count);
}
