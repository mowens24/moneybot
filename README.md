# MoneyBot - Production-Ready Cryptocurrency Trading System

MoneyBot is a high-performance, production-ready cryptocurrency trading bot written in C++. It features an advanced Market Maker strategy with dynamic spread calculation, inventory management, and comprehensive risk controls for automated trading on crypto exchanges.

## 🚀 Key Features

- **Advanced Market Making Strategy**: Dynamic spread calculation based on volatility, inventory skewing, and real-time position management
- **Production-Ready Architecture**: Modular design with robust error handling, graceful shutdown, and comprehensive logging
- **Multiple Operation Modes**: Live trading, dry-run testing, backtesting, and data analysis
- **Real-Time Monitoring**: Clean status updates with PnL tracking, trade metrics, and connection monitoring
- **WebSocket Connectivity**: Asynchronous, SSL-secured connections to crypto exchanges using Boost.Asio and Boost.Beast
- **Risk Management**: Emergency stops, position limits, drawdown controls, and order rate limiting
- **Advanced GUI Dashboard**: Optional ImGui-based interface with real-time metrics and order book visualization
- **Comprehensive Configuration**: JSON-based configuration for strategies, risk parameters, and exchange settings
- **Docker Support**: Containerized deployment for easy scaling and management

## 📊 Market Making Strategy Features

### Enhanced Algorithm
- **Dynamic Spread Calculation**: Adjusts spreads based on market volatility (10-100 bps range)
- **Inventory Management**: Intelligent position skewing to maintain balanced inventory
- **Volatility Tracking**: 100-tick rolling window for real-time volatility calculation
- **Order Refresh Logic**: Smart order replacement with 2-second intervals
- **Risk Integration**: Real-time position and drawdown monitoring

### Configuration Parameters
```json
{
  "strategy": {
    "config": {
      "base_spread_bps": 10.0,       // Base spread in basis points
      "max_spread_bps": 100.0,       // Maximum spread cap
      "min_spread_bps": 2.0,         // Minimum spread floor
      "inventory_skew_factor": 3.0,   // Position-based price skewing
      "volatility_multiplier": 2.0,   // Volatility adjustment factor
      "refresh_interval_ms": 2000,    // Order refresh frequency
      "rebalance_threshold": 0.005    // Position rebalancing trigger
    }
  }
}
```

## 🏗️ Project Structure

```
.
├── src/                # Source code (core logic, strategies, networking)
│   ├── main.cpp        # Unified entry point with multiple operation modes
│   ├── moneybot.cpp    # Core trading engine implementation
│   ├── market_maker_strategy.cpp  # Advanced market making algorithm
│   ├── strategy_factory.cpp       # Strategy instantiation factory
│   ├── backtest_engine.cpp        # Historical data simulation
│   ├── network.cpp     # WebSocket connectivity and market data
│   ├── order_manager.cpp          # Order placement and tracking
│   ├── risk_manager.cpp           # Risk controls and position monitoring
│   └── gui_main.cpp    # GUI dashboard implementation
├── include/            # Header files with clean interfaces
├── lib/                # Third-party libraries (json, spdlog, ImGui)
├── build/              # Build artifacts
├── data/               # Market data files and databases
├── logs/               # Trading and system logs
├── config.json         # Main configuration file
├── Dockerfile          # Production containerization
├── docker-compose.yml  # Multi-service deployment
└── CMakeLists.txt      # Modern CMake build system
```

## 🎯 Key Components

- **TradingEngine**: Main orchestrator for live trading operations
- **MarketMakerStrategy**: Advanced market making with dynamic pricing
- **Network**: Secure WebSocket connections with automatic reconnection
- **OrderBook**: Real-time market depth tracking and updates
- **RiskManager**: Position limits, drawdown controls, and emergency stops
- **OrderManager**: Order lifecycle management and execution tracking
- **BacktestEngine**: Historical simulation with performance metrics
- **Logger**: High-performance structured logging
- **GUI Dashboard**: Real-time trading metrics and order book visualization

## 🚀 Getting Started

### Prerequisites

- C++17 or later
- CMake 3.15+
- Boost (Asio, Beast) 1.70+
- OpenSSL 1.1+
- spdlog 1.8+
- nlohmann/json 3.9+
- GLFW3 (for GUI mode)
- OpenGL 3.3+ (for GUI mode)

### Quick Build

```bash
# Clone and build
git clone <repository>
cd moneybot
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Docker Deployment

```bash
# Production deployment
docker-compose up --build -d

# Development with logs
docker-compose -f compose-dev.yaml up --build
```

## 💻 Usage

### Command Line Options

```bash
# Production trading (requires API keys in config.json)
./build/moneybot

# Safe testing without real orders
./build/moneybot --dry-run

# Historical backtesting
./build/moneybot --backtest [data_file.json]

# Market data analysis
./build/moneybot --analyze

# GUI dashboard mode  
./build/moneybot --gui

# Custom configuration
./build/moneybot --config custom_config.json

# Help and options
./build/moneybot --help
```

### Live Trading Mode

MoneyBot displays clean, periodic status updates every 10 seconds:

```
=== MoneyBot HFT Trading System ===
Strategy: market_maker
Symbol: BTCUSDT
Exchange: https://api.binance.us
Press Ctrl+C to stop

