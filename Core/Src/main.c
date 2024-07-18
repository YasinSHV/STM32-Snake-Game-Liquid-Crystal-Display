/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "LiquidCrystal.h"
#include "stdio.h"
#include "stdlib.h"
#include "time.h"
#include "music_library.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DEBOUNCE_DELAY 350
#define DISPLAY_ON GPIO_PIN_RESET
#define DISPLAY_OFF GPIO_PIN_SET
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

RTC_HandleTypeDef hrtc;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;

PCD_HandleTypeDef hpcd_USB_FS;

/* USER CODE BEGIN PV */
struct digit {
	int number;
	uint16_t pattern;
	uint16_t anti_pattern;
};

struct RTC_Time {
	int hour;
	int minute;
	int second;
};

struct RTC_Time effect_time;

typedef unsigned char byte;
byte head[4][8] = { { 0x00, 0x00, 0x0E, 0x11, 0x11, 0x1B, 0x11, 0x1F }, { 0x1F,
		0x11, 0x1B, 0x11, 0x11, 0x0E, 0x00, 0x00 }, { 0x1C, 0x12, 0x15, 0x11,
		0x11, 0x15, 0x12, 0x1C }, { 0x07, 0x09, 0x15, 0x11, 0x11, 0x15, 0x09,
		0x07 } };

byte body[8] = { 0x1F, 0x15, 0x17, 0x1D, 0x17, 0x1D, 0x15, 0x1F };

byte apple[8] = { 0x00, 0x08, 0x04, 0x0E, 0x17, 0x1F, 0x1F, 0x0E };

byte wall[8] = { 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E };

byte heal[8] = { 0x00, 0x00, 0x0A, 0x1F, 0x1F, 0x0E, 0x04, 0x00 };

byte lucky_box[8] = { 0x00, 0x0A, 0x04, 0x1F, 0x15, 0x1F, 0x15, 0x1F };

const int GAME_WIDTH = 20;
const int GAME_HEIGHT = 4;

int highScore = 0;
int currentScore = 0;

extern int isSfx;

int carrier[] = { 0, 0, 0, 0 };

enum GameState {
	MENU, OPTIONS, GAME, GAMEOVER
};
enum GameState gameState = MENU;

struct Settings {
	int wallCollision;
	int health;
	int speed;
	int soundOn;
	int obstacleCount;
	int gameMode;
	int appleSpawnTime;
};

struct Settings settings;
int currentHealth = 0;
enum OptionsEnumerator {
	ABOUT, SETTINGS, MOD, START, NONE
};

enum OptionsEnumerator optionsEnumerator = ABOUT;
enum OptionsEnumerator currentOption = NONE;

enum Direction {
	UP, DOWN, LEFT, RIGHT
};

enum CollisionMask {
	SNAKE, APPLE, WALL, HEAL, OBSTACLE, BOX, NO_COLLISION
};

enum Effect {
	NOEFFECT, SPEED, SCORE, HEALTH
};

enum Effect effect = NOEFFECT;
enum Direction direction = RIGHT;
enum Direction prevDirection = UP;

int movementCoolDown = 0;

typedef struct {
	int x;
	int y;
} Point;

Point foodPosition;
Point medkit;
Point box;

Point wallPositions[6];

struct Node {
	Point point;
	Point prevPoint;
	struct Node *next;
	struct Node *prev;
};

struct Node *snakeHead;

struct Node* createNode(int x, int y) {
	struct Node *newNode = (struct Node*) malloc(sizeof(struct Node));
	newNode->point.x = x;
	newNode->point.y = y;
	newNode->next = NULL;
	newNode->prev = NULL;
	return newNode;
}

void append(struct Node **head, int x, int y) {
	struct Node *newNode = createNode(x, y);
	if (*head == NULL) {
		*head = newNode;
		return;
	}
	struct Node *temp = *head;
	while (temp->next != NULL) {
		temp = temp->next;
	}
	temp->next = newNode;
	newNode->prev = temp;
}

void deleteTail(struct Node **head) {
	if (*head == NULL)
		return;
	struct Node *temp = *head;
	while (temp->next != NULL) {
		temp = temp->next;
	}
	if (temp->prev != NULL) {
		temp->prev->next = NULL;
	} else {
		*head = NULL;
	}
	free(temp);
}

void deleteSnakeExceptHead(struct Node **head) {
	if (*head == NULL || (*head)->next == NULL)
		return;

	struct Node *current = (*head)->next;
	struct Node *nextNode;

	while (current != NULL) {
		nextNode = current->next;
		free(current);
		current = nextNode;
	}

	(*head)->next = NULL;
}

void updateBody() {
	struct Node *node = snakeHead->next; // Start from the second segment
	setCursor(snakeHead->point.x, snakeHead->point.y);
	write(1); // Draw the head at its new position
	setCursor(node->prev->prevPoint.x, node->prev->prevPoint.y);
	write(0);

	// Move each segment to the position of the previous segment
	while (node != NULL) {
		if (node->next == NULL) {
			setCursor(node->point.x, node->point.y);
			print(" "); // Clear the previous position
		}

		// Save current position before moving
		node->prevPoint.x = node->point.x;
		node->prevPoint.y = node->point.y;

		// Move to the previous segment's previous position
		if (node->prev != NULL) {
			node->point.x = node->prev->prevPoint.x;
			node->point.y = node->prev->prevPoint.y;
		}
		node = node->next;
	}
}

void growSnake() {
	// Add a new segment to the snake at the tail's previous position
	struct Node *tail = snakeHead;
	while (tail->next != NULL) {
		tail = tail->next;
	}
	append(&snakeHead, tail->prevPoint.x, tail->prevPoint.y);
	generateApple();
	currentScore += 1;
	if (settings.soundOn) {
		isSfx = 1;
	}

	if (currentScore < 100) {
		int score[2];
		score[0] = carrier[0];
		score[1] = carrier[1];
		int num = score[0] * 10 + carrier[1];
		if (effect == SCORE)
			num += 2;
		else
			num += 1;
		carrier[0] = num / 10;
		carrier[1] = num % 10;
	}
}

int isPointInSnake(int x, int y) {
	struct Node *temp = snakeHead;
	while (temp != NULL) {
		if (temp->point.x == x && temp->point.y == y) {
			return 1;
		}
		temp = temp->next;
	}
	return 0;
}

int isPointWall(int x, int y) {
	for (int i = 0; i < settings.obstacleCount; i++) {
		if (wallPositions[i].x == x && wallPositions[i].y == y)
			return 1;
	}
	return 0;
}

