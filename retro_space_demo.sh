#!/bin/bash

echo "🚀 Building MoneyBot with Retro Space Aesthetic..."
echo "=================================================="
echo ""
echo "🎯 RETRO CRT FEATURES:"
echo "  • Monochromatic green terminal theme"
echo "  • ASCII art borders and separators"
echo "  • CRT scanlines and flicker effects"
echo "  • Stardate timestamps"
echo "  • Terminal-style progress bars"
echo "  • Alien spaceship computer aesthetic"
echo ""

# Build the project
echo "📦 Building MoneyBot..."
if cmake --build build; then
    echo ""
    echo "✅ Build successful!"
    echo ""
    echo "🎮 LAUNCHING RETRO SPACE DASHBOARD..."
    echo "═══════════════════════════════════════════════"
    echo "    MONEYBOT TRADING SYSTEM - SECTOR 7-G     "
    echo "═══════════════════════════════════════════════"
    echo ""
    
    # Launch the retro GUI
    ./build/moneybot --gui --demo
    
    echo ""
    echo "✨ Retro space experience complete!"
    echo "   • Authentic 1970s CRT terminal aesthetic"
    echo "   • Green-on-black Alien computer styling" 
    echo "   • Professional trading functionality"
else
    echo ""
    echo "❌ Build failed. Please check the errors above."
    echo "   The retro space theme implementation needs debugging."
fi
