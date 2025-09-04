# smon

System monitor for macOS Apple Silicon (ARM64). CPU, memory, network, and process monitoring with ncurses.

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
