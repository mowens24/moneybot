#!/bin/bash

# MoneyBot Run Script
set -e

echo "🚀 Starting MoneyBot Trading Dashboard..."

# Check if binary exists
if [ ! -f "build/moneybot" ]; then
    echo "❌ MoneyBot binary not found. Please run ./build.sh first."
    exit 1
fi

# Run MoneyBot
cd build
./moneybot
        echo "💼 Portfolio Management | ⚡ Arbitrage Scanner | 🛡️ Risk Controls"
        echo ""
        ./build/moneybot --gui
        ;;
    "console"|"cli"|"2")
        echo "💻 Starting Console Trading Mode..."
        ./build/moneybot --console
        ;;
    "backtest"|"test"|"3")
        echo "📈 Starting Backtest Analysis..."
        ./build/moneybot --backtest
        ;;
    "build"|"4")
        echo "🔨 Clean Build..."
        make clean
        make -j$(nproc)
        echo "✅ Build complete!"
        ;;
    "help"|"-h"|"--help")
        echo "Usage: ./run.sh [mode]"
        echo "Modes: gui, console, backtest, build"
        ;;
    *)
        echo "❌ Unknown mode: $MODE"
        echo "Use './run.sh help' for usage information"
        exit 1
        ;;
esac