int isPointApple(int x, int y) {
	if (foodPosition.x == x && foodPosition.y == y) {
		return 1;
	}
	return 0;
}

int isPointBox(int x, int y) {
	if (box.x == x && box.y == y) {
		return 1;
	}
	return 0;
}

int isPointUI(int x, int y) {
	if (y == 0 && (x >= 0 && x <= 14))
		return 1;
	return 0;
}

void generateApple() {
	RTC_TimeTypeDef my_time;
	HAL_RTC_GetTime(&hrtc, &my_time, RTC_FORMAT_BIN);

	setCursor(foodPosition.x, foodPosition.y);
	print(" ");

	srand((my_time.Seconds) + 1 * (my_time.Minutes + 1));
	int x, y;
	Point temp;
	do {
		x = rand() % GAME_WIDTH;
		y = rand() % GAME_HEIGHT;
		temp.x = x;
		temp.y = y;
	} while (isPointInSnake(x, y) || isPointWall(x, y) || isPointBox(x, y)
			|| isCollidingWithHeal(temp) || isPointUI(x, y)); // Ensure apple is not on the snake's body

	foodPosition.x = x;
	foodPosition.y = y;

	// Draw the apple on the game board
	setCursor(foodPosition.x, foodPosition.y);
	write(3);
}

void generateAMedkit() {
	RTC_TimeTypeDef my_time;
	HAL_RTC_GetTime(&hrtc, &my_time, RTC_FORMAT_BIN);

	setCursor(medkit.x, medkit.y);
	print(" ");

	srand((my_time.Seconds) + 1 * (my_time.Minutes + 1));
	int x, y;
	Point temp;
	do {
		x = rand() % GAME_WIDTH;
		y = rand() % GAME_HEIGHT;
		temp.x = x;
		temp.y = y;
	} while (isPointInSnake(x, y) || isPointWall(x, y) || isPointBox(x, y)
			|| isCollidingWithFood(temp) || isPointUI(x, y));

	medkit = temp;

	setCursor(medkit.x, medkit.y);
	write(5);
}

void generateBox() {
	RTC_TimeTypeDef my_time;
	HAL_RTC_GetTime(&hrtc, &my_time, RTC_FORMAT_BIN);

	setCursor(box.x, box.y);
	print(" ");

	srand((my_time.Seconds) + 1 * (my_time.Minutes + 1));
	int x, y;
	Point temp;
	do {
		x = rand() % GAME_WIDTH;
		y = rand() % GAME_HEIGHT;
		temp.x = x;
		temp.y = y;
	} while (isPointInSnake(x, y) || isPointWall(x, y)
			|| isCollidingWithFood(temp) || isPointUI(x, y));

	box = temp;

	setCursor(box.x, box.y);
	write(6);
}

void generateWalls(int maxWalls, int mode) {
	if (maxWalls == 0) {
		return;
	}

	RTC_TimeTypeDef my_time;
	HAL_RTC_GetTime(&hrtc, &my_time, RTC_FORMAT_BIN);

	srand((my_time.Seconds) + 1 * (my_time.Minutes + 1));

	for (int i = 0; i < maxWalls; i++) {
		if (mode == 1) {
			i = rand() % settings.obstacleCount;
		}
		setCursor(wallPositions[i].x, wallPositions[i].y);
		print(" ");

		int x, y;
		do {
			x = rand() % GAME_WIDTH;
			y = rand() % GAME_HEIGHT;
		} while (isPointInSnake(x, y) || isPointApple(x, y) || isPointBox(x, y)
				|| isPointUI(x, y) || isPointWall(x, y));

		wallPositions[i].x = x;
		wallPositions[i].y = y;

		// Draw the apple on the game board
		setCursor(wallPositions[i].x, wallPositions[i].y);
		write(4);
	}
}

void redraw_walls() {
	for (int i = 0; i < settings.obstacleCount; i++) {
		setCursor(wallPositions[i].x, wallPositions[i].y);
		write(4);
	}
}

void move_in_direction(enum Direction dir) {
	// Update snake head's previous position
	Point temp = snakeHead->point;
	snakeHead->prevPoint = snakeHead->point;

	// Update snake head's position based on direction
	switch (dir) {
	case UP:
		if (prevDirection != DOWN) {
			snakeHead->point.y -= 1;
			if (dir != prevDirection)
				createChar(1, head[0]);
		}
		break;
	case DOWN:
		if (prevDirection != UP) {
			snakeHead->point.y += 1;
			if (dir != prevDirection)
				createChar(1, head[1]);
		}
		break;
	case RIGHT:
		if (prevDirection != LEFT) {
			snakeHead->point.x += 1;
			if (dir != prevDirection)
				createChar(1, head[2]);
		}
		break;
	case LEFT:
		if (prevDirection != RIGHT) {
			snakeHead->point.x -= 1;
			if (dir != prevDirection)
				createChar(1, head[3]);
		}
		break;
	}

	// Update the current direction
	if (prevDirection != dir)
		prevDirection = dir;

	// Detect collision
	enum CollisionMask collision = CollisionDetector();

	// Handle collision based on its type
	switch (collision) {
	case WALL:
		// Handle wall collision

		if (snakeHead->point.x < 0) {
			snakeHead->point.x = GAME_WIDTH - 1;
		} else if (snakeHead->point.x >= GAME_WIDTH) {
			snakeHead->point.x = 0;
		} else if (snakeHead->point.y < 0) {
			snakeHead->point.y = GAME_HEIGHT - 1;
		} else if (snakeHead->point.y >= GAME_HEIGHT) {
			snakeHead->point.y = 0;
		}
		if (settings.wallCollision) {
			if (currentHealth == 1) {
				gameover(); // Assume this function handles game over scenario
			} else {
				if (settings.soundOn) {
					isSfx = 2;
				}
				currentHealth--;
			}
		}
		break;
	case OBSTACLE:
		redraw_walls();
		if (currentHealth == 1) {
			gameover(); // Assume this function handles game over scenario
		} else {
			if (settings.soundOn) {
				isSfx = 2;
			}
			currentHealth--;
			direction = change_direction(temp);
			move_in_direction(direction);
			return;
		}
		break;
	case SNAKE:
		if (settings.soundOn) {
			isSfx = 2;
		}
		if (currentHealth == 1) {
			gameover(); // Assume this function handles game over scenario
		} else {
			currentHealth--;
			direction = change_direction(temp);
			move_in_direction(direction);
			return;
		}
		break;
	case APPLE:
		// Handle apple collision (e.g., grow the snake, update score)
		if (movementCoolDown < 0)
			movementCoolDown = 0;
		growSnake();
		break;
	case HEAL:
		if (currentHealth < 9) {
			currentHealth++;
			if (settings.soundOn) {
				isSfx = 1;
			}
		}
		medkit.x = -1;
		medkit.y = -1;
		break;
	case BOX:
		if (settings.soundOn) {
			isSfx = 1;
		}
		RTC_TimeTypeDef my_time;
		HAL_RTC_GetTime(&hrtc, &my_time, RTC_FORMAT_BIN);

		effect_time.hour = my_time.Hours;
		effect_time.minute = my_time.Minutes;
		effect_time.second = my_time.Seconds;

		srand((my_time.Seconds) + 1 * (my_time.Minutes + 1));
		int random = 0;
		do {
			random = rand() % 4;
		} while (random == 0);

		effect = random;
		if (effect == HEALTH) {
			if (currentHealth < 9) {
				currentHealth++;
			}
			effect = NOEFFECT;
		}

		box.x = -1;
		box.y = -1;

		break;
	default:
		break;
	}

// Update the snake's body
	updateBody();
}

