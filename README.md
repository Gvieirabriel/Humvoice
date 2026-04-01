# Humvoice

> **Voice-to-MIDI VST3 plugin** — hum, sing, or whistle and convert your voice into MIDI notes in real time.

Humvoice captures audio from any input, detects pitch or transient onsets, and generates MIDI output your DAW can route to any instrument. It runs entirely inside the plugin without external services or subscriptions.

---

## Features

### Pitch Mode
- **Real-time pitch detection** via autocorrelation optimized for the human voice (80–1000 Hz default range)
- **MIDI note output** with velocity derived from input amplitude
- **Pitch bend output** — continuous 14-bit pitch bend tracks micro-variations between notes for expressive, smooth MIDI
- **Scale quantization** — snap detected notes to Major or Minor scales with a configurable root note
- **Vocal range calibration** — sing your range for 5 seconds; Humvoice tunes the pitch detector to your voice

### Drum Mode
- **Onset detection** — detects transients (hits, plosives, percussion sounds) from any audio input
- **Single MIDI note trigger** — maps each onset to a configurable MIDI note (default: GM snare, note 38)
- **Velocity sensitivity** — hit strength is mapped logarithmically to MIDI velocity 1–127

### Signal Processing
- **Noise gate** — configurable dBFS floor, blocks detection below threshold
- **Median filter** — 5-frame pitch history rejects single-frame outliers without blurring note transitions
- **Octave error correction** — 2-pass half-lag check and temporal continuity prevent octave jumps
- **Minimum note duration** — configurable hold time prevents flicker from fast transient detections
- **Hysteresis** — 60-cent deadband around the active note absorbs natural vibrato without triggering note changes

### Controls (all DAW-automatable)
| Parameter | Range | Default | Description |
|---|---|---|---|
| Mode | Pitch / Drum | Pitch | Selects pitch-tracking or drum-trigger engine |
| Sensitivity | 0 – 1 | 0.5 | Pitch detection threshold / onset sensitivity |
| Noise Gate | −80 – 0 dBFS | −40 dB | Minimum signal level |
| Smoothing | 0 – 200 ms | 20 ms | EMA time constant for display pitch and pitch bend |
| Min Note | 0 – 500 ms | 100 ms | Minimum time before a note change is committed |
| Scale | Off / Major / Minor | Off | Scale quantization |
| Root Note | C – B | C | Root note for scale quantization |
| Drum Note | 0 – 127 | 38 | MIDI note fired on each detected onset |

---

## How to Use

### Basic Setup
1. Insert **Humvoice** on an audio track carrying your microphone or instrument input.
2. Route its **MIDI output** to an instrument track in your DAW.
3. Arm the instrument track and play or record.

### Pitch Mode (default)
1. Set **Mode → Pitch**.
2. Adjust **Noise Gate** until the display reads `--` in silence.
3. Hum or sing — the pitch panel shows the detected note, cents offset, and a stability bar.
4. Use **Scale** and **Root Note** to lock output to a key.
5. Increase **Smoothing** for legato feel; lower it for faster response.

### Vocal Range Calibration
1. Click **Calibrate** and sing through your full range (low to high) within 5 seconds.
2. Humvoice stores your range with ±25 % headroom and constrains pitch detection to it.
3. The pitch panel briefly displays your captured range (e.g., `C2 – G4`).

### Drum Mode
1. Set **Mode → Drum**.
2. Use your voice as a percussion instrument — clicks, pops, and beatbox hits trigger MIDI.
3. Set **Drum Note** to the target GM note (36 = kick, 38 = snare, 42 = hi-hat, etc.).
4. Adjust **Sensitivity** and **Noise Gate** to dial in how hard you need to hit.

### Pitch Bend
Pitch bend is always active in Pitch Mode. Humvoice sends 14-bit pitch bend messages continuously as your voice moves between notes, giving connected instruments expressive, human-feeling transitions. Pitch bend resets to centre on every note change. The receiving instrument must have its pitch bend range set to **±2 semitones** (MIDI standard) to match.

---

## Architecture

```
src/
├── AudioEngine      Stereo-to-mono mixing, circular buffer, block RMS
├── PitchEngine      Autocorrelation, octave correction, median filter, EMA smoothing
├── MidiEngine       Hysteresis, candidate logic, scale quantization, velocity, pitch bend
├── DrumEngine       Onset detection, retrigger protection, drum MIDI output
├── PluginProcessor  Orchestrates engines, APVTS parameters, calibration state
└── PluginEditor     JUCE UI — knobs, combos, mode switch, pitch display, calibrate button
```

Each engine has a single `prepare()` and `process()` method and no cross-dependencies. `PluginProcessor` owns all state and wires the data flow.

---

## Building

### Prerequisites
- JUCE 8.x source tree
- CMake 3.22+
- C++17 compiler (Visual Studio 2019/2022 or equivalent)

### JUCE Path

**Option A — environment variable (recommended)**
```powershell
$env:JUCE_PATH = "C:\Path\To\JUCE"
```

**Option B — edit CMakeLists.txt**
Set the `JUCE_PATH` variable to your JUCE installation directory.

### Configure and Build
```bash
cmake -B build
cmake --build build --config Release
```

### Output
```
build/Humvoice_artefacts/Release/VST3/Humvoice.vst3
```

Copy or symlink into your DAW's VST3 scan path and rescan.

---

## Plugin Info

| Field | Value |
|---|---|
| Name | Humvoice |
| Format | VST3 |
| Category | Synth |
| Manufacturer Code | Hmvc |
| Plugin Code | Hmv1 |
| Audio I/O | Stereo in, Stereo out |
| MIDI | Output |

---

## License

Released under the JUCE license. See the JUCE source tree for full terms.
