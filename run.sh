#!/bin/bash
# MoneyBot - Complete Trading System Demo

echo "🚀 MoneyBot - Goldman Sachs-Level Multi-Asset Trading System"
echo "============================================================"
echo ""
echo "Available modes:"
echo "1. GUI Dashboard (Default)"
echo "2. Console Trading Mode"
echo "3. Backtest Mode" 
echo "4. Clean Build"
echo ""

# Default to GUI if no argument provided
MODE=${1:-gui}

case $MODE in
    "gui"|"dashboard"|"1")
        echo "🎮 Starting Goldman Sachs-Level GUI Dashboard..."
        echo "📊 Multi-Asset Trading Interface with Real-Time Data"
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