int change_direction(Point prev) {
	Point temp;
	switch (prevDirection) {
	case UP:
	case DOWN:
		if (!isPointWall(snakeHead->point.x + 1, snakeHead->point.y)) {
			temp = prev;
			temp.x += 1;
			if (!isCollidingWithSelf(temp)) {
				snakeHead->point = prev;
				return RIGHT;
			}
		}
		if (!isPointWall(snakeHead->point.x - 1, snakeHead->point.y)) {
			temp = prev;
			temp.x -= 1;
			if (!isCollidingWithSelf(temp)) {
				snakeHead->point = prev;
				return LEFT;
			}
		}
		break;

	case RIGHT:
	case LEFT:
		if (!isPointWall(snakeHead->point.x, snakeHead->point.y - 1)) {
			temp = prev;
			temp.y -= 1;
			if (!isCollidingWithSelf(temp)) {
				snakeHead->point = prev;
				return UP;
			}
		}
		if (!isPointWall(snakeHead->point.x, snakeHead->point.y + 1)) {
			temp = prev;
			temp.y += 1;
			if (!isCollidingWithSelf(temp)) {
				snakeHead->point = prev;
				return DOWN;
			}
		}
		break;
	}
	gameover();
	return RIGHT;
}

int isCollidingWithWall(Point point) {
	return (point.x < 0 || point.x >= GAME_WIDTH)
			|| (point.y < 0 || point.y >= GAME_HEIGHT);
}

int isCollidingWithSelf(Point point) {
	struct Node *temp = snakeHead->next; // Skip the head
	while (temp != NULL) {
		if (temp->point.x == point.x && temp->point.y == point.y) {
			return 1;
		}
		temp = temp->next;
	}
	return 0;
}

int isCollidingWithFood(Point point) {
	return point.x == foodPosition.x && point.y == foodPosition.y;
}

int isCollidingWithHeal(Point point) {
	return point.x == medkit.x && point.y == medkit.y;
}
int isCollidingWithObstacle(Point point) {
	for (int i = 0; i < settings.obstacleCount; i++) {
		if (point.x == wallPositions[i].x && point.y == wallPositions[i].y) {
			return 1;
		}
	}
	return 0;
}

int CollisionDetector() {
	enum CollisionMask collision = NO_COLLISION;

	if (isCollidingWithWall(snakeHead->point)) {
		collision = WALL;
	} else if (isCollidingWithSelf(snakeHead->point)) {
		collision = SNAKE;
	} else if (isCollidingWithFood(snakeHead->point)) {
		collision = APPLE;
	} else if (isCollidingWithHeal(snakeHead->point)) {
		collision = HEAL;
	} else if (isCollidingWithObstacle(snakeHead->point)) {
		collision = OBSTACLE;
	} else if (isPointBox(snakeHead->point.x, snakeHead->point.y)) {
		collision = BOX;
	}

	return collision;
}

int game_ready = 0;
int frame = 0;
int prev_frame = 0;

struct RTC_Time start_time;
struct RTC_Time speed_increase_time;
struct RTC_Time apple_spawn_time;
struct RTC_Time heal_spawn_time;
struct RTC_Time heal_despawn_time;
struct RTC_Time box_spawn_time;
struct RTC_Time box_despawn_time;
struct RTC_Time wall_spawn_time;

int game_paused = 0;
int paused = 0;
int is_dead = 0;
int in_menu = 1;
int medkitSpawned = 0;
int boxSpawned = 0;

//Seven-Segment
int index = 0;
uint16_t led[4];
struct digit digits[10];

extern struct Dictionary *playlist;
extern char **playlistOrder;
int playedCount = 0, currentMusic = 0, adc_select = 0;

extern volatile uint16_t volume;
struct Tone *melody;

enum ProgramMode {
	Shuffle, Liner
};
enum ProgramMode programMode = Liner;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_USB_PCD_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_RTC_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */

int compareStrings(const char *str1, const uint8_t *str2, int n) {
	for (int i = 0; i < n; i++) {
		if (str1[i] != str2[i]) {
			return 0;
		}
	}
	return 1;
}

int setting_threshold = 0;

char done[4] = "done";

char username[15];

uint8_t data[100];
uint8_t d;
uint8_t i;

void set_sound(int value) {
	if (value >= 0 && value <= 1) {
		settings.soundOn = value;
		char ack[] = "Sound setting updated\n";
		HAL_UART_Transmit_IT(&huart1, (uint8_t*) ack, strlen(ack));
	} else {
		char ack[] = "Invalid sound value\n";
		HAL_UART_Transmit_IT(&huart1, (uint8_t*) ack, strlen(ack));
	}
}

void set_health(int value) {
	if (value >= 1 && value <= 6) {
		settings.health = value;
		char ack[] = "Health setting updated\n";
		HAL_UART_Transmit_IT(&huart1, (uint8_t*) ack, strlen(ack));
	} else {
		char ack[] = "Invalid health value\n";
		HAL_UART_Transmit_IT(&huart1, (uint8_t*) ack, strlen(ack));
	}
}

void set_obstacle_count(int value) {
	if (value >= 0 && value <= 6) {
		settings.obstacleCount = value;
		char ack[] = "Obstacle count updated\n";
		HAL_UART_Transmit_IT(&huart1, (uint8_t*) ack, strlen(ack));
	} else {
		char ack[] = "Invalid obstacle count value\n";
		HAL_UART_Transmit_IT(&huart1, (uint8_t*) ack, strlen(ack));
	}
}

