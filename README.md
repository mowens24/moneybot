# MoneyBot - Goldman Sachs-Level Multi-Asset Crypto Trading System

MoneyBot is a production-grade, institutional-quality cryptocurrency trading system written in C++. Featuring advanced multi-exchange arbitrage, statistical arbitrage, portfolio optimization, and comprehensive risk management - designed to compete with Wall Street's most sophisticated trading infrastructure.

## 🚀 Goldman Sachs-Level Features

### Multi-Asset Trading Architecture
- **Multi-Exchange Gateway**: Simultaneous trading across Binance, Coinbase, Kraken with unified order book aggregation
- **Cross-Exchange Arbitrage**: Real-time opportunity scanning with sub-100ms execution
- **Statistical Arbitrage**: Pairs trading, mean reversion, and cointegration strategies
- **Portfolio Optimization**: Dynamic rebalancing with Modern Portfolio Theory integration
- **Advanced Market Making**: Multi-venue liquidity provision with inventory optimization

### Institutional-Grade Infrastructure
- **Production-Ready Architecture**: Modular design with robust error handling, graceful shutdown, and comprehensive logging
- **Real-Time Risk Management**: Position limits, VaR calculations, drawdown controls across all exchanges
- **High-Performance Connectivity**: Asynchronous WebSocket connections with automatic failover
- **Advanced Analytics**: Real-time performance attribution, Sharpe ratio tracking, and trade analytics
- **Professional Monitoring**: Clean status updates with multi-venue PnL tracking and connection monitoring

## 📊 Multi-Asset Strategy Features

### Cross-Exchange Arbitrage
- **Real-Time Opportunity Detection**: Continuous scanning across exchanges for price discrepancies
- **Latency-Optimized Execution**: Sub-100ms order placement with confidence scoring
- **Risk-Adjusted Sizing**: Dynamic position sizing based on available liquidity and latency
- **Automatic Profit Taking**: Intelligent exit strategies with slippage protection

### Statistical Arbitrage Engine
- **Pairs Trading**: Cointegration-based mean reversion strategies
- **Z-Score Analysis**: Statistical significance testing for entry/exit signals  
- **Correlation Matrix**: Real-time correlation tracking across asset pairs
- **Kalman Filtering**: Advanced signal processing for noise reduction

### Portfolio Optimization
- **Modern Portfolio Theory**: Markowitz optimization with risk-return maximization
- **Dynamic Rebalancing**: Continuous portfolio weight adjustments
- **Multi-Asset Allocation**: Optimal capital distribution across crypto assets
- **Risk Parity**: Equal risk contribution portfolio construction

### Enhanced Market Making
- **Multi-Venue Liquidity**: Simultaneous market making across exchanges
- **Inventory Optimization**: Cross-exchange position balancing
- **Dynamic Spreads**: Volatility and inventory-adjusted pricing
- **Smart Order Routing**: Optimal execution across venue ecosystem

### Configuration Parameters
```json
{
  "strategy": {
    "type": "multi_asset",
    "mode": "goldman_sachs_level"
  },
  "multi_asset": {
    "exchanges": [
      {"name": "binance", "enabled": true, "weight": 0.4},
      {"name": "coinbase", "enabled": true, "weight": 0.35},
      {"name": "kraken", "enabled": true, "weight": 0.25}
    ]
  },
  "strategies": {
    "cross_exchange_arbitrage": {
      "enabled": true,
      "min_profit_bps": 15.0,
      "max_position_size": 1.0,
      "latency_threshold_ms": 100
    },
    "statistical_arbitrage": {
      "enabled": true,
      "lookback_window": 100,
      "z_score_entry": 2.0,
      "z_score_exit": 0.5,
      "max_pairs": 10
    },
    "portfolio_optimization": {
      "enabled": true,
      "rebalance_frequency": 3600,
      "risk_target": 0.15,
      "max_weight": 0.3
    }
  }
}
```

## 🏗️ Project Structure

