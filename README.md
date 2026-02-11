# KNDL Synth

**Sintetizador VST3/Standalone experimental orientado a la modulacion compleja y el diseno sonoro.**

KNDL Synth es un instrumento virtual polifonico (16 voces) construido con JUCE, disenado para la creacion de sonidos modulados, texturas experimentales, pads evolutivos y bajos agresivos. Su arquitectura combina sintesis substractiva clasica con un sistema de modulacion avanzado (Spellbook + Mod Matrix) que permite generar movimiento y complejidad sonora mas alla de los sintetizadores convencionales.

---

## Arquitectura General

```
MIDI In / Sequencer
        |
   +---------+
   | Voice x16| (polifonia)
   |  OSC1 ---+
   |  OSC2    |---> Mixer ---> Filter ---> Amp Envelope ---> Output
   |  SUB  ---+
   +---------+
        |
   Effects Chain: Distortion -> Chorus -> Delay -> Reverb -> OTT
        |
   DC Blocker -> Master Gain -> Soft Clipper -> DAW
```

Cada voz contiene sus propios osciladores, filtro y envelopes. Las modulaciones (LFO, Spellbook, Envelopes, Velocity) se aplican globalmente a traves de la Modulation Matrix.

---

## Componentes

### Osciladores

El sintetizador cuenta con **3 fuentes de sonido** por voz:

| Componente | Formas de onda | Controles |
|-----------|---------------|-----------|
| **OSC 1** | Sine, Triangle, Saw, Square | Level, Detune (+-100 cents), Octave (+-2), Enable |
| **OSC 2** | Sine, Triangle, Saw, Square | Level, Detune (+-100 cents), Octave (+-2), Enable |
| **SUB** | Sine (sub-oscilador) | Level, Octave (-2 a 0), Enable |

- Los osciladores Saw y Square usan **PolyBLEP** para reduccion de aliasing.
- Cada oscilador tiene un boton ON/OFF que lo silencia completamente en el DSP.

### Filtro

4 modos de filtro disponibles:

| Modo | Descripcion | Controles especificos |
|------|------------|----------------------|
| **SVF** | State Variable Filter clasico | Low Pass / High Pass / Band Pass |
| **Formant** | Filtro de formantes vocales | Vocales: A, E, I, O, U |
| **Comb** | Filtro de peine (delay-based) | Crea resonancias metalicas |
| **Notch** | Filtro de rechazo de banda | Elimina frecuencias especificas |

**Controles comunes:** Cutoff (20-20kHz), Resonance, Drive, Env Amount (-1 a +1).

### Envelopes (ADSR)

Dos envelopes independientes:

- **AMP Envelope**: Controla el volumen de la voz. Define el contorno dinamico de cada nota (Attack, Decay, Sustain, Release).
- **FLT Envelope**: Modula el cutoff del filtro. Permite barridos de filtro automaticos al pulsar una nota.

### LFOs

2 LFOs independientes con formas de onda seleccionables:

| Forma | Descripcion |
|-------|------------|
| **SIN** | Onda sinusoidal suave |
| **TRI** | Onda triangular |
| **SAW** | Rampa ascendente |
| **SQR** | Onda cuadrada (on/off) |

- Rango: 0.1 - 20 Hz
- Soporte de sync al tempo del DAW
- Cada LFO puede tener una forma de onda diferente

### Spellbook (Modulador Geometrico)

El **Spellbook** es un modulador unico de KNDL Synth. Traza un punto a lo largo de formas geometricas y genera senales de modulacion a partir de las coordenadas X/Y de esa trayectoria.

**7 formas disponibles:**

| Forma | Descripcion |
|-------|------------|
| **Circle** | Circulo - modulacion suave y ciclica |
| **Triangle** | Triangulo equilatero - transiciones angulares |
| **Square** | Cuadrado - cambios abruptos en las esquinas |
| **Pentagon** | Pentagono - patron de 5 puntos |
| **Star** | Estrella de 5 puntas - modulacion radical con picos |
| **Spiral** | Espiral de Arquimedes - radio creciente |
| **Lemniscate** | Lemniscata (simbolo infinito) - patron en forma de 8 |

- **Rate**: 0.01 Hz a 20 kHz (puede alcanzar audio-rate para FM)
- **Outputs**: Hasta 16 salidas con multiplicadores de velocidad
- Las salidas SB.A, SB.B, SB.C, SB.D estan disponibles como fuentes de modulacion