void set_speed(int value) {
	if (value >= 1 && value <= 3) {
		settings.speed = value;
		char ack[] = "Speed setting updated\n";
		HAL_UART_Transmit_IT(&huart1, (uint8_t*) ack, strlen(ack));
	} else {
		char ack[] = "Invalid speed value\n";
		HAL_UART_Transmit_IT(&huart1, (uint8_t*) ack, strlen(ack));
	}
}

void set_game_mode(int value) {
	if (value >= 1 && value <= 3) {
		settings.gameMode = value;
		char ack[] = "Game mode updated\n";
		HAL_UART_Transmit_IT(&huart1, (uint8_t*) ack, strlen(ack));
	} else {
		char ack[] = "Invalid game mode value\n";
		HAL_UART_Transmit_IT(&huart1, (uint8_t*) ack, strlen(ack));
	}
}

void set_username(const char *new_username) {
	strncpy(username, new_username, sizeof(username) - 1);
	username[sizeof(username) - 1] = '\0';
	char ack[] = "Username updated\n";
	HAL_UART_Transmit_IT(&huart1, (uint8_t*) ack, strlen(ack));
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART1) {
		HAL_UART_Receive_IT(&huart1, &d, 1);
		data[i++] = d;
		if (d == '\n') {
			data[i] = '\0';
			char *command = strtok((char*) data, "(");
			char *value = strtok(NULL, ")");

			if (command && value) {
				if (gameState == OPTIONS && currentOption == SETTINGS
						|| currentOption == MOD) {
					if (currentOption == OPTIONS) {
						if (strcmp(command, "set_name") == 0) {
							set_username(value);
						} else if (strcmp(command, "set_sound") == 0) {
							set_sound(atoi(value));
							char sn[16];
							i = settings.soundOn;
							snprintf(sn, sizeof(sn), "1-SFX         %d", i);
							setCursor(4, 2);
							print(sn);
						} else if (strcmp(command, "set_health") == 0) {
							set_health(atoi(value));
							char hp[16];
							int i = settings.health;
							snprintf(hp, sizeof(hp), "1-Health      %d", i);
							setCursor(4, 0);
							print(hp);
						} else if (strcmp(command, "set_obstacle_count") == 0) {
							set_obstacle_count(atoi(value));
							setCursor(4, 3);
							char wl[16];
							i = settings.obstacleCount;
							snprintf(wl, sizeof(wl), "1-Wall Count  %d", i);
							print(wl);
						} else if (strcmp(command, "set_speed") == 0) {
							set_speed(atoi(value));
							char sp[16];
							i = settings.speed;
							snprintf(sp, sizeof(sp), "1-Speed       %d", i);
							setCursor(4, 1);
							print(sp);
						} else if (strcmp(command, "set_game_mode") == 0) {
							set_game_mode(atoi(value));
							char sp[16];
							int i = settings.gameMode;
							snprintf(sp, sizeof(sp), "1-Game Mode   %d", i);
							setCursor(2, 1);
							print(sp);
						}
					}
				} else {
					char ack[] = "ERROR\n";
					HAL_UART_Transmit_IT(&huart1, (uint8_t*) ack, strlen(ack));
					i = 0;
					return;
				}
			}

			i = 0; // Reset the buffer index
		}
	}
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM1) {
		if (game_ready) {
			if (game_paused) {
				game_paused = 0;
				if (!paused) {
					paused = 1;
					setCursor(0, 0);
					print("II ");
				} else {
					paused = 0;
				}
			}

			if (!paused) {

				RTC_TimeTypeDef my_time;
				HAL_RTC_GetTime(&hrtc, &my_time, RTC_FORMAT_BIN);
				struct RTC_Time temp;
				temp.hour = my_time.Hours;
				temp.minute = my_time.Minutes;
				temp.second = my_time.Seconds;

				if (settings.gameMode == 3) {

					int diffrance = calculate_time_difference(
							speed_increase_time, temp);
					if (diffrance > 5) {
						movementCoolDown -= 100;
						speed_increase_time.hour = my_time.Hours;
						speed_increase_time.minute = my_time.Minutes;
						speed_increase_time.second = my_time.Seconds;
					}

				}

				if (currentScore > highScore)
					highScore = currentScore;

				int movementFactor = 0;
				if (effect == SPEED)
					movementFactor = 500;

				if (frame - prev_frame > movementCoolDown - movementFactor) {
					move_in_direction(direction);
					prev_frame = frame;
					char hp[4];
					sprintf(hp, "HP%d", currentHealth);
					setCursor(0, 0);
					print(hp);

					if (effect != NOEFFECT) {
						setCursor(3, 0);
						if (effect == SPEED) {
							print(">>");
						} else if (effect == SCORE) {
							print("2X");
						}

						if (calculate_time_difference(effect_time, temp) > 10) {
							effect = NOEFFECT;
							setCursor(3, 0);
							print("  ");
						}
					}

					uint32_t current_tick = HAL_GetTick();
				}
				if (calculate_time_difference(wall_spawn_time, temp) > 30) {
					generateWalls(1, 1);
					wall_spawn_time.hour = my_time.Hours;
					wall_spawn_time.minute = my_time.Minutes;
					wall_spawn_time.second = my_time.Seconds;
				}

				if (calculate_time_difference(apple_spawn_time, temp)
						> settings.appleSpawnTime) {
					apple_spawn_time.hour = my_time.Hours;
					apple_spawn_time.minute = my_time.Minutes;
					apple_spawn_time.second = my_time.Seconds;
					generateApple();
				}
				if (calculate_time_difference(heal_spawn_time, temp) > 10) {
					if (!medkitSpawned) {
						generateAMedkit();
						medkitSpawned = 1;
						heal_despawn_time.hour = my_time.Hours;
						heal_despawn_time.minute = my_time.Minutes;
						heal_despawn_time.second = my_time.Seconds;
					}

					if (calculate_time_difference(heal_despawn_time, temp)
							> 5) {
						medkitSpawned = 0;

						setCursor(medkit.x, medkit.y);
						print(" ");
						medkit.x = -1;
						medkit.y = -1;

						heal_spawn_time.hour = my_time.Hours;
						heal_spawn_time.minute = my_time.Minutes;
						heal_spawn_time.second = my_time.Seconds;

					}
				}

				if (calculate_time_difference(box_spawn_time, temp) > 60) {
					if (!boxSpawned) {
						generateBox();
						boxSpawned = 1;
						box_despawn_time.hour = my_time.Hours;
						box_despawn_time.minute = my_time.Minutes;
						box_despawn_time.second = my_time.Seconds;
					}

					if (calculate_time_difference(box_despawn_time, temp)
							> 15) {
						boxSpawned = 0;

						setCursor(box.x, box.y);
						print(" ");
						box.x = -1;
						box.y = -1;

						box_spawn_time.hour = my_time.Hours;
						box_spawn_time.minute = my_time.Minutes;
						box_spawn_time.second = my_time.Seconds;

					}
				}

				int num = calculate_time_difference(start_time, temp);
				if (num > 99) {
					start_time.hour = my_time.Hours;
					start_time.minute = my_time.Minutes;
					start_time.second = my_time.Seconds;
					num = 0;
				}
				carrier[3] = num % 10;
				carrier[2] = num / 10;
				frame++;
			}

			if (gameState == MENU) {
				clear();
				menu();
				return;
			} else if (game_ready == 0) {
				clear();
				gameover();
			}
		}

		if (gameState == OPTIONS) {
			if (currentOption == ABOUT) {
				RTC_TimeTypeDef my_time;
				HAL_RTC_GetTime(&hrtc, &my_time, RTC_FORMAT_BIN);
				RTC_DateTypeDef my_date;
				HAL_RTC_GetDate(&hrtc, &my_date, RTC_FORMAT_BIN);

				char time_string[20];
				int hour = my_time.Hours;
				int min = my_time.Minutes;
				int sec = my_time.Seconds;

				char date_string[20];
				int year = my_date.Year;
				int month = my_date.Month;
				int date = my_date.Date;

				snprintf(time_string, sizeof(time_string),
						"TIME(%02d:%02d:%02d)", hour, min, sec);
				snprintf(date_string, sizeof(date_string),
						"DATE(%02d:%02d:%02d)", year, month, date);
				setCursor(3, 2);
				print(date_string);
				setCursor(3, 3);
				print(time_string);
				if (currentOption == NONE)
					show_options();
			}

		}
	} else if (htim->Instance == TIM3) {
		static int index = 0;
		if (index > 3) {
			init_display();
			index = 0;
		}
		display_number(index, carrier[index]);
		index++;
	}
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void display_number(int led_flag, int _number) {
	HAL_GPIO_WritePin(GPIOD,
	GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOD, led[led_flag], DISPLAY_ON);
	if (led_flag == 1)
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
	else {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
	}
	if (_number != 0) {
		HAL_GPIO_WritePin(GPIOB, digits[_number].pattern, GPIO_PIN_SET);
	}
	HAL_GPIO_WritePin(GPIOB, digits[_number].anti_pattern, GPIO_PIN_RESET);
}