```
.
├── src/                # Source code (core logic, strategies, networking)
│   ├── main.cpp        # Unified entry point with multi-asset mode support
│   ├── moneybot.cpp    # Core trading engine implementation
│   ├── multi_exchange_gateway.cpp    # Multi-exchange coordination and arbitrage
│   ├── exchange_connectors.cpp       # Exchange-specific API implementations
│   ├── market_maker_strategy.cpp     # Advanced market making algorithm
│   ├── strategy_factory.cpp          # Strategy instantiation factory
│   ├── backtest_engine.cpp           # Historical data simulation
│   ├── network.cpp     # WebSocket connectivity and market data
│   ├── order_manager.cpp             # Order placement and tracking
│   ├── risk_manager.cpp              # Risk controls and position monitoring
│   └── gui_main.cpp    # GUI dashboard implementation
├── include/            # Header files with clean interfaces
│   ├── multi_exchange_gateway.h      # Multi-exchange trading interface
│   ├── exchange_connectors.h         # Exchange connector definitions
│   ├── statistical_arbitrage_strategy.h  # Advanced statistical strategies
│   └── ...             # Other component headers
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
- **MultiExchangeGateway**: Cross-exchange order book aggregation and arbitrage scanning
- **ExchangeConnectors**: Binance, Coinbase, and Kraken API implementations
- **StatisticalArbitrageStrategy**: Pairs trading and cointegration engine
- **MarketMakerStrategy**: Advanced market making with dynamic pricing
- **Network**: Secure WebSocket connections with automatic reconnection
- **OrderBook**: Real-time market depth tracking and updates
- **RiskManager**: Position limits, VaR calculations, and emergency stops
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
# Production multi-asset trading (requires API keys in config.json)
./build/moneybot --multi-asset

# Safe testing without real orders
./build/moneybot --dry-run

# Single-exchange market making (legacy mode)
./build/moneybot

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

### Multi-Asset Trading Mode

MoneyBot in multi-asset mode displays institutional-level status updates:

```
=== MoneyBot HFT Trading System ===
🚀 MULTI-ASSET TRADING MODE
Strategy: Multi-Asset Goldman Sachs Level
Exchanges: binance coinbase kraken 
Strategies: cross_exchange_arbitrage portfolio_optimization statistical_arbitrage 
Press Ctrl+C to stop

=== Status Update ===
Connection: 🟢 Connected
Running: Yes
Uptime: 120 seconds
Total PnL: $145.67
Total Trades: 87
Avg PnL per Trade: $1.67
Best Bid: 43250.50 | Best Ask: 43251.00 | Spread: 0.50
Emergency Stop: No
Drawdown: 0.25%
---
```

### Legacy Single-Exchange Mode

```
=== MoneyBot HFT Trading System ===
Strategy: market_maker
Symbol: BTCUSDT
Exchange: https://api.binance.us
Press Ctrl+C to stop
```


## 🛡️ Institutional Risk Management

### Multi-Level Risk Controls
- **Portfolio-Level Limits**: Maximum portfolio exposure and concentration limits
- **Exchange-Level Limits**: Per-exchange position and order size enforcement
- **Strategy-Level Limits**: Individual strategy risk budgets and drawdown controls
- **Real-Time VaR**: Value-at-Risk calculation across all positions
- **Cross-Exchange Monitoring**: Aggregate exposure tracking and margining
- **Emergency Stop System**: Immediate halt across all exchanges and strategies
- **Graceful Shutdown**: Clean exit on SIGINT/SIGTERM signals with position unwinding

### Advanced Risk Configuration
```json
{
  "risk": {
    "portfolio": {
      "max_total_exposure": 10.0,     // Maximum total position value (BTC)
      "max_asset_weight": 0.3,        // Maximum single asset allocation
      "var_confidence": 0.99,         // VaR confidence level
      "var_horizon_hours": 24,        // VaR time horizon
      "max_daily_loss": -500.0,       // Daily loss limit ($)
      "max_drawdown": -200.0          // Portfolio drawdown limit ($)
    },
    "exchanges": {
      "binance": {
        "max_position_size": 1.0,     // Maximum position per exchange
        "max_order_size": 0.1,        // Maximum single order
        "max_orders_per_minute": 120  // Rate limiting
      }
    },
    "strategies": {
      "arbitrage": {
        "max_exposure": 2.0,          // Strategy-specific limits
        "min_profit_bps": 15.0,       // Minimum profit threshold
        "max_latency_ms": 100         // Latency tolerance
      }
    }
  }
}
```

## 📈 Institutional Performance Analytics

### Real-Time Metrics
- **Multi-Asset PnL**: Real-time profit/loss across all exchanges and strategies  
- **Performance Attribution**: Strategy and exchange-level contribution analysis
- **Risk-Adjusted Returns**: Sharpe ratio, Calmar ratio, and maximum drawdown tracking
- **Trade Analytics**: Fill rates, slippage analysis, and execution quality metrics
- **Market Data Quality**: Latency monitoring and connection health across exchanges
- **Arbitrage Statistics**: Opportunity detection rates and profit capture efficiency

### Advanced Reporting
- **Strategy Performance**: Individual strategy returns and risk metrics
- **Cross-Exchange Analytics**: Spread analysis and arbitrage profit tracking
- **Portfolio Composition**: Real-time asset allocation and rebalancing history
- **Risk Decomposition**: VaR attribution by exchange, strategy, and asset
- **Correlation Analysis**: Real-time correlation matrix updates
- **Trade Cost Analysis**: Commission, slippage, and market impact measurement

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
1. **Multi-Asset Strategies**: Create strategies that operate across multiple exchanges
2. **Statistical Models**: Implement advanced statistical arbitrage and mean reversion
3. **Portfolio Optimization**: Build Modern Portfolio Theory-based allocation strategies
4. **Cross-Exchange Arbitrage**: Develop latency-sensitive arbitrage algorithms
5. **Factory Registration**: Add new strategies to `strategy_factory.cpp`
6. **Configuration**: Define strategy parameters in `config.json`
7. **Testing**: Use dry-run and backtest modes for validation

### Development Workflow
```bash
# Edit multi-asset strategy
vim src/statistical_arbitrage_strategy.cpp

