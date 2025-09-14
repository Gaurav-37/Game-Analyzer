# Game Analyzer

A professional-grade Windows application for memory analysis and monitoring in games and applications. Features advanced memory scanning, value change detection, and automated workflow tools.

## Features

### 🔍 **Advanced Memory Scanning**
- **Process Memory Discovery**: Automatically scan and discover readable memory regions
- **Game Data Detection**: Smart filtering to find likely game variables (health, score, etc.)
- **Memory Region Analysis**: Detailed information about memory protection and state
- **Value Interpretation**: Automatic detection of different data types (integers, floats, ASCII)

### 📊 **Value Change Detection**
- **Baseline Comparison**: Capture current memory state for comparison
- **Change Tracking**: Automatically detect which memory values have changed
- **Smart Filtering**: Focus on meaningful changes in game data
- **Auto-Monitoring**: Changed addresses automatically added to monitoring list

### 🎮 **Game Profile System**
- **Save Profiles**: Save discovered addresses for specific games
- **Load Profiles**: Quickly reload saved configurations
- **Profile Management**: Automatic naming based on process name
- **Cross-Session**: Preserve your work between application restarts

### 🔧 **Professional UI/UX**
- **Clean Interface**: Streamlined workflow without redundant controls
- **Checkbox Selection**: Easy multi-select for discovered addresses
- **Real-time Search**: Filter processes and addresses as you type
- **Progress Indicators**: Visual feedback for scanning operations
- **Large Display Areas**: Optimized for viewing many addresses

### 📈 **Data Management**
- **Live Monitoring**: Real-time monitoring of selected memory addresses
- **CSV Export**: Export collected data for analysis in Excel/Google Sheets
- **Memory Reading**: Direct Windows API integration for reliable data access
- **Process Filtering**: Smart filtering to show only user applications

### 👁️ **Vision Analysis & Screen Capture**
- **Screen Capture**: DirectX 11 integration for high-quality screen capture
- **OCR Text Detection**: Basic optical character recognition for game UI text
- **Text Region Analysis**: Automatic detection of text areas and labels
- **Vision Accuracy Metrics**: Confidence scoring for detected text regions

### 🔗 **Hybrid Analysis Engine**
- **Memory + Vision Correlation**: Combines memory data with visual UI elements
- **Cross-Validation**: Validates memory values against visible game text
- **Intelligent Matching**: Automatically correlates memory addresses with UI elements
- **Comprehensive Insights**: Generates detailed analysis combining both data sources

### 📊 **Advanced Analytics Engine**
- **Performance Metrics**: Real-time tracking of memory stability and vision accuracy
- **Trend Analysis**: Statistical analysis of value changes over time
- **Data Correlation**: Advanced correlation analysis between different metrics
- **Export Analytics**: CSV export of comprehensive analytics data

### 🎛️ **Professional Dashboard**
- **Bloomberg-Style Interface**: Professional terminal-style analytics dashboard
- **Real-time Charts**: ASCII-based visualizations of performance trends
- **Performance Monitor**: Comprehensive system status and health monitoring
- **Executive Summary**: High-level insights and recommendations

### 🛡️ **Reliability & Performance**
- **Single Instance**: Prevents multiple application windows
- **Error Handling**: Robust error handling for protected processes
- **Lightweight**: Optimized executable size
- **Fast Scanning**: Efficient memory region enumeration

## Quick Start

### For Users (Download & Run)
1. Download `GameAnalyzer.exe` from the releases
2. Run the executable
3. Select a process from the list
4. Click "Scan Process Memory" to discover addresses
5. Use "Compare Values" to capture baseline state
6. Play the game and click "Show Changed Values"
7. Changed addresses are automatically added to monitoring
8. Start monitoring and export data

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

### Basic Workflow

#### 1. **Select a Process**
- Launch the application
- Use the search bar to find specific applications
- Click on a process to select it or double-click for quick selection
- The status bar will show your selection

#### 2. **Discover Memory Addresses**
- Click **"Scan Process Memory"** for general memory scanning
- Click **"Scan Game Data"** for targeted game variable detection
- Wait for the scan to complete (progress bar shows status)
- Review discovered addresses in the list

#### 3. **Find Changing Values**
- Click **"Compare Values"** to capture current memory state
- Play your game or interact with the application
- Click **"Show Changed Values"** to see what changed
- Changed addresses are automatically added to monitoring

#### 4. **Select Addresses to Monitor**
- Use checkboxes to select addresses from the discovered list
- Click **"Add Selected Addresses"** to add them to monitoring
- Use the search box to filter addresses by value or type

#### 5. **Monitor and Export**
- Click **"Start Monitoring"** to begin real-time tracking
- Watch values update in the monitored addresses list
- Click **"Export Data"** to save results to CSV

#### 6. **Vision Analysis (Phase 2)**
- Click **"Capture Screen"** to capture current game screen
- Click **"Start Vision Analysis"** to detect text regions
- Review detected text in the vision analysis results

