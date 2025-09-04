# smon

System monitor for macOS Apple Silicon (ARM64). CPU, memory, network, and process monitoring with ncurses.

```
 System Monitor
Press 'q' to quit | Arrow keys to scroll processes

CPU Usage
Total: 23.4%
Core 1: 18.2%    Core 2: 28.1%
Core 3: 21.7%    Core 4: 25.3%
Load: 1.45 1.32 1.28

Memory
Used: 12.3 GB / 16.0 GB (76.9%)
Free: 3.7 GB
Swap: 1.2 GB / 4.0 GB (30.0%)
Uptime: 2d 14h 32m

Network
RX: 45632 bytes/s
TX: 32189 bytes/s

Disk I/O
Reads: 234 ops/s
Writes: 156 ops/s

Top Processes (by CPU)
PID      Name                 CPU%     Memory
8472     Xcode                12.3     892340 KB
1247     Terminal             8.9      234560 KB
3456     Chrome               6.7      456780 KB
7890     Slack                4.2      123450 KB
```

## Features

- CPU usage (total + per-core)
- Memory and swap usage
- Network throughput
- Top processes by CPU
- Load average and uptime
- Keyboard navigation (q to quit, arrows to scroll)

## Requirements

- macOS 11.0+ on Apple Silicon
- ncurses

## Install

```bash
brew install ncurses
git clone https://github.com/barturba/smon.git
cd smon
make
```

## Run

```bash
./build/smon
```

## Download

Pre-built binary available at [Releases](https://github.com/barturba/smon/releases).

```bash
curl -L https://github.com/barturba/smon/releases/latest/download/smon-macos-arm64.zip -o smon.zip
unzip smon.zip && chmod +x smon
./smon
```

## License

MIT