# Build and test
make && ./build/moneybot --dry-run --multi-asset

# Backtest with multi-asset data
./build/moneybot --backtest data/multi_exchange_ticks.json

# Deploy to production
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
- `dashboard_metrics_widget.h/cpp`: Multi-asset trading performance display
- `dashboard_orderbook_widget.h/cpp`: Cross-exchange market depth visualization
- Multi-exchange order book aggregation and real-time arbitrage opportunity display
- Shared configuration and data with console mode for seamless operation

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
  "strategy": {
    "type": "multi_asset",
    "mode": "goldman_sachs_level"
  },
  "multi_asset": {
    "exchanges": [
      {
        "name": "binance",
        "enabled": true,
        "rest_url": "https://api.binance.com",
        "ws_url": "wss://stream.binance.com:9443/ws",
        "api_key": "${BINANCE_API_KEY}",
        "secret_key": "${BINANCE_SECRET_KEY}",
        "taker_fee": 0.001,
        "maker_fee": 0.001
      }
    ]
  },
  "strategies": {
    "cross_exchange_arbitrage": {
      "enabled": true,
      "min_profit_bps": 15.0
    },
    "statistical_arbitrage": {
      "enabled": true,
      "pairs": ["BTC/ETH", "ETH/ADA"]
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
- **Multi-Exchange Monitoring**: Simultaneous health checks across all connected exchanges
- **Strategy Performance Tracking**: Individual and aggregate strategy performance metrics
- **Cross-Exchange Latency**: Real-time latency monitoring for arbitrage opportunities
- **Risk Alert System**: Automatic notifications for limit breaches and emergency stops
- **Performance Attribution**: Detailed breakdown of returns by exchange and strategy
- **Log Analysis**: Structured JSON logs optimized for institutional monitoring systems
- **Health Checks**: WebSocket connectivity and trading status across all venues
- **Prometheus Integration**: Export metrics for institutional monitoring infrastructure
- **Error Handling**: Automatic restart and failover capabilities for high availability

## 📋 System Requirements

### Minimum Requirements
- **CPU**: 2 cores, 2.0GHz
- **RAM**: 512MB
- **Network**: Stable internet connection (<50ms latency to exchange)
- **Storage**: 1GB for logs and market data

### Recommended for Production
- **CPU**: 8+ cores, 3.5GHz+ (for multi-exchange processing)
- **RAM**: 8GB+ (for real-time analytics and strategy computation)
- **Network**: Co-location near major exchange data centers (<10ms latency)
- **Storage**: NVMe SSD with 50GB+ free space (for market data and logs)
- **OS**: Ubuntu 22.04 LTS or Docker-compatible environment
- **Infrastructure**: Kubernetes cluster for high availability and auto-scaling

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

**IMPORTANT: Multi-asset and cross-exchange trading significantly increases complexity and risk. Goldman Sachs-level features require institutional risk management expertise.**

- Always test with dry-run mode first
- Start with small position sizes across all exchanges  
- Monitor risk management settings continuously
- Keep API keys secure for all exchanges
- Regular backups of configuration and logs
- Understand correlation risks in multi-asset portfolios
- Monitor cross-exchange exposure and concentration limits
- Implement proper disaster recovery procedures