void init_display() {
	HAL_GPIO_WritePin(GPIOD,
	GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOB,
	GPIO_PIN_10, GPIO_PIN_SET);
}

int digit_count(int val) {
	if (val == 0)
		return 1;
	int size = 0;
	int temp = val;

	while (temp > 0) {
		temp /= 10;
		size++;
	}
	return size;
}

void extract_int_to_carrier(int val) {
	int i;
	i = digit_count(val) - 1;

	while (val > 0) {
		carrier[i--] = val % 10;
		val /= 10;
	}
}

int array_to_number(int *array, int size) {
	int number = 0;
	for (int i = 0; i < size; i++) {
		number = number * 10 + array[i];
	}
	return number;
}

int generate_random_int(int limit) {
	int random_number;
	return random_number = rand() % limit;
}

void set_music(int num) {
	int toneCount;
	struct DictionaryNode *node;
	melody = lookup(playlist, playlistOrder[num - 1], &toneCount, &node);
	Change_Melody(melody, toneCount);
	currentMusic = num;
	playedCount = num;
	unsetBlacklisted(node);
}

int choice_indx = -1;
void increment_choice() {
	if (currentOption == NONE) {
		setCursor(3, optionsEnumerator++);
		print(" ");
		if (optionsEnumerator > 3)
			optionsEnumerator = 0;
		setCursor(3, optionsEnumerator);
		write(2);
	} else {
		if (currentOption == SETTINGS) {
			handle_sub_menu(3, 1);
		}
	}

}

void handle_sub_menu(int higherBound, int modifier) {
	if (modifier > 0) {
		setCursor(3, choice_indx++);
		print(" ");
		if (choice_indx > higherBound)
			choice_indx = 0;
		setCursor(3, choice_indx);
		write(2);
	} else {
		setCursor(3, choice_indx--);
		print(" ");
		if (choice_indx < 0)
			choice_indx = higherBound;
		setCursor(3, choice_indx);
		write(2);
	}
}

void decrement_choice() {
	if (currentOption == NONE) {
		setCursor(3, optionsEnumerator--);
		print(" ");
		if (optionsEnumerator == 255)
			optionsEnumerator = 3;
		setCursor(3, optionsEnumerator);
		write(2);
	} else {
		if (currentOption == SETTINGS) {
			handle_sub_menu(3, -1);
		}

	}
}

