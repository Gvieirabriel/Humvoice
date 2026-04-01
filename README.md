# Humvoice - Minimal JUCE VST3 Plugin

A minimal VST3 plugin built with JUCE and CMake.

## Features

- **AudioProcessor + Editor**: Full plugin architecture with a simple UI
- **Stereo I/O**: Stereo input and output buses
- **MIDI Support**: MIDI output enabled for extensibility
- **Pass-through Audio**: No DSP logic - audio flows through unchanged
- **CMake Build**: No Projucer required

## Project Structure

```
Humvoice/
├── CMakeLists.txt              # CMake configuration
├── CMakePresets.json           # CMake presets
├── .vscode/
│   └── c_cpp_properties.json    # VS Code IntelliSense config
├── .gitignore                  # Git ignore rules
├── src/
│   ├── PluginProcessor.h       # Audio processor interface
│   ├── PluginProcessor.cpp     # Audio processor implementation
│   ├── PluginEditor.h          # Editor interface
│   └── PluginEditor.cpp        # Editor implementation
└── README.md                   # This file
```

## Prerequisites

- JUCE 8.x (source code distribution)
- CMake 3.22+
- C++17 compatible compiler (Visual Studio 2019+)
- Git

## Setup

### 1. Clone JUCE

Download JUCE 8.x from the [official repository](https://github.com/juce-framework/JUCE) and place it alongside this project:

```
Projects/
├── Humvoice/          # This project
└── juce-8.0.12-windows/  # JUCE source (rename to match your version)
    └── JUCE/
```

### 2. Configure JUCE Path

You can configure the JUCE path in several ways:

**Option A: Environment Variable (Recommended)**
```bash
# Windows PowerShell
$env:JUCE_PATH = "C:\Path\To\Your\Projects\juce-8.0.12-windows\JUCE"
```

**Option B: Modify CMakeLists.txt**
Edit the `JUCE_PATH` variable in `CMakeLists.txt` to point to your JUCE installation.

## Building

### Configure

```bash
cmake -B build
```

### Build

```bash
cmake --build build --config Release
```

### Output

The VST3 plugin will be created at:
```
build/Humvoice_artefacts/Release/VST3/Humvoice.vst3
```

## Development

### Adding DSP Logic

Modify `PluginProcessor::processBlock()` in `src/PluginProcessor.cpp`:

```cpp
void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Your DSP code here
    // Example: apply gain
    buffer.applyGain(0.5f);
}
```

### Adding UI Controls

Modify `PluginEditor` classes in `src/PluginEditor.*`:

```cpp
// Add sliders, buttons, etc. in the constructor
addAndMakeVisible(gainSlider);
gainSlider.setRange(0.0, 1.0);
gainSlider.onValueChange = [this] { /* handle value change */ };
```

## Plugin Information

- **Name**: Humvoice
- **Manufacturer**: Humvoice
- **Plugin Code**: Hmv1
- **Manufacturer Code**: Hmvc
- **Category**: Synth
- **Formats**: VST3

## Notes

- The plugin accepts MIDI input/output but doesn't process it (pass-through)
- `processBlock()` is currently a no-op (no audio modification)
- Editor shows basic UI with plugin name
- State save/restore is stubbed but functional

## Usage

1. Open your DAW or VST3 host.
2. Scan paths including `build/Humvoice_artefacts/Release/VST3`.
3. Load `Humvoice.vst3`.
4. MIDI in/out is available; audio is pass-through.

## Build

### Requisitos
- JUCE 8.x source tree
- CMake 3.22+
- Visual Studio 2019/2022 ou equivalente com suporte C++

### Instruções

```powershell
cd "C:\Users\gviei\OneDrive\Documents\Projects\Humvoice"
# Ajuste JUCE_PATH se necessário
$env:JUCE_PATH = "C:\Users\gviei\OneDrive\Documents\juce-8.0.12-windows\JUCE"
cmake -B build
cmake --build build --config Release
```

### Saída
- `build/Humvoice_artefacts/Release/VST3/Humvoice.vst3`

## Open Source Policy

Este é um projeto open source. Por favor, siga estas diretrizes:

- Respeito e cortesia são obrigatórios.
- Assédio, discurso de ódio ou conduta tóxica não são permitidos.
- Não coloque credenciais, chaves ou dados sensíveis em código/pull requests.
- Use `CMakeLists.local.txt` para path/variáveis locais.

## Contribuindo

1. Fork e clone este repositório.
2. Crie branch de feature: `feature/nome`.
3. Faça alterações com commits atômicos e mensagens claras.
4. Execute build e testes.
5. Abra PR com descrição e casos de uso.

## License

Este projeto segue a mesma licença do JUCE (ver arquivo de licenciamento JUCE).
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test the build
5. Submit a pull request

## License

This project is released under the same license as JUCE. See JUCE license for details.
