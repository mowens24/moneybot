# MoneyBot CLI Trading System

## Project Structure (After Cleanup)

```
moneybot/
├── src/
│   ├── cli_main.cpp                    # ✅ Main CLI application
│   ├── config_manager.cpp              # ✅ Configuration management
│   ├── logger.cpp                      # ✅ Basic logging
│   ├── main.cpp                        # ✅ Alternative main entry
│   ├── order_manager.cpp               # ✅ Order execution
│   ├── risk_manager.cpp                # ✅ Risk management
│   ├── types.cpp                       # ✅ Common types
│   ├── market_data_simulator.cpp       # ✅ Market data simulation
│   ├── multi_exchange_gateway.cpp      # ✅ Exchange connectivity
│   ├── exchange_connectors.cpp         # ✅ Exchange implementations
│   ├── market_maker_strategy.cpp       # ✅ Trading strategy
│   ├── moneybot.cpp                    # ✅ Core trading logic
│   ├── strategy_factory.cpp            # ✅ Strategy creation
│   ├── backtest_engine.cpp             # ✅ Backtesting
│   ├── data_analyzer.cpp               # ✅ Data analysis
│   ├── network.cpp                     # ✅ Network utilities
│   ├── order_book.cpp                  # ✅ Order book management
│   └── core/                           # ✅ Core components
│       ├── exchange_manager.cpp
│       ├── portfolio_manager.cpp
│       └── strategy_controller.cpp
├── include/
│   ├── simple_logger.h                 # ✅ Simple logging interface
│   ├── config_manager.h                # ✅ Configuration interface
│   ├── logger.h                        # ✅ Logger interface
│   ├── risk_manager.h                  # ✅ Risk management
│   ├── types.h                         # ✅ Common types
│   ├── order_manager.h                 # ✅ Order management
│   ├── market_data_simulator.h         # ✅ Market simulation
│   ├── multi_exchange_gateway.h        # ✅ Exchange gateway
│   ├── exchange_connectors.h           # ✅ Exchange connections
│   ├── market_maker_strategy.h         # ✅ Trading strategy
│   ├── moneybot.h                      # ✅ Main header
│   ├── strategy_factory.h              # ✅ Strategy factory
│   ├── backtest_engine.h               # ✅ Backtesting
│   ├── data_analyzer.h                 # ✅ Data analysis
│   ├── network.h                       # ✅ Network utilities
│   ├── order_book.h                    # ✅ Order book
│   ├── strategy.h                      # ✅ Strategy base
│   ├── dummy_strategy.h                # ✅ Example strategy
│   ├── statistical_arbitrage_strategy.h # ✅ Arbitrage strategy
│   ├── ring_buffer.h                   # ✅ Data structures
│   └── core/                           # ✅ Core headers
│       ├── exchange_manager.h
│       ├── portfolio_manager.h
│       └── strategy_controller.h
├── lib/
│   ├── json/                           # ✅ JSON library
│   └── spdlog/                         # ✅ Logging library
├── config.json                         # ✅ Main configuration
├── build/                              # ✅ Build directory
├── logs/                               # ✅ Log files
└── data/                               # ✅ Market data
```

## Removed Components

### ❌ GUI Components (Problematic)
- `src/gui/` - Complex ImGui interface
- `src/gui_main.cpp` - 2000+ line GUI main
- `src/gui_main_simple.cpp` - Simple GUI attempt  
- `src/imgui_buffer_sink.cpp` - ImGui logging
- `include/imgui_buffer_sink.h` - ImGui headers
- `include/*widget.h` - GUI widget headers

### ❌ Library Dependencies (Unnecessary)
- `lib/imgui/` - ImGui library
- `lib/imgui-terminal/` - Terminal widgets
- `lib/ImGuiColorTextEdit/` - Text editing
- `lib/imterm/` - Terminal emulation
- `lib/libvterm/` - Virtual terminal

### ❌ Build Complexity
- Complex CMakeLists.txt with GUI dependencies
- Docker configuration files
- Broken test configurations

## Current Status

### ✅ Working Components
- CLI interface with professional commands
- Basic logging and configuration
- Clean build system
- Modular architecture ready for expansion

### 🔄 Next Steps
1. **Integrate real trading components** into CLI
2. **Add exchange API connections**
3. **Implement strategy system**
4. **Add risk management**
5. **Create data persistence**

## Build & Run

```bash
# Build
cd build
cmake ..
make -j4

# Run
./moneybot_cli version
./moneybot_cli status
./moneybot_cli --help
```

## CLI Commands

- `start` - Start trading system
- `stop` - Stop trading system  
- `status` - Show system status
- `portfolio` - View portfolio
- `strategies` - Manage strategies
- `risk-check` - Risk metrics
- `config` - Configuration
- `version` - Version info
