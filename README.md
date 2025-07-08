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

#### Console Application
```sh
mkdir build
cd build
cmake ..
make moneybot
```

#### GUI Application
```sh
mkdir build
cd build
cmake ..
make moneybot_gui
```

#### Build Both Targets
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

#### Standard Trading Mode
```sh
./build/moneybot
```

#### Backtesting Mode
Run historical simulations with your strategy and data:
```sh
./build/moneybot --backtest [ticks.db]
```
If no file is given, defaults to `data/ticks.db`.

#### Analysis Mode
Analyze historical data (spread, VWAP, tick count):
```sh
./build/moneybot --analyze
```

#### Dry-Run Mode
Simulate live trading without placing real orders:
```sh
./build/moneybot --dry-run
```

#### With Docker
```sh
docker-compose up
```


## Advanced GUI Dashboard

MoneyBot features a modern, modular dashboard built with Dear ImGui, providing real-time visualization of trading data and system metrics.

### Dashboard Features

- **Metrics Widget**: Displays trading performance, P&L, active orders, and market statistics
- **Order Book Widget**: Real-time bid/ask visualization with color-coded price levels
- **Modular Architecture**: Clean separation of dashboard components for maintainability
- **Real-time Updates**: Live data streaming from the trading engine

### GUI Architecture

The dashboard is built using modular widgets:

- `dashboard_metrics_widget.h/cpp`: Trading metrics and performance display
- `dashboard_orderbook_widget.h/cpp`: Order book visualization
- `gui_main.cpp`: Main GUI loop and window management

### Building and Running the GUI

#### Prerequisites (macOS)
```sh
# Install GLFW via Homebrew
brew install glfw

# Ensure ImGui is available (already included in lib/imgui)
```

#### Build and Run
```sh
# Build the GUI application
cmake -S . -B build
cmake --build build --target moneybot_gui

# Run the dashboard
./build/moneybot_gui
```

### Running CLI and GUI Together
Start each in a separate terminal for full functionality:
```sh
# Terminal 1: Start the trading engine
./build/moneybot

# Terminal 2: Start the GUI dashboard
./build/moneybot_gui
```

### macOS Configuration Notes

- Uses OpenGL 3.3 Core Profile with GLSL 330 for optimal performance
- Automatically silences OpenGL deprecation warnings
- Configured for Apple Silicon (ARM64) and Intel compatibility
- Uses absolute config paths for reliable operation


## Backtesting & Strategy Development

- Use `--backtest` to simulate strategies on historical data.
- Use `--analyze` for quick data/statistics reports.
- Add new strategies by inheriting from the `Strategy` base class (see `DummyStrategy` for a minimal example).
- The strategy factory (`strategy_factory.h/.cpp`) allows dynamic instantiation from config.
- Implement new exchange adapters by extending the `Network` component.
- Use the logging and risk management facilities for robust, safe trading.

## License

This project is licensed under the MIT License.