#### 7. **Hybrid Analysis (Phase 2)**
- Click **"Run Hybrid Analysis"** to combine memory and vision data
- Review comprehensive insights combining both data sources
- Click **"Compare Memory vs Vision"** for detailed comparison

#### 8. **Analytics & Dashboard (Phase 3)**
- Click **"Start Analytics"** to collect performance metrics
- Click **"Show Metrics"** to view detailed analytics
- Click **"Open Dashboard"** for professional Bloomberg-style interface
- Click **"Real-time Charts"** for visual performance trends
- Click **"Performance Monitor"** for system health overview
- Click **"Export Analytics"** to save comprehensive analytics data

### Advanced Features

#### **Game Profiles**
- Click **"Save Game Profile"** to save current addresses for the selected game
- Click **"Load Game Profile"** to reload saved addresses
- Profiles are automatically named based on the process name

#### **Search and Filter**
- **Process Search**: Type in the process search box to filter running applications
- **Address Search**: Type in the address search box to filter discovered addresses
- **System Processes**: Toggle "Show System Processes" to include/exclude system processes

#### **Memory Analysis**
- **Value Interpretation**: Addresses show interpreted values (e.g., "Possible: Health/Score/Count")
- **Memory Regions**: Information about memory protection and state
- **Data Types**: Automatic detection of integers, floats, and ASCII strings

## Technical Details

- **Language**: C++17 with Windows API integration
- **GUI Framework**: Native Win32 API with custom controls
- **Memory Reading**: Windows API (`ReadProcessMemory`, `VirtualQueryEx`)
- **Process Enumeration**: Windows API (`EnumProcesses`, `GetModuleBaseName`)
- **Screen Capture**: DirectX 11 with DXGI Graphics Capture API
- **Vision Processing**: Custom OCR engine with text region detection
- **Analytics Engine**: Statistical analysis with trend calculation
- **Memory Scanning**: Advanced region enumeration and value sampling
- **Threading**: Multi-threaded scanning, monitoring, and analysis operations
- **Build System**: GCC/G++ with static linking for portability

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

### **Build Issues**

#### "g++ not found"
- Install GCC/G++ compiler (Cygwin, MinGW, or Visual Studio)
- Make sure it's in your PATH

#### "ImGui not found"
- Run `setup_dependencies.bat` first
- Check internet connection for downloading dependencies

### **Runtime Issues**

#### "Access Denied" when reading memory
- Run as Administrator for best results
- Some processes may be protected by Windows security
- Try selecting a different process

#### Process not showing in list
- Click "Refresh Processes" to update the list
- Toggle "Show System Processes" if needed
- Use the search bar to find specific applications

#### Memory scanning shows no results
- Ensure the selected process is actually running
- Try "Scan Game Data" for more targeted results
- Some processes may have limited readable memory regions

#### "Please select a process first" error
- Make sure you've clicked on a process in the list
- Try double-clicking the process for quick selection
- Check that the process is still running

#### Changed values not detected
- Make sure you clicked "Compare Values" first
- Ensure you're actually changing values in the game/application
- Try scanning for different types of game data

### **Performance Tips**
- Use "Scan Game Data" instead of "Scan Process Memory" for faster results
- Save game profiles to avoid re-scanning
- Filter addresses using the search box for better performance

## Recent Updates

### Version 3.0 - Professional Analytics Platform (Complete Phase 3)
- ✅ **Professional Dashboard**: Bloomberg Terminal-style analytics interface
- ✅ **Real-time Charts**: ASCII-based visualizations of performance trends
- ✅ **Performance Monitor**: Comprehensive system health and status monitoring
- ✅ **Advanced Analytics Engine**: Statistical analysis with trend calculation
- ✅ **Vision Analysis**: DirectX 11 screen capture with OCR text detection
- ✅ **Hybrid Analysis**: Memory + Vision correlation for comprehensive insights
- ✅ **Analytics Export**: CSV export of detailed performance metrics

### Version 2.0 - Major UI/UX Improvements
- ✅ **Streamlined Workflow**: Removed redundant manual address input
- ✅ **Automated Monitoring**: Changed addresses automatically added to monitoring
- ✅ **Enhanced Search**: Fixed search functionality with proper filtering
- ✅ **Professional UI**: Cleaner interface with better spacing and organization
- ✅ **Game Profiles**: Save/load functionality for different games
- ✅ **Single Instance**: Prevented multiple application windows
- ✅ **Value Interpretation**: Smart detection of data types and meanings
- ✅ **Memory Analysis**: Advanced region scanning and game data detection

### Key Improvements
- **Complete Hybrid Platform**: Memory analysis + Vision analysis + Analytics dashboard
- **Professional Grade**: Bloomberg Terminal-style interface with real-time monitoring
- **Comprehensive Analysis**: Combines multiple data sources for deep insights
- **Simplified Workflow**: Scan → Compare → Auto-add changed addresses
- **Better UX**: Checkbox selection, real-time search, progress indicators
- **Professional Features**: Game profiles, advanced memory scanning, analytics
- **Reliability**: Single instance prevention, robust error handling