### Modulation Matrix

El corazon del diseno sonoro experimental. Permite conectar **cualquier fuente** a **cualquier destino** con cantidad bipolar.

**8 slots de modulacion**, cada uno con:
- **Source** (fuente de modulacion)
- **Destination** (parametro a modular)
- **Amount** (-1.0 a +1.0, bipolar)

#### Fuentes disponibles

| Fuente | Descripcion |
|--------|------------|
| LFO1, LFO2 | Osciladores de baja frecuencia |
| AmpEnv | Envelope de amplitud |
| FilterEnv | Envelope del filtro |
| Velocity | Velocidad MIDI de la nota |
| ModWheel | Rueda de modulacion (CC1) |
| Aftertouch | Presion posterior MIDI |
| SB.A, SB.B | Spellbook salida 1 (X, Y) |
| SB.C, SB.D | Spellbook salida 2 (X, Y) |

#### Destinos disponibles

| Destino | Descripcion |
|---------|------------|
| Osc1Pitch, Osc2Pitch | Tono de los osciladores (en semitonos) |
| Osc1Level, Osc2Level | Nivel de los osciladores |
| SubLevel | Nivel del sub-oscilador |
| FilterCutoff | Frecuencia de corte del filtro |
| FilterReso | Resonancia del filtro |
| AmpLevel | Nivel de amplitud general |
| LFO1Rate, LFO2Rate | Velocidad de los LFOs (meta-modulacion) |

### Cadena de Efectos

5 efectos en serie, cada uno con bypass independiente:

| Efecto | Descripcion | Controles |
|--------|------------|-----------|
| **Distortion** | Saturacion/distorsion | Drive, Mix |
| **Chorus** | Efecto de coro (modulacion de delay) | Rate, Depth, Mix |
| **Delay** | Eco/repeticion temporal | Time (10-1000ms), Feedback, Mix |
| **Reverb** | Reverberacion (Schroeder, 4 comb + 2 allpass) | Size, Damping, Mix |
| **OTT** | Compresor multibanda agresivo (3 bandas) | Depth, Time, Mix |

### Secuenciador Interno

Generador de notas MIDI integrado para previsualizar sonidos sin controlador externo.

**8 patrones:**

| Patron | Descripcion |
|--------|------------|
| Minor Scale | Escala menor ascendente/descendente |
| Major Arpeggio | Arpegio de triada mayor |
| Minor Arpeggio | Arpegio de triada menor |
| Fifths | Patron de quintas (power chords) |
| Chromatic | Corrida cromatica |
| Random | Notas aleatorias de escala pentatonica menor |
| Chord Stabs | Progresion de acordes (i-VI-III-VII) |
| Drone | Nota sostenida |

- Tempo: 40 - 300 BPM
- Octava base: 1 - 7
- Timing sample-accurate

---

## Seccion Monitor

La interfaz incluye una seccion de monitoreo en tiempo real:

| Display | Que muestra |
|---------|------------|
| **Scope** | Forma de onda de la salida master |
| **Filt.Resp** | Curva de respuesta del filtro segun el modo activo |
| **SB.Shape** | Visualizacion de la forma del Spellbook con posicion actual y trail |
| **Data** | Valores numericos: nota, envelopes, LFOs, Spellbook outputs, nivel de salida |

---

## Presets

KNDL Synth incluye **presets de fabrica** que se instalan automaticamente en `~/Documents/KndlSynth/Presets/`:

| Preset | Categoria | Descripcion |
|--------|----------|------------|
| Init | Basic | Patch inicial limpio |
| Deep Bass | Bass | Bajo profundo con sub |
| Hypnotic Pad | Pad | Pad con LFO en filtro |
| Acid Lead | Lead | Lead acido con resonancia |
| Reese Bass | Bass | Bajo Reese con detune |
| Techno Stab | Synth | Stab de techno con decay corto |
| Dark Atmosphere | Ambient | Atmosfera oscura con reverb |
| Psychedelic Drone | Ambient | Drone psicodelico con chorus |
| Formant Choir | Pad | Coro con filtro de formantes |
| Comb Pluck | Pluck | Pluck metalico con filtro comb |
| Spellbook Pad | Pad | Pad modulado con Spellbook |
| Notch Sweep | FX | Barrido de filtro notch |
| Vowel Bass | Bass | Bajo con vocales (formant) |
| Spiral Texture | FX | Textura con espiral del Spellbook |
| OTT Supersaw | Lead | Supersaw comprimido con OTT |
| Square Wobble | Bass | Wobble bass con LFO cuadrado |
| Lemniscate Keys | Keys | Teclado con modulacion en infinito |
| Metallic Ring | FX | Timbre metalico con filtro comb |

