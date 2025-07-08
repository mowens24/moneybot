# Moneybot

Moneybot is a high-performance, modular cryptocurrency trading bot written in C++. It is designed for real-time trading on exchanges via WebSocket APIs, with a focus on extensibility, robust order management, and advanced trading strategies.

## Features

- **Modular Architecture**: Clean separation of concerns with components for networking, order book management, strategies, logging, and risk management.
- **WebSocket Connectivity**: Asynchronous, SSL-secured WebSocket connections to crypto exchanges using Boost.Asio and Boost.Beast.
- **Order Book Management**: Real-time order book updates and trade processing.
- **Strategy Support**: Pluggable strategy interface for implementing custom trading logic (e.g., market making).
- **Advanced GUI Dashboard**: Modular ImGui-based interface with real-time metrics and order book visualization.
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
- **GUI Dashboard**: Modular ImGui-based interface with separate widgets for metrics and order book visualization.

## Getting Started

### Prerequisites

- C++17 or later
- CMake
- Boost (Asio, Beast)
- OpenSSL
- spdlog
- nlohmann/json
- GLFW3 (for GUI)
- OpenGL (for GUI)

### Build Instructions

#### Combined Application (Console + GUI)
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


### Running MoneyBot

#### Console Mode (Default)
```sh
./build/moneybot
```

#### GUI Dashboard Mode
```sh
./build/moneybot --gui
# or
./build/moneybot -g
```

#### Other Options
```sh
# Analysis mode
./build/moneybot --analyze

# Backtesting mode
./build/moneybot --backtest [ticks.db]

# Dry-run mode
./build/moneybot --dry-run

# Help
./build/moneybot --help
```

#### With Docker
```sh
docker-compose up
```


## Advanced GUI Dashboard

MoneyBot features a modern, modular dashboard built with Dear ImGui, accessible via command line flags for seamless integration with console operations.

### Dashboard Features

- **Metrics Widget**: Displays trading performance, P&L, active orders, and market statistics
- **Order Book Widget**: Real-time bid/ask visualization with color-coded price levels
- **Modular Architecture**: Clean separation of dashboard components for maintainability
- **Real-time Updates**: Live data streaming from the trading engine
- **Integrated Design**: Single executable with both console and GUI modes

### GUI Architecture

The dashboard is built using modular widgets within a unified application:

- `dashboard_metrics_widget.h/cpp`: Trading metrics and performance display
- `dashboard_orderbook_widget.h/cpp`: Order book visualization
- `gui_main.cpp`: GUI mode implementation (called via `--gui` flag)
- `main.cpp`: Unified entry point with mode detection

### Usage

#### GUI Mode
```sh
# Launch GUI dashboard
./build/moneybot --gui
./build/moneybot -g
```

#### Console Mode (Default)
```sh
# Standard console operation
./build/moneybot
./build/moneybot --analyze
./build/moneybot --backtest
./build/moneybot --dry-run
```

### Prerequisites (for GUI mode)

#### macOS
```sh
# Install GLFW via Homebrew
brew install glfw

# ImGui is included in lib/imgui (no additional installation needed)
```

### Configuration Notes

- **Single executable**: Both console and GUI modes in one binary
- **Command-line driven**: No separate GUI executable to manage  
- **Shared configuration**: Both modes use the same `config.json`
- **macOS optimized**: Uses OpenGL 3.3 Core Profile with GLSL 330
- **Cross-platform**: Supports both Apple Silicon (ARM64) and Intel architectures


## Backtesting & Strategy Development

- Use `--backtest` to simulate strategies on historical data.
- Use `--analyze` for quick data/statistics reports.
- Add new strategies by inheriting from the `Strategy` base class (see `DummyStrategy` for a minimal example).
- The strategy factory (`strategy_factory.h/.cpp`) allows dynamic instantiation from config.
- Implement new exchange adapters by extending the `Network` component.
- Use the logging and risk management facilities for robust, safe trading.

## License

This project is licensed under the MIT License.