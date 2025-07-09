#!/bin/bash

# MoneyBot API Keys Setup Script
# This script helps you set up environment variables for exchange API keys

echo "=================================================="
echo "  MoneyBot Exchange API Keys Setup"
echo "=================================================="
echo ""

# Create environment file
ENV_FILE=".env"
echo "Creating environment file: $ENV_FILE"

cat > $ENV_FILE << 'EOF'
# MoneyBot Environment Configuration
# 
# SECURITY WARNING: 
# - Keep this file secure and never commit it to version control
# - Use strong API keys with minimal required permissions
# - Enable IP whitelisting on exchange accounts when possible

# Production/Development Mode
MONEYBOT_PRODUCTION=false
MONEYBOT_DRY_RUN=true

# Binance API Configuration
# Get these from: https://www.binance.us/en/usercenter/settings/api-management
BINANCE_API_KEY=your_binance_api_key_here
BINANCE_SECRET_KEY=your_binance_secret_key_here

# Coinbase Pro API Configuration  
# Get these from: https://pro.coinbase.com/profile/api
COINBASE_API_KEY=your_coinbase_api_key_here
COINBASE_SECRET_KEY=your_coinbase_secret_key_here
COINBASE_PASSPHRASE=your_coinbase_passphrase_here

# Kraken API Configuration
# Get these from: https://www.kraken.com/u/security/api
KRAKEN_API_KEY=your_kraken_api_key_here
KRAKEN_SECRET_KEY=your_kraken_secret_key_here

# Optional: Logging and Monitoring
MONEYBOT_LOG_LEVEL=INFO
MONEYBOT_METRICS_ENABLED=true
EOF

echo "Environment file created: $ENV_FILE"
echo ""

# Create a gitignore entry if it doesn't exist
if [ ! -f .gitignore ]; then
    echo "Creating .gitignore file..."
    cat > .gitignore << 'EOF'
# Build artifacts
build/
bin/
*.o
*.a
*.so

# Environment files (NEVER commit these)
.env
.env.local
.env.production

# Logs
logs/
*.log

# Data files
data/
*.db

# IDE files
.vscode/
.idea/
*.swp
*.swo

# OS files
.DS_Store
Thumbs.db
EOF
else
    # Add .env to gitignore if not already there
    if ! grep -q "\.env" .gitignore; then
        echo "" >> .gitignore
        echo "# Environment files (NEVER commit these)" >> .gitignore
        echo ".env" >> .gitignore
        echo ".env.local" >> .gitignore
        echo ".env.production" >> .gitignore
    fi
fi

echo "Updated .gitignore to exclude environment files"
echo ""

# Create load script
cat > load_env.sh << 'EOF'
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
EOF

chmod +x load_env.sh

echo "Created load_env.sh script to load environment variables"
echo ""

echo "=================================================="
echo "  NEXT STEPS:"
echo "=================================================="
echo ""
echo "1. Edit the .env file with your actual API keys:"
echo "   nano .env"
echo ""
echo "2. Configure your exchange accounts:"
echo "   - Enable API access with trading permissions"
echo "   - Set up IP whitelisting for security"
echo "   - Use minimal required permissions"
echo ""
echo "3. Load environment variables before running MoneyBot:"
echo "   source load_env.sh"
echo "   ./build/moneybot --multi-asset"
echo ""
echo "4. For production use:"
echo "   - Set MONEYBOT_PRODUCTION=true"
echo "   - Set MONEYBOT_DRY_RUN=false"
echo "   - Test thoroughly in dry-run mode first"
echo ""
echo "SECURITY REMINDERS:"
echo "- Never commit .env files to version control"
echo "- Use strong, unique API keys"
echo "- Enable 2FA on all exchange accounts"
echo "- Monitor API usage and set rate limits"
echo "- Regularly rotate API keys"
echo ""
echo "Setup complete! Edit .env with your API keys and run 'source load_env.sh' to start."
