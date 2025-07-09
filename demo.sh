#!/bin/bash

# MoneyBot Goldman Sachs-Level Demo Script
# This script demonstrates the full capabilities of the enhanced MoneyBot system

echo "=================================================="
echo "  🚀 MoneyBot Goldman Sachs-Level Trading System"
echo "=================================================="
echo ""
echo "This demo showcases:"
echo "✅ Multi-asset, multi-exchange trading architecture"
echo "✅ Real-time market data simulation"
echo "✅ Advanced trading strategies (arbitrage, portfolio optimization)"
echo "✅ Live market data feeds with realistic pricing"
echo "✅ Risk management and performance monitoring"
echo "✅ Production-ready configuration management"
echo "✅ Secure API key handling via environment variables"
echo ""

# Ensure we're in the right directory
cd "$(dirname "$0")"

echo "Setting up demo environment..."

# Load demo API keys
echo "📁 Loading demo API keys..."
./setup_demo_keys.sh > /dev/null 2>&1

# Load environment
echo "🔧 Loading environment variables..."
source load_env.sh

echo ""
echo "🎯 DEMO FEATURES:"
echo ""

echo "1. Multi-Asset Trading with Live Data Simulation"
echo "   - 5 major crypto pairs (BTC, ETH, ADA, DOT, LINK)"
echo "   - 3 exchanges (Binance, Coinbase, Kraken)"
echo "   - Realistic bid/ask spreads (1-10 basis points)"
echo "   - Dynamic price movements with volatility"
echo ""

echo "2. Advanced Trading Strategies"
echo "   - Statistical Arbitrage"
echo "   - Cross-Exchange Arbitrage"
echo "   - Portfolio Optimization"
echo "   - Market Making with inventory management"
echo ""

echo "3. Risk Management & Monitoring"
echo "   - Real-time P&L tracking"
echo "   - Drawdown monitoring"
echo "   - Emergency stop mechanisms"
echo "   - Performance analytics"
echo ""

echo "4. Market Event Simulation"
echo "   - Simulated news events affecting prices"
echo "   - Market volatility adjustments"
echo "   - Trending market conditions"
echo ""

echo "=================================================="
echo "  🎬 LIVE DEMO"
echo "=================================================="
echo ""
echo "Starting MoneyBot with live market data simulation..."
echo "Watch for:"
echo "  📊 Real-time price ticks with bid/ask spreads"
echo "  📈 Market updates showing price movements"
echo "  📰 Simulated news events (after 30 seconds)"
echo "  📋 Status updates every 10 seconds"
echo ""
echo "Press Ctrl+C to stop the demo"
echo ""
read -p "Press Enter to start the live demo..."

# Run the system
./build/moneybot --multi-asset --dry-run
