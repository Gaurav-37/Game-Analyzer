# Game Profiles Database

This directory contains pre-configured memory profiles for popular games, making it easier to quickly start monitoring specific games without manual scanning.

## Supported Games

### FPS Games
- **Counter-Strike 2** - Health, armor, ammo, money
- **Valorant** - Health, shield, abilities, credits
- **Apex Legends** - Health, shield, ammo, inventory
- **Call of Duty: Warzone** - Health, armor, ammo, cash
- **Overwatch 2** - Health, ultimate charge, abilities

### RPG Games
- **The Elder Scrolls V: Skyrim** - Health, magicka, stamina, level
- **The Witcher 3** - Health, toxicity, experience, level
- **Dark Souls III** - Health, stamina, souls, humanity
- **Fallout 4** - Health, radiation, caps, experience
- **Cyberpunk 2077** - Health, cyberware, eddies, street cred

### Strategy Games
- **Age of Empires IV** - Resources (food, wood, stone, gold)
- **Civilization VI** - Science, culture, gold, faith
- **Total War: Warhammer III** - Movement points, gold, influence
- **Starcraft II** - Minerals, gas, supply, energy

### Indie Games
- **Hollow Knight** - Health, soul, geo, masks
- **Celeste** - Strawberries, deaths, time, hearts
- **Dead Cells** - Health, cells, gold, scrolls
- **Hades** - Health, darkness, gems, keys

## Profile Format

Each profile is a text file containing:
```
# Game: [Game Name]
# Version: [Game Version]
# Last Updated: [Date]

[Address Name]=[Memory Address]
[Address Name]=[Memory Address]
...
```

## Usage

1. Select your game process
2. Click "Load Game Profile"
3. The system will automatically detect and load the appropriate profile
4. Start monitoring immediately

## Contributing

To add a new game profile:
1. Create a new `.txt` file in this directory
2. Follow the format above
3. Test with the actual game
4. Submit for inclusion