---

## Glosario

| Sigla / Termino | Significado |
|-----------------|------------|
| **VST3** | Virtual Studio Technology 3 - formato de plugin de audio |
| **MIDI** | Musical Instrument Digital Interface - protocolo de control musical |
| **OSC** | Oscillator (Oscilador) - genera la forma de onda base |
| **SUB** | Sub-Oscillator - oscilador adicional una o dos octavas por debajo |
| **SVF** | State Variable Filter - filtro que puede ser LP, HP o BP simultaneamente |
| **LP / HP / BP** | Low Pass / High Pass / Band Pass - tipos de filtro |
| **ADSR** | Attack, Decay, Sustain, Release - fases de un envelope |
| **LFO** | Low Frequency Oscillator - oscilador lento para modulacion |
| **Mod Matrix** | Modulation Matrix - sistema de ruteo de modulacion flexible |
| **SB** | Spellbook - modulador geometrico exclusivo de KNDL |
| **FM** | Frequency Modulation - modulacion de frecuencia (cuando Spellbook opera a audio-rate) |
| **OTT** | Over The Top - compresor multibanda agresivo (3 bandas) |
| **BPM** | Beats Per Minute - tempo musical |
| **CC** | Control Change - mensaje MIDI de controlador |
| **Cutoff** | Frecuencia de corte del filtro |
| **Reso** | Resonance - enfasis de la frecuencia de corte del filtro |
| **Drive** | Ganancia de saturacion aplicada al filtro |
| **Env** | Envelope - envolvente que controla la evolucion temporal de un parametro |
| **Detune** | Desafinacion en cents entre osciladores |
| **Cents** | Unidad de afinacion (100 cents = 1 semitono) |
| **Semitono** | Intervalo minimo en la escala cromatica (ej: Do a Do#) |
| **Bipolar** | Rango de -1 a +1 (modulacion que puede ir en ambas direcciones) |
| **Unipolar** | Rango de 0 a 1 (modulacion en una sola direccion) |
| **PolyBLEP** | Polymorph Band-Limited Step - tecnica anti-aliasing para osciladores |
| **DC Blocker** | Filtro que elimina offset de corriente continua de la senal |
| **Schroeder** | Algoritmo de reverb basado en filtros comb y allpass |
| **Comb Filter** | Filtro de peine - crea resonancias metalicas por retroalimentacion de delay corto |
| **Notch Filter** | Filtro de rechazo de banda - elimina una frecuencia especifica |
| **Formant** | Resonancias del tracto vocal que definen las vocales (A, E, I, O, U) |
| **Lemniscate** | Curva en forma de infinito (lemniscata de Bernoulli) |
| **Gate** | Duracion de la nota en relacion al step del secuenciador |
| **Velocity** | Intensidad con la que se pulsa una tecla MIDI (0-127) |
| **Aftertouch** | Presion aplicada a la tecla despues de pulsarla |
| **ModWheel** | Rueda de modulacion - controlador fisico MIDI (CC1) |
| **Voice Stealing** | Reasignacion de una voz activa cuando se excede la polifonia |
| **DAW** | Digital Audio Workstation - software de produccion musical |

---

## Requisitos Tecnicos

- **Formato**: VST3, Standalone
- **Polifonia**: 16 voces
- **Sample Rate**: Adaptativo (44.1kHz - 192kHz)
- **Framework**: JUCE 8
- **Lenguaje**: C++17
- **Plataforma**: macOS (ARM/x86_64)

## Build

```bash
# Clonar con submodulos
git clone --recursive https://github.com/tu-usuario/kndl-synth.git

# Configurar y compilar
cd kndl-synth
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# El VST3 se genera en:
# build/KndlSynth_artefacts/Release/VST3/KndlSynth.vst3

# El Standalone se genera en:
# build/KndlSynth_artefacts/Release/Standalone/KndlSynth.app
```

## Tests

```bash
cmake --build build --target KndlSynthTests
./build/KndlSynthTests_artefacts/Debug/KndlSynthTests
```

---

*KNDL Synth - Diseno sonoro experimental sin limites.*
