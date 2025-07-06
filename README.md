# Moneybot

Moneybot is a high-performance, modular cryptocurrency trading bot written in C++. It is designed for real-time trading on exchanges via WebSocket APIs, with a focus on extensibility, robust order management, and advanced trading strategies.

## Features

- **Modular Architecture**: Clean separation of concerns with components for networking, order book management, strategies, logging, and risk management.
- **WebSocket Connectivity**: Asynchronous, SSL-secured WebSocket connections to crypto exchanges using Boost.Asio and Boost.Beast.
- **Order Book Management**: Real-time order book updates and trade processing.
- **Strategy Support**: Pluggable strategy interface for implementing custom trading logic (e.g., market making).
- **Robust Logging**: Integrated logging using spdlog for debugging and monitoring.
- **Configurable**: JSON-based configuration for exchange endpoints, credentials, and strategy parameters.
- **Docker Support**: Dockerfile and compose files for easy deployment and development.

## Project Structure

```
.
├── src/                # Source code (core logic, strategies, networking)
├── include/            # Header files
├── lib/                # Third-party libraries (e.g., nlohmann/json, spdlog)
├── build/              # Build artifacts
├── data/               # Data files (e.g., ticks.db)
├── logs/               # Log files
├── config.json         # Main configuration file
├── Dockerfile          # Docker build file
├── docker-compose.yml  # Docker Compose setup
├── README.md           # Project documentation
└── CMakeLists.txt      # CMake build configuration
```

## Key Components

- **Network**: Handles secure WebSocket connections, message parsing, and subscription to exchange data streams.
- **OrderBook**: Maintains the current state of the market's order book and processes updates.
- **Strategy**: Base class for trading strategies; includes a sample market maker strategy.
- **Logger**: Centralized logging facility.
- **RiskManager**: Monitors and enforces trading risk limits.
- **OrderManager**: Handles order placement, tracking, and execution reports.

## Getting Started

### Prerequisites

- C++17 or later
- CMake
- Boost (Asio, Beast)
- OpenSSL
- spdlog
- nlohmann/json

### Build Instructions

```sh
mkdir build
cd build
cmake ..
make
```

Or use the provided Docker setup:

```sh
docker-compose up --build
```

### Configuration

Edit `config.json` to set exchange endpoints, API keys, and strategy parameters.

### Running Moneybot

```sh
./build/moneybot
```

Or via Docker:

```sh
docker-compose up
```

## Graphical Dashboard (ImGui GUI)

MoneyBot now includes a modern real-time dashboard built with Dear ImGui. The GUI displays live trading stats, order book, connection status, and more.

### Prerequisites (for GUI)

- All core prerequisites (see above)
- [Dear ImGui](https://github.com/ocornut/imgui) (source in `lib/imgui`)
- [GLFW](https://www.glfw.org/) (install via Homebrew: `brew install glfw`)
- OpenGL (system-provided on macOS)

### Building the GUI

```sh
cmake -S . -B build
cmake --build build
```

### Running the GUI

```sh
./build/moneybot_gui
```

#### macOS OpenGL/GLSL Troubleshooting

- The GUI is configured to use OpenGL 2.1 and GLSL 120 for maximum compatibility on macOS.
- If you see shader errors, ensure you have the correct GLSL version in `src/gui_main.cpp`:
  ```cpp
  const char* glsl_version = "#version 120";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  ```
- Make sure you have installed GLFW via Homebrew and that `/opt/homebrew/lib` is in your library path.

## Extending Moneybot

- Add new strategies by inheriting from the `Strategy` base class.
- Implement new exchange adapters by extending the `Network` component.
- Use the logging and risk management facilities for robust, safe trading.

## License

This project is licensed under the MIT License.