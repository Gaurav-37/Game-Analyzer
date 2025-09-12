# Game Analyzer

A lightweight, fast Windows application for monitoring and analyzing memory values in running games and applications.

## Features

- **Real Process Monitoring**: Shows actual running applications (filters out system processes)
- **Smart Search**: Find processes quickly by typing part of their name
- **Memory Reading**: Actually reads memory from selected processes using Windows API
- **Live Monitoring**: Real-time monitoring of memory values
- **Data Export**: Export collected data to CSV files
- **Lightweight**: Only ~115KB executable
- **Professional UI**: Clean Windows GUI interface

## Quick Start

### For Users (Download & Run)
1. Download `GameAnalyzer.exe` from the releases
2. Run the executable
3. Select a process from the list
4. Add memory addresses you want to monitor
5. Start monitoring and export data

### For Developers (Build from Source)

#### Prerequisites
- Windows 10/11
- GCC/G++ compiler (Cygwin, MinGW, or Visual Studio)
- Git (for downloading dependencies)

#### Setup
1. Clone the repository:
   ```bash
   git clone <repository-url>
   cd Game-Analyzer
   ```

2. Setup dependencies:
   ```bash
   setup_dependencies.bat
   ```

3. Build the application:
   ```bash
   build_final.bat
   ```

## Usage

### Selecting a Process
1. Launch the application
2. Use the search bar to find specific applications
3. Click on a process to select it
4. The status bar will show your selection

### Adding Memory Addresses
1. Enter memory address in hex format (e.g., `0x12345678`)
2. Give it a descriptive name (e.g., "Health", "Score")
3. Click "Add Address"

### Monitoring
1. Click "Start Monitoring" to begin reading memory
2. Watch real-time values in the status bar
3. Click "Stop Monitoring" to end

### Exporting Data
1. Click "Export Data" to save to CSV
2. File will be saved as `game_analysis.csv` in the application directory
3. Open in Excel or any spreadsheet application

## Technical Details

- **Language**: C++17
- **GUI Framework**: Win32 API
- **Memory Reading**: Windows API (`ReadProcessMemory`)
- **Process Enumeration**: Windows API (`EnumProcesses`)
- **Build System**: GCC/G++ with batch scripts

## File Structure

```
Game Analyzer/
├── GameAnalyzer.exe          # Main application
├── src/main.cpp              # Source code
├── external/imgui/           # ImGui dependencies
├── build_final.bat           # Build script
├── setup_dependencies.bat    # Dependency setup
├── create_release.bat        # Release builder
├── installer.bat             # Simple installer
├── README.md                 # This file
└── LICENSE                   # MIT License
```

## Building

The project uses a simple batch-based build system:

1. **Setup Dependencies**: Downloads ImGui automatically
2. **Build**: Compiles with GCC/G++ and links Windows libraries
3. **Clean**: Removes unnecessary files for distribution

## Distribution

### For End Users
- **Portable**: Just run `GameAnalyzer.exe`
- **Installer**: Use `installer.bat` for system installation
- **Release Package**: Use `create_release.bat` for distribution

### For Developers
- **Source Code**: Single `main.cpp` file with all functionality
- **Dependencies**: Automatically downloaded by setup script
- **Build Scripts**: Simple batch files for compilation

## Requirements

- **OS**: Windows 10/11
- **Architecture**: x64
- **Permissions**: Run as Administrator for best results
- **Memory**: ~2-3 MB RAM usage

## License

MIT License - see LICENSE file for details.

## Contributing

1. Fork the repository
2. Make your changes
3. Test with `build_final.bat`
4. Submit a pull request

## Troubleshooting

### "g++ not found"
- Install GCC/G++ compiler (Cygwin, MinGW, or Visual Studio)
- Make sure it's in your PATH

### "ImGui not found"
- Run `setup_dependencies.bat` first
- Check internet connection for downloading dependencies

### "Access Denied" when reading memory
- Run as Administrator
- Some processes may be protected by Windows security

### Process not showing in list
- Click "Refresh Processes"
- Check "Show System Processes" if needed
- Use search bar to find specific applications