# AXIOMA — Made By ORYMY

Sintetizador VST3 (Windows) con interfaz negro/blanco futurista.

- 3 osciladores *morph* (seno → triángulo → sierra → cuadrada, continuo) con octava, semitono y fine
- Unison hasta 7 voces con spread, 16 voces de polifonía
- Filtro LP/BP/HP con envolvente propia, ampli ADSR
- LFO (Sine, Tri, Saw, Square, S&H) → Cutoff / Pitch / Wave / Amp (rueda de modulación controla la profundidad)
- Arpeggiator (Up, Down, Up/Down, Random, Played · 1/4–1/32 · 1–4 octavas · gate) sincronizado al BPM del host
- Chorus, Delay ping-pong, Reverb
- **20.000 presets**: Pads, Keys, Lead, Arp, Bass, Pluck, Bells, FX (2.500 por categoría), con buscador

## Compilar en GitHub (sin instalar nada en tu PC)

1. Crea un repositorio nuevo en GitHub (vacío).
2. En esta carpeta:
   ```
   git init
   git add .
   git commit -m "AXIOMA 1.0"
   git branch -M main
   git remote add origin https://github.com/TU_USUARIO/AXIOMA.git
   git push -u origin main
   ```
3. En GitHub entra a **Actions → Build AXIOMA** y espera (~10–15 min la primera vez).
4. Abajo en la ejecución, en **Artifacts**, descarga **AXIOMA-setup** → dentro está **setup.exe**.
5. Ejecuta `setup.exe`; instala en `C:\Program Files\Common Files\VST3\AXIOMA.vst3`.
6. En FL Studio: *Options → Manage plugins → Find plugins* y busca **AXIOMA** (Instrument).

Para publicar una Release con el setup.exe adjunto: `git tag v1.0.0 && git push --tags`.

## Compilar local (opcional)
```
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target AXIOMA_VST3
```

## Estructura
- `Source/Params.h` — parámetros y generador de los 20.000 presets (determinista: el preset #N siempre suena igual)
- `Source/PluginProcessor.*` — motor de síntesis, arpegiador, efectos
- `Source/PluginEditor.*` — interfaz
- `installer/AXIOMA.iss` — script de Inno Setup (genera `setup.exe`)
- `.github/workflows/build.yml` — compilación automática