void handle_menu_choice(int modifier) {
	currentOption = optionsEnumerator;

	switch (currentOption) {
	case ABOUT:
		clear();
		setCursor(3, 0);
		print("Yasin Shabani");
		setCursor(3, 1);
		print("Amir Shariati");
		break;

	case SETTINGS:
		switch (choice_indx) {
		case 0:
			if ((modifier < 0 && settings.health > 1)
					|| (modifier > 0 && settings.health < 6)) {
				settings.health += modifier;
				char hp[16];
				int i = settings.health;
				snprintf(hp, sizeof(hp), "1-Health      %d", i);
				setCursor(4, 0);
				print(hp);
			}
			break;
		case 1:
			if ((modifier < 0 && settings.speed > 1)
					|| (modifier > 0 && settings.speed < 3)) {
				settings.speed += modifier;
				char sp[16];
				i = settings.speed;
				snprintf(sp, sizeof(sp), "1-Speed       %d", i);
				setCursor(4, 1);
				print(sp);
			}
			break;
		case 2:
			if (modifier < 0) {
				settings.soundOn = 0;
			} else {
				settings.soundOn = 1;
			}
			char sn[16];
			i = settings.soundOn;
			snprintf(sn, sizeof(sn), "1-SFX         %d", i);
			setCursor(4, 2);
			print(sn);
			break;

		case 3:
			if ((modifier < 0 && settings.obstacleCount > 1)
					|| (modifier > 0 && settings.obstacleCount < 6)) {
				settings.obstacleCount += modifier;
				setCursor(4, 3);
				char wl[16];
				i = settings.obstacleCount;
				snprintf(wl, sizeof(wl), "1-Wall Count  %d", i);
				print(wl);
			}
		default:
			break;
		}

		if (choice_indx == -1) {
			char hp[16];
			int i = settings.health;
			snprintf(hp, sizeof(hp), "1-Health      %d", i);
			clear();
			setCursor(4, 0);
			print(hp);
			setCursor(4, 1);
			char sp[16];
			i = settings.speed;
			snprintf(sp, sizeof(sp), "1-Speed       %d", i);
			print(sp);
			setCursor(4, 2);
			char sn[16];
			i = settings.speed;
			snprintf(sn, sizeof(sn), "1-SFX         %d", i);
			print(sn);
			setCursor(4, 3);
			char wl[16];
			i = settings.obstacleCount;
			snprintf(wl, sizeof(wl), "1-Wall Count  %d", i);
			print(wl);
			choice_indx = 0;
		}
		break;

	case MOD:
		if (choice_indx == -1) {
			clear();
			char sp[16];
			int i = settings.gameMode;
			snprintf(sp, sizeof(sp), "1-Game Mode   %d", i);
			setCursor(2, 1);
			print(sp);
			choice_indx = 0;
		} else {
			if ((modifier < 0 && settings.gameMode > 1)
					|| (modifier > 0 && settings.gameMode < 3)) {
				settings.gameMode += modifier;
				char sp[16];
				int i = settings.gameMode;
				snprintf(sp, sizeof(sp), "1-Game Mode   %d", i);
				setCursor(2, 1);
				print(sp);
			}
		}
		break;

	case START:
		start_game();
		break;
	}
}

void show_options() {
	createChar(0, body);
	createChar(3, apple);
	createChar(4, wall);
	createChar(5, heal);
	createChar(6, lucky_box);

	choice_indx = -1;
	currentOption = NONE;
	optionsEnumerator = ABOUT;
	clear();
	setCursor(4, 0);
	print("1-About Us");
	setCursor(4, 1);
	print("2-Settings");
	setCursor(4, 2);
	print("3-Game Mode");
	setCursor(4, 3);
	print("4-Start Game");
//create arrow
	byte arrow[8] = { 0x00, 0x00, 0x04, 0x02, 0x1F, 0x02, 0x04, 0x00 };
	createChar(2, arrow);
	gameState = OPTIONS;
}

void onDeath() {
    struct Node *current = snakeHead;
    while (current != NULL) {
        setCursor(current->point.x, current->point.y);
        print(" ");
        current = current->next;
        HAL_Delay(50);
    }
}

void gameover() {
	onDeath();


	if (settings.soundOn) {
		isSfx = 3;
	}

	game_ready = 0;
	is_dead = 1;
	volume = 1;
	int score = currentScore;
	frame = 0;

	RTC_TimeTypeDef my_time;
	HAL_RTC_GetTime(&hrtc, &my_time, RTC_FORMAT_BIN);

	clear();

	char msg[20];
	sprintf(msg, "Score: %d", score);
	setCursor(2, 1);
	print(msg);
	setCursor(2, 2);
	print(username);

	setCursor(14, 1);
	print("GAME");
	setCursor(14, 2);
	print("OVER");
	set_music(6);

	char time_string[20];
	int hour = my_time.Hours;
	int min = my_time.Minutes;
	int sec = my_time.Seconds;
	snprintf(time_string, sizeof(time_string), "DIED AT (%02d:%02d:%02d)\n",
			hour, min, sec);

	HAL_UART_Transmit(&huart1, time_string, 20, HAL_MAX_DELAY);

}

void start_game() {
// Reset game-specific variables
	for (int i = 0; i < 4; i++) {
		carrier[i] = 0;
	}
	effect = NOEFFECT;
	currentHealth = settings.health;
	currentScore = 0;
	volume = 1;
	set_music(3);
	in_menu = 0;
	is_dead = 0;
	game_paused = 0;
	paused = 0;
	frame = settings.appleSpawnTime;
	prev_frame = 0;
	direction = RIGHT;
	prevDirection = UP;
	box.x = -1;
	box.y = -1;
	medkit.x = -1;
	medkit.y = -1;

// Set movement cooldown based on speed setting
	if (settings.speed == 1) {
		movementCoolDown = 2000;
		settings.appleSpawnTime = 7;
	} else if (settings.speed == 2) {
		movementCoolDown = 1000;
		settings.appleSpawnTime = 5;
	} else if (settings.speed == 3) {
		movementCoolDown = 50;
		settings.appleSpawnTime = 3;
	}
	frame = settings.appleSpawnTime;

	if (settings.gameMode == 1) {
		settings.wallCollision = 0;
	} else if (settings.gameMode == 2) {
		settings.wallCollision = 1;
	} else if (settings.gameMode == 3) {
		settings.wallCollision = 0;
	}

// Initialize the snake
	deleteSnakeExceptHead(&snakeHead);
	snakeHead = createNode(5, 0);
	append(&snakeHead, 4, 0);
	append(&snakeHead, 3, 0);
	append(&snakeHead, 2, 0);
	append(&snakeHead, 1, 0);

// Generate the first apple
	generateApple();

// Clear the display and reset game state
	clear();
	currentOption = NONE;
	optionsEnumerator = ABOUT;
	gameState = GAME;
	createChar(1, head[2]);

// Indicate the game is ready
	game_ready = 1;
	RTC_TimeTypeDef my_time;
	HAL_RTC_GetTime(&hrtc, &my_time, RTC_FORMAT_BIN);
	start_time.hour = my_time.Hours;
	start_time.minute = my_time.Minutes;
	start_time.second = my_time.Seconds;

	speed_increase_time = start_time;
	apple_spawn_time.hour = my_time.Hours;
	apple_spawn_time.minute = my_time.Minutes;
	apple_spawn_time.second = my_time.Seconds + 7;

	heal_despawn_time = start_time;
	heal_spawn_time = start_time;
	box_spawn_time = start_time;
	box_despawn_time = start_time;
	wall_spawn_time = start_time;

// Draw the initial apple position
	generateWalls(settings.obstacleCount, 0);
	generateApple();
}

