# smon - Minimalist System Monitor for macOS

A beautiful, real-time system activity monitor for macOS Apple Silicon (ARM64), written in C with ncurses. Inspired by minimalist design principles - clean, efficient, and ruthlessly streamlined.

![smon screenshot](https://via.placeholder.com/800x600/000000/00FFFF?text=smon+terminal+screenshot)

## Features

- **Real-time monitoring**: Updates every second with live system statistics
- **CPU usage**: Total and per-core CPU utilization
- **Memory usage**: RAM and swap usage with percentages
- **Network throughput**: Real-time RX/TX bytes per second
- **Disk I/O**: Read/write operations per second
- **Top processes**: Processes sorted by CPU usage with memory consumption
- **System info**: Load average and uptime
- **Minimalist UI**: Clean design with subtle color scheme (blues/greens on dark background)
- **Responsive layout**: Adapts to terminal window size
- **Keyboard navigation**: Arrow keys to scroll processes, 'q' to quit

## Requirements

- **macOS 11.0+** on Apple Silicon (ARM64)
- **ncurses** (install via Homebrew: `brew install ncurses`)
- **CMake 3.16+** for building

## Installation

### Option 1: Using Makefile (Recommended)

```bash
# Clone the repository
git clone https://github.com/yourusername/smon.git
cd smon

# Build for ARM64
make

# Install (optional)
sudo make install
```

### Option 2: Using CMake directly

```bash
# Clone the repository
git clone https://github.com/yourusername/smon.git
cd smon

# Create build directory
mkdir build && cd build

# Configure for ARM64
cmake -DCMAKE_OSX_ARCHITECTURES="arm64" -DCMAKE_BUILD_TYPE=Release ..

# Build
make -j$(sysctl -n hw.ncpu)

# Install (optional)
sudo make install
```

## Usage

```bash
# Run smon
./build/smon

# Or if installed
smon
```

### Controls

- **`q`** or **`Q`**: Quit the application
- **`↑`** / **`↓`**: Scroll through process list
- **`Page Up`** / **`Page Down`**: Scroll faster through process list

## Building for Development

### Prerequisites

```bash
# Install dependencies via Homebrew
brew install ncurses cmake
```

### Debug Build

```bash
mkdir build-debug && cd build-debug
cmake -DCMAKE_OSX_ARCHITECTURES="arm64" -DCMAKE_BUILD_TYPE=Debug ..
make -j$(sysctl -n hw.ncpu)
```

### Release Build

```bash
mkdir build-release && cd build-release
cmake -DCMAKE_OSX_ARCHITECTURES="arm64" -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.ncpu)
```

## Architecture

smon uses macOS-specific APIs for optimal performance:

- **CPU**: `host_statistics()` with `HOST_CPU_LOAD_INFO`
- **Memory**: `host_statistics64()` with `HOST_VM_INFO64`
- **Processes**: `sysctl()` with `KERN_PROC_ALL` and `proc_pidinfo()`
- **Network**: `sysctl()` with network interface statistics
- **System info**: `sysctl()` for load average and uptime

## Project Structure

```
smon/
├── main.c              # Main application code
├── CMakeLists.txt      # CMake build configuration
├── Makefile           # Simple build wrapper
├── README.md          # This file
├── LICENSE            # MIT License
├── .gitignore         # Git ignore rules
└── .github/
    └── workflows/
        └── ci.yml     # GitHub Actions CI/CD
```

## CI/CD Pipeline

This project includes GitHub Actions for automated building and testing:

- **Builds on**: `macos-latest` (ARM64)
- **Tests**: Basic smoke tests and binary validation
- **Artifacts**: Uploads build artifacts on every push
- **Releases**: Automatic ZIP creation for tagged releases

### Monitoring CI Builds

```bash
# View recent workflow runs
gh run list

# Watch a specific run
gh run watch <run-id>

# View run details
gh run view <run-id>

# View logs
gh run view <run-id> --log
```

## Creating and Managing Repository with GitHub CLI

### Initial Setup

```bash
# Create new repository
gh repo create smon --public --description "Minimalist system monitor for macOS"

# Add remote origin
git remote add origin https://github.com/yourusername/smon.git

# Initial push
git push -u origin main
```

### Pushing Changes and Monitoring

```bash
# Push changes
git add .
git commit -m "Your commit message"
git push

# Check build status
gh run list --limit 5

# Watch latest build
gh run watch $(gh run list --limit 1 --json databaseId -q '.[0].databaseId')

# View build logs if failed
gh run view --log
```

### Creating Releases

```bash
# Create a tag
git tag v1.0.0
git push origin v1.0.0

# Or create release with gh
gh release create v1.0.0 --title "Release v1.0.0" --notes "First stable release"

# Monitor release build
gh run watch $(gh run list --limit 1 --json databaseId -q '.[0].databaseId')
```

### Troubleshooting Builds

```bash
# Check workflow status
gh workflow list

# View workflow file
gh workflow view ci.yml

# Rerun failed jobs
gh run rerun <run-id>

# View job logs
gh run view <run-id> --log
```

## Screenshots

### Main Interface
```
 System Monitor
Press 'q' to quit | Arrow keys to scroll processes

CPU Usage
Total: 15.2%
Core 1: 12.5%    Core 2: 18.1%
Core 3: 14.8%    Core 4: 16.3%
Load: 1.25 1.18 1.12

Memory
Used: 8192 MB / 16384 MB (50.0%)
Free: 8192 MB
Swap: 1024 MB / 2048 MB (50.0%)
Uptime: 3d 14h 22m

Network
RX: 125430 bytes/s
TX: 89234 bytes/s

Disk I/O
Reads: 45 ops/s
Writes: 23 ops/s

Top Processes (by CPU)
PID      Name                 CPU%     Memory
1234     Terminal             8.5      245760 KB
5678     Safari               6.2      456320 KB
9012     Xcode                4.1      678400 KB
3456     Slack                2.8      123456 KB
```

## Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature-name`
3. Make your changes
4. Test on macOS ARM64
5. Commit: `git commit -am 'Add feature'`
6. Push: `git push origin feature-name`
7. Submit a pull request

## License

MIT License - see [LICENSE](LICENSE) file for details.

## Performance

smon is designed for minimal resource usage:
- **Memory footprint**: ~2-3 MB
- **CPU usage**: <1% when monitoring
- **Update frequency**: 1 Hz with responsive UI

## Known Limitations

- Network and disk I/O statistics are simplified (uses basic sysctl counters)
- Process memory reporting may vary by macOS version
- Designed exclusively for macOS Apple Silicon (ARM64)
