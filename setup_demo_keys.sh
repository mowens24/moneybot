#!/bin/bash

# Demo API Keys Setup for Testing
# This script sets up fake API keys for development and testing purposes

echo "Setting up DEMO API keys for testing..."

# Create a demo environment file
cat > .env.demo << 'EOF'
# MoneyBot Demo Environment Configuration
# These are FAKE keys for testing purposes only

# Development Mode
MONEYBOT_PRODUCTION=false
MONEYBOT_DRY_RUN=true

# Demo Binance API Configuration (FAKE KEYS)
BINANCE_API_KEY=demo_binance_api_key_for_testing_12345
BINANCE_SECRET_KEY=demo_binance_secret_key_for_testing_67890

# Demo Coinbase Pro API Configuration (FAKE KEYS)  
COINBASE_API_KEY=demo_coinbase_api_key_for_testing_abcde
COINBASE_SECRET_KEY=demo_coinbase_secret_key_for_testing_fghij
COINBASE_PASSPHRASE=demo_coinbase_passphrase_for_testing

# Demo Kraken API Configuration (FAKE KEYS)
KRAKEN_API_KEY=demo_kraken_api_key_for_testing_klmno
KRAKEN_SECRET_KEY=demo_kraken_secret_key_for_testing_pqrst

# Logging and Monitoring
MONEYBOT_LOG_LEVEL=INFO
MONEYBOT_METRICS_ENABLED=true
EOF

echo "Demo environment file created: .env.demo"

# Copy to main .env for testing
cp .env.demo .env

echo "Demo API keys copied to .env for testing"
echo ""
echo "Now you can test with:"
echo "  source load_env.sh"
echo "  ./build/moneybot --multi-asset --dry-run"
echo ""
echo "NOTE: These are FAKE API keys for testing only!"
echo "For real trading, replace with actual API keys from exchanges."
