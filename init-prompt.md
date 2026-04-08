# TASK: AUTONOMOUS VST3/AU END-TO-END DEVELOPMENT
# PROJECT: RISeed-Industrial (CloudSeed Core)
# PRIORITY: 100% Deterministic CI/CD Deployment

### 1. INFRASTRUCTURE & ASSUMPTIONS
* **Environment**: Assume the repository `RISeed-Industrial` already exists.
* **Secrets**: Assume `GITHUB_TOKEN` is already available as a GitHub Actions secret.
* **Automation**: All build tasks must run exclusively inside GitHub Actions. No manual intervention.
* **Toolchain**: Use CMake exclusively. Do NOT use Projucer.

### 2. CMAKE & DEPENDENCY INTEGRATION
* **JUCE Framework**: Integrate via `add_subdirectory(JUCE)`.
* **Plugin Definition**: Use the JUCE CMake API specifically:
  `juce_add_plugin(RISeed-Industrial FORMATS VST3 AU PRODUCT_NAME "RISeed-Industrial")`
* **Source Declaration (REQUIRED)**:
  - All plugin source files MUST be explicitly declared via `target_sources()`.
* **Linking (REQUIRED)**:
  - All dependencies MUST be explicitly linked using `target_link_libraries()`.
* **CloudSeedCore**:
  - Do NOT link as a prebuilt static library.
  - Compile CloudSeedCore directly by including its source files in the plugin target.

### 3. DSP ARCHITECTURE & AUDIO THREAD SAFETY
* **Core Reverb**:
  - Wrap CloudSeedCore inside a JUCE `AudioProcessor`.

* **Saturation Module**:
  - Pre-fader soft clipper:
    y = tanh(crunch * x)
  - Apply parameter smoothing to `Crunch` to prevent zipper noise.

* **Oversampling**:
  - Use `juce::dsp::Oversampling` at 4x.
  - Apply ONLY to the saturation stage (post-reverb tail).
  - Normalize output gain after oversampling to prevent loudness jumps.

* **Shimmer Algorithm**:
  - Implement pitch shifting (+12 semitones) inside the feedback loop.
  - Use delay-based pitch shifting with crossfade to avoid artifacts.
  - Include a hard limiter in the feedback loop (ceiling: -0.5 dBFS).

* **Audio Safety**:
  - No dynamic allocation or file I/O on the audio thread.
  - Use denormal protection (`juce::ScopedNoDenormals`).

### 4. CLI BRIDGE & BACKGROUND AUTOMATION
* **FileWatcher**:
  - Monitor: `~/RISeed/automation.json`
  - Polling interval: 200ms MAX.
  - Reload ONLY if file modification timestamp changes.

* **Threading**:
  - FileWatcher MUST run on a background thread.
  - NEVER perform file I/O on the audio thread.

* **State Updates**:
  - Use lock-free atomic updates or `AudioProcessorValueTreeState (APVTS)`.

### 5. INDUSTRIAL VECTOR UI & PRESETS
* **Style**:
  - Background: `#0A0A0A`
  - Active Elements: `#32CD32`
  - Font: JUCE default sans-serif
  - No raster assets

* **Layout**:
  - Main: 3 knobs (Mix, Decay, Crunch)
  - Overlay: 5-column "Preset Matrix"

* **Preset Interaction**:
  - Left-click: Load preset
  - Right-click (User column): Save preset

* **Factory Presets (Hardcoded JSON)**:
  - Small Rooms (3)
  - Large Halls (3)
  - Ethereal/Shimmer (3)
  - Glitch/Industrial (3)

### 6. CI/CD WORKFLOW (.github/workflows/build_plugins.yml)
* **Caching (REQUIRED)**:
  - Cache JUCE and build directories:
    key: ${{ runner.os }}-juce-${{ hashFiles('CMakeLists.txt') }}

* **Windows Job**:
  - Runner: `windows-latest`
  - Toolchain: MSVC 2022
  - Build: VST3 (64-bit)

* **macOS Job**:
  - Runner: `macos-14`
  - Build: VST3 + AU
  - MUST produce Universal Binary:
    CMAKE_OSX_ARCHITECTURES="x86_64;arm64"

* **Output Directory (REQUIRED)**:
  - Force all outputs to:
    build/RISeed-Industrial_artefacts/
  - CI must ONLY use this directory for artifacts.

* **Artifacts**:
  - Use `actions/upload-artifact` to upload binaries from the defined output path.

### 7. RELEASE & SCRIPTING
* **Release Action**:
  - Use `softprops/action-gh-release`

* **Tag**:
  - `v1.0.0-beta`

* **Job Dependency (REQUIRED)**:
  - Release job MUST depend on:
    needs: [build-windows, build-macos]
  - Release MUST NOT run unless BOTH builds succeed.

* **Mac Script (`dequarantine.sh`)**:
  xattr -rd com.apple.quarantine *.vst3 *.component

* **Windows Script (`install_vst3.ps1`)**:
  Copy-Item *.vst3 "$env:COMMONPROGRAMFILES\VST3" -Recurse

### 8. BUILD DETERMINISM & FINAL GUARANTEES
* All build steps MUST be deterministic and non-interactive.
* Any deviation from:
  - defined output paths
  - universal binary requirement
  - artifact completeness
  MUST be treated as a FAILED build.

* The system MUST NOT:
  - infer missing configuration
  - assume default paths
  - skip failed steps silently

* The pipeline is considered SUCCESSFUL ONLY IF:
  - Windows VST3 is built
  - macOS VST3 + AU universal binaries are built
  - Artifacts are uploaded
  - Release is created with all assets