=== Status Update ===
Connection: 🟢 Connected
Running: Yes
Uptime: 120 seconds
Total PnL: $45.67
Total Trades: 23
Avg PnL per Trade: $1.99
Best Bid: 43250.50 | Best Ask: 43251.00 | Spread: 0.50
Emergency Stop: No
Drawdown: 0.15%
---
```


## 🛡️ Safety & Risk Management

### Built-in Risk Controls
- **Position Limits**: Maximum position size and order size enforcement
- **Drawdown Protection**: Automatic trading halt on excessive losses
- **Order Rate Limiting**: Prevents exchange API abuse
- **Emergency Stop**: Manual or automatic trading suspension
- **Graceful Shutdown**: Clean exit on SIGINT/SIGTERM signals

### Configuration Example
```json
{
  "risk": {
    "max_position_size": 0.1,      // Maximum position (BTC)
    "max_order_size": 0.01,        // Maximum single order
    "max_daily_loss": -100.0,      // Daily loss limit ($)
    "max_drawdown": -50.0,         // Drawdown limit ($)
    "max_orders_per_minute": 60,   // Rate limiting
    "min_spread": 0.0001,          // Minimum viable spread
    "max_slippage": 0.001          // Slippage tolerance
  }
}
```

## 📈 Performance Monitoring

### Real-Time Metrics
- **PnL Tracking**: Real-time profit/loss calculation
- **Trade Statistics**: Count, win/loss ratio, average PnL
- **Market Data**: Best bid/ask, spread monitoring
- **Connection Status**: WebSocket connectivity health
- **Risk Status**: Emergency stop state, drawdown levels

### Logging
- **Structured Logging**: JSON-formatted logs with timestamps
- **Log Rotation**: Automatic file rotation and archival
- **Multiple Levels**: Debug, info, warning, error classification
- **Performance Logs**: Trade execution and latency metrics

## 🧪 Testing & Development

### Backtesting
```bash
# Create test data
echo '{"timestamp": 1640995200000, "symbol": "BTCUSDT", "price": 50000.0, "quantity": 0.1, "side": "buy"}' > test_data.json

# Run backtest
./build/moneybot --backtest test_data.json

# Output:
# === Backtest Mode ===
# Loading data from: test_data.json
# Backtest complete!
# Total PnL: $0.00
# Max Drawdown: 0%
# Total Trades: 1
# Win/Loss: 0/0
# Sharpe Ratio: 0
```

### Strategy Development
1. **Inherit from Strategy**: Create new strategies by extending the base `Strategy` class
2. **Factory Registration**: Add new strategies to `strategy_factory.cpp`
3. **Configuration**: Define strategy parameters in `config.json`
4. **Testing**: Use dry-run and backtest modes for validation

### Development Workflow
```bash
# Edit strategy
vim src/market_maker_strategy.cpp

# Build and test
make && ./build/moneybot --dry-run

# Backtest
./build/moneybot --backtest data/test_ticks.json

# Deploy
docker-compose up --build -d
```

## 🖥️ GUI Dashboard (Optional)

### Features
- **Real-time Metrics**: Live PnL, trade count, and performance statistics
- **Order Book Visualization**: Color-coded bid/ask levels with depth
- **Modular Design**: Separate widgets for different functionality
- **Single Executable**: Unified binary with command-line mode switching

### Usage
```bash
# Launch GUI dashboard
./build/moneybot --gui

# GUI with custom config
./build/moneybot --gui --config production.json
```

### GUI Architecture
- `gui_main.cpp`: Main GUI entry point and ImGui setup
- `dashboard_metrics_widget.h/cpp`: Trading performance display
- `dashboard_orderbook_widget.h/cpp`: Market depth visualization
- Shared configuration and data with console mode

## 🐳 Production Deployment

### Docker Configuration
```yaml
# docker-compose.yml
version: '3.8'
services:
  moneybot:
    build: .
    environment:
      - CONFIG_FILE=config.json
    volumes:
      - ./config.json:/app/config.json:ro
      - ./logs:/app/logs
    restart: unless-stopped
```

### Configuration Management
```json
{
  "exchange": {
    "rest_api": {
      "base_url": "https://api.binance.us",
      "api_key": "${BINANCE_API_KEY}",
      "secret_key": "${BINANCE_SECRET_KEY}"
    }
  },
  "logging": {
    "level": "info",
    "file": "logs/moneybot.log",
    "max_file_size": "10MB",
    "max_files": 5
  }
}
```

### Monitoring & Alerts
- **Log Analysis**: Structured JSON logs for monitoring systems
- **Health Checks**: WebSocket connectivity and trading status
- **Performance Metrics**: Export to Prometheus/Grafana
- **Error Handling**: Automatic restart on recoverable failures

## 📋 System Requirements

### Minimum Requirements
- **CPU**: 2 cores, 2.0GHz
- **RAM**: 512MB
- **Network**: Stable internet connection (<50ms latency to exchange)
- **Storage**: 1GB for logs and market data

### Recommended for Production
- **CPU**: 4+ cores, 3.0GHz+
- **RAM**: 2GB+
- **Network**: Dedicated VPS near exchange data centers
- **Storage**: SSD with 10GB+ free space
- **OS**: Ubuntu 20.04 LTS or Docker-compatible environment

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Development Guidelines
- Follow C++17 best practices
- Add unit tests for new features
- Update documentation
- Test with dry-run mode before production
- Use structured logging for debugging

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## ⚠️ Disclaimer

**Trading cryptocurrencies involves substantial risk of loss. This software is provided for educational and research purposes. Use at your own risk. The authors are not responsible for any financial losses.**

- Always test with dry-run mode first
- Start with small position sizes
- Monitor risk management settings
- Keep API keys secure
- Regular backups of configuration and logs