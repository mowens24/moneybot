#!/bin/bash
# Load environment variables from .env file

if [ -f .env ]; then
    echo "Loading environment variables from .env..."
    export $(cat .env | grep -v '^#' | grep -v '^$' | xargs)
    echo "Environment variables loaded successfully"
    
    # Validate critical variables are set
    if [ -z "$BINANCE_API_KEY" ] || [ "$BINANCE_API_KEY" = "your_binance_api_key_here" ]; then
        echo "WARNING: Binance API key not configured"
    fi
    
    if [ -z "$COINBASE_API_KEY" ] || [ "$COINBASE_API_KEY" = "your_coinbase_api_key_here" ]; then
        echo "WARNING: Coinbase API key not configured" 
    fi
    
    if [ -z "$KRAKEN_API_KEY" ] || [ "$KRAKEN_API_KEY" = "your_kraken_api_key_here" ]; then
        echo "WARNING: Kraken API key not configured"
    fi
    
    echo "Production mode: $MONEYBOT_PRODUCTION"
    echo "Dry run mode: $MONEYBOT_DRY_RUN"
else
    echo "ERROR: .env file not found. Run setup_env.sh first."
    exit 1
fi