void menu() {
	for (int i = 0; i < 4; i++) {
		carrier[i] = 0;
	}
	game_ready = 0;
	paused = 0;
	volume = 1;
	in_menu = 1;
	is_dead = 1;
	currentOption = NONE;
	optionsEnumerator = ABOUT;
	gameState = MENU;
	LiquidCrystal(GPIOD, GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_11,
	GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_14);
	begin(20, 4);
	setCursor(4, 1);
	print("HONEST SNAKE");
	createChar(1, head[1]);
	setCursor(10, 2);
	write(1);
	set_music(1);

}

int to_seconds(const struct RTC_Time time) {
	return time.hour * 3600 + time.minute * 60 + time.second;
}

int calculate_time_difference(const struct RTC_Time start,
		const struct RTC_Time end) {
	int start_seconds = to_seconds(start);
	int end_seconds = to_seconds(end);
	return end_seconds - start_seconds;
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
	/* USER CODE BEGIN 1 */
	struct digit _digits[10];
	_digits[0].number = 0;
	_digits[0].anti_pattern = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14
			| GPIO_PIN_15;

	_digits[1].number = 1;
	_digits[1].pattern = GPIO_PIN_12;
	_digits[1].anti_pattern = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;

	_digits[2].number = 2;
	_digits[2].pattern = GPIO_PIN_13;
	_digits[2].anti_pattern = GPIO_PIN_12 | GPIO_PIN_14 | GPIO_PIN_15;

	_digits[3].number = 3;
	_digits[3].pattern = GPIO_PIN_12 | GPIO_PIN_13;
	_digits[3].anti_pattern = GPIO_PIN_14 | GPIO_PIN_15;

	_digits[4].number = 4;
	_digits[4].pattern = GPIO_PIN_14;
	_digits[4].anti_pattern = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_15;

	_digits[5].number = 5;
	_digits[5].pattern = GPIO_PIN_12 | GPIO_PIN_14;
	_digits[5].anti_pattern = GPIO_PIN_13 | GPIO_PIN_15;

	_digits[6].number = 6;
	_digits[6].pattern = GPIO_PIN_13 | GPIO_PIN_14;
	_digits[6].anti_pattern = GPIO_PIN_12 | GPIO_PIN_15;

	_digits[7].number = 7;
	_digits[7].pattern = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14;
	_digits[7].anti_pattern = GPIO_PIN_15;

	_digits[8].number = 8;
	_digits[8].pattern = GPIO_PIN_15;
	_digits[8].anti_pattern = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14;

	_digits[9].number = 9;
	_digits[9].pattern = GPIO_PIN_12 | GPIO_PIN_15;
	_digits[9].anti_pattern = GPIO_PIN_13 | GPIO_PIN_14;

//initialize Global digits structure
	for (int i = 0; i < 10; i++) {
		digits[i] = _digits[i];
	}

	Point negative;
	negative.x = -1;
	negative.y = -1;

	for (int i = 0; i < 6; i++) {
		wallPositions[i] = negative;
	}

	settings = (struct Settings ) { 0, 3, 1, 1, 4, 1, 10000 };
	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */
//Set 7-segment PINS
	led[0] = GPIO_PIN_4;
	led[1] = GPIO_PIN_5;
	led[2] = GPIO_PIN_6;
	led[3] = GPIO_PIN_7;

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_I2C1_Init();
	MX_SPI1_Init();
	MX_USB_PCD_Init();
	MX_TIM1_Init();
	MX_TIM2_Init();
	MX_RTC_Init();
	MX_USART1_UART_Init();
	MX_TIM3_Init();
	/* USER CODE BEGIN 2 */
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
	HAL_TIM_Base_Start_IT(&htim1);
	HAL_TIM_Base_Start_IT(&htim3);
	initTonesDictionary();
	PWM_Start();
	menu();
	HAL_UART_Transmit_IT(&huart1,
			"=============\nProgram Running\n=============\n", 44);
	HAL_UART_Receive_IT(&huart1, &d, 1);
	init_display();

	RTC_TimeTypeDef my_time;
	my_time.Hours = 0;
	my_time.Minutes = 0;
	my_time.Seconds = 0;
	HAL_RTC_SetTime(&hrtc, &my_time, RTC_FORMAT_BIN);

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */

//		if (in_menu) {
//			setCursor(1, 2);
//			print("press s8 to start");
//			HAL_Delay(1000);
//			setCursor(1, 2);
//			print("                 ");
//			HAL_Delay(200);
//		}
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
	RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI
			| RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.LSIState = RCC_LSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
		Error_Handler();
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB
			| RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_I2C1 | RCC_PERIPHCLK_RTC
			| RCC_PERIPHCLK_TIM1;
	PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
	PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
	PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
	PeriphClkInit.USBClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
	PeriphClkInit.Tim1ClockSelection = RCC_TIM1CLK_HCLK;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void) {

	/* USER CODE BEGIN I2C1_Init 0 */

	/* USER CODE END I2C1_Init 0 */

	/* USER CODE BEGIN I2C1_Init 1 */

	/* USER CODE END I2C1_Init 1 */
	hi2c1.Instance = I2C1;
	hi2c1.Init.Timing = 0x2000090E;
	hi2c1.Init.OwnAddress1 = 0;
	hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c1.Init.OwnAddress2 = 0;
	hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
	hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
		Error_Handler();
	}

	/** Configure Analogue filter
	 */
	if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE)
			!= HAL_OK) {
		Error_Handler();
	}

	/** Configure Digital filter
	 */
	if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN I2C1_Init 2 */

	/* USER CODE END I2C1_Init 2 */

}

/**
 * @brief RTC Initialization Function
 * @param None
 * @retval None
 */
static void MX_RTC_Init(void) {

	/* USER CODE BEGIN RTC_Init 0 */

	/* USER CODE END RTC_Init 0 */

	RTC_TimeTypeDef sTime = { 0 };
	RTC_DateTypeDef sDate = { 0 };

	/* USER CODE BEGIN RTC_Init 1 */

	/* USER CODE END RTC_Init 1 */

	/** Initialize RTC Only
	 */
	hrtc.Instance = RTC;
	hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
	hrtc.Init.AsynchPrediv = 39;
	hrtc.Init.SynchPrediv = 999;
	hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
	hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
	hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
	if (HAL_RTC_Init(&hrtc) != HAL_OK) {
		Error_Handler();
	}

	/* USER CODE BEGIN Check_RTC_BKUP */

	/* USER CODE END Check_RTC_BKUP */

	/** Initialize RTC and set the Time and Date
	 */
	sTime.Hours = 0x0;
	sTime.Minutes = 0x0;
	sTime.Seconds = 0x0;
	sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	sTime.StoreOperation = RTC_STOREOPERATION_RESET;
	if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK) {
		Error_Handler();
	}
	sDate.WeekDay = RTC_WEEKDAY_MONDAY;
	sDate.Month = RTC_MONTH_JULY;
	sDate.Date = 0x3;
	sDate.Year = 0x24;

	if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN RTC_Init 2 */

	/* USER CODE END RTC_Init 2 */

}

