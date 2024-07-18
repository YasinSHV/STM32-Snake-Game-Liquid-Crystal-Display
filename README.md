# STM32 Snake Game on Liquid Crystal Display

This repository contains the implementation of a classic Snake game designed for a 20x4 Liquid Crystal Display (LCD) using C and STM32CubeIDE. The game logic utilizes a linked list to handle the dynamic size of the snake, ensuring efficient and seamless animations.

## Features

- **Dynamic Snake Size:** The snake's size is managed using a linked list, allowing it to grow as it consumes food.
- **Efficient Drawing:** Each frame, the game updates only the necessary parts of the screen (head, neck, and tail), ensuring maximum efficiency.
- **Point Structure:** All game objects are represented as `Point` structures, enabling linear time checks.
- **Interrupt-Driven:** The entire game logic is handled using interrupts, avoiding the use of the `while(1)` loop for better performance and responsiveness.
- **Seamless Gameplay:** Despite not using FreeRTOS, this implementation ensures smooth and seamless gameplay.

## Files

- `dictionary.h` / `dictionary.c`: Contains helper functions and data structures.
- `LiquidCrystal.h`: Manages the communication with the LCD.
- `main.h` / `main.c`: The main entry point and game logic.
- `music_library.h`: Contains functions and definitions for game sounds.
- `stm32f3xx_hal_conf.h`: HAL configuration file.
- `stm32f3xx_it.h` / `stm32f3xx_it.c`: Interrupt handlers for the game.

## How It Works

1. **Snake Logic:** The snake is represented using a linked list of `Point` structures. This allows the snake to grow and shrink dynamically as it moves and consumes food.
2. **Drawing Logic:** Only the snake's head, neck, and the last position of the tail are updated on the LCD each frame. This minimizes the number of updates required, ensuring smooth animations.
3. **Interrupt-Driven Execution:** The game logic is executed using interrupts, providing a responsive and efficient gaming experience without relying on a continuous loop.

## Getting Started

1. Clone this repository.
2. Open the project in STM32CubeIDE.
3. Build and flash the firmware to your STM32 microcontroller.
4. Connect the 20x4 LCD to the microcontroller as per the provided schematics.
5. Enjoy the game!