/**
 * @brief SPI1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI1_Init(void) {

	/* USER CODE BEGIN SPI1_Init 0 */

	/* USER CODE END SPI1_Init 0 */

	/* USER CODE BEGIN SPI1_Init 1 */

	/* USER CODE END SPI1_Init 1 */
	/* SPI1 parameter configuration*/
	hspi1.Instance = SPI1;
	hspi1.Init.Mode = SPI_MODE_MASTER;
	hspi1.Init.Direction = SPI_DIRECTION_2LINES;
	hspi1.Init.DataSize = SPI_DATASIZE_4BIT;
	hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi1.Init.NSS = SPI_NSS_SOFT;
	hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
	hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi1.Init.CRCPolynomial = 7;
	hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
	hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
	if (HAL_SPI_Init(&hspi1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN SPI1_Init 2 */

	/* USER CODE END SPI1_Init 2 */

}

/**
 * @brief TIM1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM1_Init(void) {

	/* USER CODE BEGIN TIM1_Init 0 */

	/* USER CODE END TIM1_Init 0 */

	TIM_ClockConfigTypeDef sClockSourceConfig = { 0 };
	TIM_MasterConfigTypeDef sMasterConfig = { 0 };

	/* USER CODE BEGIN TIM1_Init 1 */

	/* USER CODE END TIM1_Init 1 */
	htim1.Instance = TIM1;
	htim1.Init.Prescaler = 4799;
	htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim1.Init.Period = 1;
	htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim1.Init.RepetitionCounter = 0;
	htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim1) != HAL_OK) {
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN TIM1_Init 2 */

	/* USER CODE END TIM1_Init 2 */

}

/**
 * @brief TIM2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM2_Init(void) {

	/* USER CODE BEGIN TIM2_Init 0 */

	/* USER CODE END TIM2_Init 0 */

	TIM_ClockConfigTypeDef sClockSourceConfig = { 0 };
	TIM_MasterConfigTypeDef sMasterConfig = { 0 };
	TIM_OC_InitTypeDef sConfigOC = { 0 };

	/* USER CODE BEGIN TIM2_Init 1 */

	/* USER CODE END TIM2_Init 1 */
	htim2.Instance = TIM2;
	htim2.Init.Prescaler = 0;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = 4294967295;
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK) {
		Error_Handler();
	}
	if (HAL_TIM_PWM_Init(&htim2) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = 0;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1)
			!= HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN TIM2_Init 2 */

	/* USER CODE END TIM2_Init 2 */
	HAL_TIM_MspPostInit(&htim2);

}

/**
 * @brief TIM3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM3_Init(void) {

	/* USER CODE BEGIN TIM3_Init 0 */

	/* USER CODE END TIM3_Init 0 */

	TIM_ClockConfigTypeDef sClockSourceConfig = { 0 };
	TIM_MasterConfigTypeDef sMasterConfig = { 0 };

	/* USER CODE BEGIN TIM3_Init 1 */

	/* USER CODE END TIM3_Init 1 */
	htim3.Instance = TIM3;
	htim3.Init.Prescaler = 4799;
	htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim3.Init.Period = 5;
	htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim3) != HAL_OK) {
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN TIM3_Init 2 */

	/* USER CODE END TIM3_Init 2 */

}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void) {

	/* USER CODE BEGIN USART1_Init 0 */

	/* USER CODE END USART1_Init 0 */

	/* USER CODE BEGIN USART1_Init 1 */

	/* USER CODE END USART1_Init 1 */
	huart1.Instance = USART1;
	huart1.Init.BaudRate = 115200;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_UART_Init(&huart1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USART1_Init 2 */

	/* USER CODE END USART1_Init 2 */

}

/**
 * @brief USB Initialization Function
 * @param None
 * @retval None
 */
static void MX_USB_PCD_Init(void) {

	/* USER CODE BEGIN USB_Init 0 */

	/* USER CODE END USB_Init 0 */

	/* USER CODE BEGIN USB_Init 1 */

	/* USER CODE END USB_Init 1 */
	hpcd_USB_FS.Instance = USB;
	hpcd_USB_FS.Init.dev_endpoints = 8;
	hpcd_USB_FS.Init.speed = PCD_SPEED_FULL;
	hpcd_USB_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
	hpcd_USB_FS.Init.low_power_enable = DISABLE;
	hpcd_USB_FS.Init.battery_charging_enable = DISABLE;
	if (HAL_PCD_Init(&hpcd_USB_FS) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USB_Init 2 */

	/* USER CODE END USB_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(CS_I2C_SPI_GPIO_Port, CS_I2C_SPI_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
			GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB,
	GPIO_PIN_10 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15,
			GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOD,
			GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12
					| GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15 | GPIO_PIN_4
					| GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_RESET);

	/*Configure GPIO pin : CS_I2C_SPI_Pin */
	GPIO_InitStruct.Pin = CS_I2C_SPI_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(CS_I2C_SPI_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : MEMS_INT3_Pin MEMS_INT4_Pin */
	GPIO_InitStruct.Pin = MEMS_INT3_Pin | MEMS_INT4_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

	/*Configure GPIO pins : PC0 PC1 PC2 PC3 */
	GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pin : B1_Pin */
	GPIO_InitStruct.Pin = B1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : PB10 PB12 PB13 PB14
	 PB15 */
	GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14
			| GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pins : PD8 PD9 PD10 PD11
	 PD12 PD13 PD14 PD15
	 PD4 PD5 PD6 PD7 */
	GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11
			| GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15 | GPIO_PIN_4
			| GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	/*Configure GPIO pins : PD0 PD1 PD2 PD3 */
	GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	/* EXTI interrupt init*/
	HAL_NVIC_SetPriority(EXTI0_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(EXTI0_IRQn);

	HAL_NVIC_SetPriority(EXTI1_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(EXTI1_IRQn);

	HAL_NVIC_SetPriority(EXTI2_TSC_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(EXTI2_TSC_IRQn);

	HAL_NVIC_SetPriority(EXTI3_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(EXTI3_IRQn);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
