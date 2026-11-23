#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Genera la musica y los efectos de sonido de Operacion Macondo (estilo chiptune retro).

Uso:   python3 herramientas/generar_audio.py
Requiere: numpy.  Escribe los .wav en assets/audio/ (mono, 16 bits, 22050 Hz).

Todo se sintetiza con las mismas ideas de las consolas de 8 bits:
  - onda cuadrada (melodia y arpegios), onda triangular (bajo) y ruido (percusion y explosiones)
  - un ligero "bitcrush" (se reduce la resolucion) para el sonido sucio y nostalgico

Para cambiar algo (tempo, notas, efectos) se edita este archivo y se vuelve a ejecutar.
"""
import os
import wave

import numpy as np

SR = 22050
rng = np.random.default_rng(1985)
SALIDA = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "assets", "audio"))

NOTAS = {'C': 0, 'C#': 1, 'D': 2, 'D#': 3, 'E': 4, 'F': 5, 'F#': 6, 'G': 7, 'G#': 8, 'A': 9, 'A#': 10, 'B': 11}


def midi(nombre):
    """'A5' -> 81, 'G#5' -> 80"""
    if len(nombre) > 2 and nombre[1] == '#':
        pc, octava = NOTAS[nombre[:2]], int(nombre[2:])
    else:
        pc, octava = NOTAS[nombre[0]], int(nombre[1:])
    return 12 * (octava + 1) + pc


def hz(m):
    return 440.0 * 2.0 ** ((m - 69) / 12.0)


# ----------------------------------------------------------------------------- sintesis basica

def onda(fase, tipo, duty=0.5):
    f = fase % 1.0
    if tipo == 'cuadrada':
        return np.where(f < duty, 1.0, -1.0)
    if tipo == 'triangulo':
        return 4.0 * np.abs(f - 0.5) - 1.0
    if tipo == 'seno':
        return np.sin(2 * np.pi * fase)
    raise ValueError(tipo)


def adsr(t, dur, a, d, s, r):
    a = min(a, dur / 2.0)
    d = min(d, dur - a)
    return np.interp(t, [0, a, a + d, dur, dur + r], [0, 1, s, s, 0])


def tono(frec, dur, tipo='cuadrada', duty=0.5, vol=1.0, a=0.003, d=0.06, s=0.7, r=0.04,
         vib=0.0, vib_hz=5.5, hasta=None):
    """Una nota. 'hasta' hace un deslizamiento (glissando) hasta esa frecuencia."""
    n = int((dur + r) * SR)
    t = np.arange(n) / SR
    if hasta is None:
        f = np.full(n, float(frec))
    else:
        f = frec * (hasta / frec) ** np.clip(t / (dur + r), 0, 1)
    if vib > 0:
        f = f * (1.0 + vib * np.sin(2 * np.pi * vib_hz * t) * np.clip(t / 0.15, 0, 1))
    fase = np.cumsum(f) / SR
    return vol * onda(fase, tipo, duty) * adsr(t, dur, a, d, s, r)


def poner(buf, sig, inicio, circular=False):
    i = int(round(inicio * SR))
    n = len(sig)
    if circular:
        idx = (np.arange(n) + i) % len(buf)
        buf[idx] += sig                      # la cola de la nota vuelve al inicio: bucle sin cortes
    else:
        fin = min(len(buf), i + n)
        if fin > i:
            buf[i:fin] += sig[:fin - i]


def ruido_retro(n, mantener=2):
    """Ruido tipo canal de ruido de NES (valores que se mantienen unas muestras)."""
    return np.repeat(rng.uniform(-1, 1, n // mantener + 1), mantener)[:n]


def pasa_bajos(x, alfa_ini, alfa_fin):
    """Filtro de un polo con frecuencia de corte que cambia con el tiempo."""
    y = np.zeros_like(x)
    alfas = np.linspace(alfa_ini, alfa_fin, len(x))
    acc = 0.0
    for i in range(len(x)):
        acc += alfas[i] * (x[i] - acc)
        y[i] = acc
    return y


# ----------------------------------------------------------------------------- percusion

def bombo(vol=1.0):
    n = int(0.18 * SR)
    t = np.arange(n) / SR
    f = 42 + 140 * np.exp(-t * 22)
    return vol * np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t * 20)


def caja(vol=1.0):
    n = int(0.15 * SR)
    t = np.arange(n) / SR
    return vol * (0.8 * ruido_retro(n) * np.exp(-t * 26) + 0.35 * np.sin(2 * np.pi * 190 * t) * np.exp(-t * 40))


def hihat(vol=1.0, abierto=False):
    n = int((0.2 if abierto else 0.05) * SR)
    t = np.arange(n) / SR
    r = np.diff(rng.uniform(-1, 1, n), prepend=0.0)
    return vol * r * np.exp(-t * (16 if abierto else 80))


# ----------------------------------------------------------------------------- musica

ACORDES = {            # nombre: (nota raiz, intervalos del acorde)
    'Am': ('A', [0, 3, 7]), 'Dm': ('D', [0, 3, 7]),
    'F': ('F', [0, 4, 7]), 'C': ('C', [0, 4, 7]), 'G': ('G', [0, 4, 7]), 'E': ('E', [0, 4, 7]),
}

# 16 compases en La menor: tema principal (cada compas dura 4 tiempos)
PROGRESION = ['Am', 'F', 'C', 'G', 'Am', 'F', 'C', 'E',
              'Dm', 'G', 'C', 'F', 'Dm', 'E', 'Am', 'Am']

# (tiempo de inicio dentro del compas, duracion en tiempos, nota)
MELODIA = [
    [(0, .5, 'E5'), (.5, .5, 'A5'), (1, 1, 'C6'), (2, .5, 'B5'), (2.5, .5, 'A5'), (3, 1, 'E5')],
    [(0, .5, 'F5'), (.5, .5, 'A5'), (1, 1, 'C6'), (2, .5, 'A5'), (2.5, .5, 'F5'), (3, 1, 'A5')],
    [(0, .5, 'G5'), (.5, .5, 'E5'), (1, 1, 'C5'), (2, .5, 'E5'), (2.5, .5, 'G5'), (3, 1, 'C6')],
    [(0, .5, 'B5'), (.5, .5, 'A5'), (1, .5, 'G5'), (1.5, .5, 'A5'), (2, 2, 'B5')],
    [(0, .5, 'E5'), (.5, .5, 'A5'), (1, 1, 'C6'), (2, .5, 'D6'), (2.5, .5, 'C6'), (3, 1, 'B5')],
    [(0, .5, 'A5'), (.5, .5, 'C6'), (1, 1, 'F6'), (2, .5, 'E6'), (2.5, .5, 'C6'), (3, 1, 'A5')],
    [(0, .5, 'G5'), (.5, .5, 'C6'), (1, 1, 'E6'), (2, .5, 'D6'), (2.5, .5, 'C6'), (3, 1, 'G5')],
    [(0, .5, 'E5'), (.5, .5, 'G#5'), (1, .5, 'B5'), (1.5, .5, 'G#5'), (2, 1, 'B5'), (3, 1, 'G#5')],
    [(0, .5, 'D5'), (.5, .5, 'F5'), (1, 1, 'A5'), (2, .5, 'F5'), (2.5, .5, 'D5'), (3, 1, 'A4')],
    [(0, .5, 'D5'), (.5, .5, 'G5'), (1, 1, 'B5'), (2, .5, 'A5'), (2.5, .5, 'G5'), (3, 1, 'D5')],
    [(0, .5, 'E5'), (.5, .5, 'G5'), (1, 1, 'C6'), (2, .5, 'B5'), (2.5, .5, 'C6'), (3, 1, 'E6')],
    [(0, .5, 'C6'), (.5, .5, 'A5'), (1, 1, 'F5'), (2, .5, 'A5'), (2.5, .5, 'C6'), (3, 1, 'A5')],
    [(0, 1, 'D6'), (1, .5, 'C6'), (1.5, .5, 'A5'), (2, 1, 'F5'), (3, 1, 'A5')],
    [(0, 1, 'B5'), (1, .5, 'G#5'), (1.5, .5, 'B5'), (2, 1, 'E6'), (3, 1, 'D6')],
    [(0, 1, 'C6'), (1, .5, 'B5'), (1.5, .5, 'A5'), (2, 1, 'E5'), (3, 1, 'A5')],
    [(0, .5, 'E5'), (.5, .5, 'A5'), (1, .5, 'C6'), (1.5, .5, 'E6'), (2, 2, 'E6')],
]

PATRON_ARPEGIO = [0, 1, 2, 3, 2, 1, 2, 1]


def componer_tema(bpm, progresion, melodia, transposicion=0, rapido=False):
    beat = 60.0 / bpm
    compas = 4 * beat
    n = int(round(len(progresion) * compas * SR))
    buf = np.zeros(n)

    for c, (acorde, notas) in enumerate(zip(progresion, melodia)):
        t0 = c * compas
        letra, intervalos = ACORDES[acorde]

        # bajo (triangulo) con saltos de octava
        raiz = 12 * 3 + NOTAS[letra] + transposicion          # octava 2
        for ini, nota in [(0, raiz), (.5, raiz + 12), (1, raiz), (1.5, raiz + 7),
                          (2, raiz), (2.5, raiz + 12), (3, raiz), (3.5, raiz + 7)]:
            poner(buf, tono(hz(nota), beat * 0.42, 'triangulo', vol=0.55, a=0.002, d=0.05, s=0.8, r=0.02),
                  t0 + ini * beat, True)
            poner(buf, tono(hz(nota), beat * 0.30, 'cuadrada', duty=0.125, vol=0.07, a=0.002, d=0.04, s=0.5, r=0.02),
                  t0 + ini * beat, True)

        # arpegio rapido (cuadrada 25 %)
        base = 12 * 5 + NOTAS[letra] + transposicion          # octava 4
        tonos = [base, base + intervalos[1], base + intervalos[2], base + 12]
        pasos = 16
        for i in range(pasos):
            m = tonos[PATRON_ARPEGIO[i % 8]]
            poner(buf, tono(hz(m), beat * 0.2, 'cuadrada', duty=0.25, vol=0.11, a=0.002, d=0.03, s=0.4, r=0.02),
                  t0 + i * beat / 4, True)

        # melodia principal (cuadrada 50 %)
        for ini, dur, nota in notas:
            largo = dur >= 1.0
            poner(buf, tono(hz(midi(nota) + transposicion), dur * beat * 0.92, 'cuadrada', duty=0.5, vol=0.30,
                            a=0.004, d=0.08, s=0.75, r=0.05, vib=0.004 if largo else 0.0),
                  t0 + ini * beat, True)

        # percusion
        if rapido:
            for k in range(4):
                poner(buf, bombo(0.65), t0 + k * beat, True)
            for ini in (1, 3):
                poner(buf, caja(0.55), t0 + ini * beat, True)
            for k in range(16):
                poner(buf, hihat(0.18), t0 + k * beat / 4, True)
        else:
            for ini in (0, 1.5, 2):
                poner(buf, bombo(0.70), t0 + ini * beat, True)
            for ini in (1, 3):
                poner(buf, caja(0.50), t0 + ini * beat, True)
            for k in range(8):
                poner(buf, hihat(0.16 if k % 2 else 0.10), t0 + k * beat / 2, True)
            poner(buf, hihat(0.15, abierto=True), t0 + 3.5 * beat, True)

    return buf


# ----------------------------------------------------------------------------- efectos

def secuencia(partes, duracion):
    buf = np.zeros(int(duracion * SR))
    for sig, ini in partes:
        poner(buf, sig, ini)
    return buf


def sfx_carril():
    return tono(520, 0.04, 'cuadrada', 0.25, 0.6, d=0.03, s=0.3, r=0.03, hasta=900)


def sfx_cuenta():
    return tono(880, 0.12, 'cuadrada', 0.5, 0.8, a=0.002, d=0.05, s=0.6, r=0.06)


def sfx_ya():
    return secuencia([(tono(880, 0.10, 'cuadrada', 0.5, 0.8, r=0.03), 0.0),
                      (tono(1320, 0.40, 'cuadrada', 0.5, 0.8, d=0.15, s=0.6, r=0.15), 0.12),
                      (tono(660, 0.40, 'cuadrada', 0.25, 0.3, d=0.15, s=0.6, r=0.15), 0.12)], 0.85)


def sfx_lanzamiento():
    n = int(0.32 * SR)
    t = np.arange(n) / SR
    r = pasa_bajos(rng.uniform(-1, 1, n), 0.04, 0.45)
    env = np.sin(np.pi * np.clip(t / 0.32, 0, 1)) ** 1.5
    silbido = np.sin(2 * np.pi * np.cumsum(900 * (300 / 900) ** (t / 0.32)) / SR) * 0.25
    return (r * 2.2 + silbido) * env


def tintineo(duracion, cantidad, vol):
    n = int(duracion * SR)
    buf = np.zeros(n)
    for _ in range(cantidad):
        f = rng.uniform(2400, 5200)
        ini = rng.uniform(0, duracion * 0.6)
        d = rng.uniform(0.05, 0.14)
        m = int(d * SR)
        tt = np.arange(m) / SR
        poner(buf, vol * np.sin(2 * np.pi * f * tt) * np.exp(-tt * 38), ini)
    return buf


def sfx_impacto_botella():
    n = int(0.45 * SR)
    t = np.arange(n) / SR
    vidrio = np.diff(rng.uniform(-1, 1, n), prepend=0.0) * np.exp(-t * 11) * 0.8
    ay = tono(520, 0.16, 'cuadrada', 0.5, 0.55, a=0.002, d=0.06, s=0.5, r=0.05, hasta=130)
    return secuencia([(vidrio, 0.0), (tintineo(0.45, 9, 0.35), 0.0), (ay, 0.02)], 0.45)


def sfx_botella_fallada():
    n = int(0.30 * SR)
    t = np.arange(n) / SR
    vidrio = np.diff(rng.uniform(-1, 1, n), prepend=0.0) * np.exp(-t * 16) * 0.5
    return secuencia([(vidrio, 0.0), (tintineo(0.3, 6, 0.30), 0.0)], 0.3)


def sfx_hueco():
    caida = tono(520, 0.5, 'cuadrada', 0.5, 0.6, a=0.005, d=0.2, s=0.5, r=0.08, hasta=70, vib=0.03, vib_hz=9)
    return secuencia([(caida, 0.0), (bombo(0.9), 0.5)], 0.75)


def sfx_premio():
    notas = ['E5', 'G5', 'C6', 'E6', 'G6']
    partes = []
    for i, nombre in enumerate(notas):
        ultimo = i == len(notas) - 1
        dur = 0.30 if ultimo else 0.055
        partes.append((tono(hz(midi(nombre)), dur, 'cuadrada', 0.25, 0.55, d=0.1, s=0.6, r=0.12 if ultimo else 0.03), i * 0.06))
        partes.append((tono(hz(midi(nombre)) / 2, dur, 'triangulo', 0.5, 0.35, d=0.1, s=0.6, r=0.12 if ultimo else 0.03), i * 0.06))
    return secuencia(partes, 0.85)


def sfx_rapidin_fin():
    partes = [(tono(hz(midi(nm)), 0.08, 'cuadrada', 0.5, 0.5, r=0.04), i * 0.11) for i, nm in enumerate(['G5', 'E5', 'C5'])]
    partes.append((tono(hz(midi('G4')), 0.18, 'triangulo', 0.5, 0.6, r=0.1), 0.34))
    return secuencia(partes, 0.7)


def sfx_choque():
    n = int(1.1 * SR)
    t = np.arange(n) / SR
    explosion = pasa_bajos(ruido_retro(n, 2), 0.7, 0.04) * np.exp(-t * 3.2) * 3.0
    golpe = np.sin(2 * np.pi * np.cumsum(35 + 110 * np.exp(-t * 7)) / SR) * np.exp(-t * 5) * 0.9
    chasquido = np.diff(rng.uniform(-1, 1, n), prepend=0.0) * np.exp(-t * 40) * 0.7
    return explosion + golpe + chasquido


def sfx_victoria():
    b = 0.11
    notas = [(0, 'G4', 1), (1, 'C5', 1), (2, 'E5', 1), (3, 'G5', 1), (4, 'E5', 1), (5, 'G5', 1)]
    partes = []
    for ini, nm, d in notas:
        partes.append((tono(hz(midi(nm)), d * b * 0.9, 'cuadrada', 0.5, 0.5, r=0.03), ini * b))
        partes.append((tono(hz(midi(nm)) / 2, d * b * 0.9, 'triangulo', 0.5, 0.4, r=0.03), ini * b))
    t_final = 6 * b
    for nm, vol, tipo in [('C6', 0.5, 'cuadrada'), ('E5', 0.3, 'cuadrada'), ('G5', 0.3, 'cuadrada')]:
        partes.append((tono(hz(midi(nm)), 0.9, tipo, 0.5 if nm == 'C6' else 0.25, vol, a=0.01, d=0.3, s=0.7, r=0.4,
                            vib=0.004), t_final))
    partes.append((tono(hz(midi('C3')), 0.9, 'triangulo', 0.5, 0.6, a=0.01, d=0.3, s=0.8, r=0.4), t_final))
    # destellos finales
    for i, nm in enumerate(['C6', 'E6', 'G6', 'C7']):
        partes.append((tono(hz(midi(nm)), 0.06, 'cuadrada', 0.25, 0.25, r=0.1), t_final + 0.55 + i * 0.09))
    return secuencia(partes, t_final + 1.9)


def sfx_derrota():
    partes = []
    notas = [('A4', 0.0, 0.32), ('G#4', 0.38, 0.32), ('G4', 0.76, 0.32)]
    for nm, ini, d in notas:
        partes.append((tono(hz(midi(nm)), d, 'cuadrada', 0.5, 0.5, d=0.1, s=0.6, r=0.05, vib=0.01), ini))
        partes.append((tono(hz(midi(nm)) / 2, d, 'triangulo', 0.5, 0.45, r=0.05), ini))
    partes.append((tono(hz(midi('F#4')), 0.9, 'cuadrada', 0.5, 0.5, a=0.01, d=0.2, s=0.7, r=0.3, vib=0.02, vib_hz=6,
                        hasta=hz(midi('F#4')) * 0.6), 1.14))
    partes.append((tono(hz(midi('A2')), 1.0, 'triangulo', 0.5, 0.6, a=0.01, d=0.3, s=0.8, r=0.4), 1.14))
    return secuencia(partes, 2.9)


# ----------------------------------------------------------------------------- salida

def guardar(nombre, x, pico=0.9, crush=127):
    x = np.asarray(x, dtype=float)
    x = x / max(1e-9, np.max(np.abs(x))) * pico
    x = np.round(x * crush) / crush                  # bitcrush suave (sonido de 8 bits)
    datos = (x * 32767).astype('<i2')
    ruta = os.path.join(SALIDA, nombre)
    with wave.open(ruta, 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(datos.tobytes())
    print(f"{nombre:24s} {len(x) / SR:6.2f} s  {os.path.getsize(ruta) / 1024:7.0f} KB")


def main():
    os.makedirs(SALIDA, exist_ok=True)

    guardar("musica_nivel.wav", componer_tema(150, PROGRESION, MELODIA), pico=0.8)
    guardar("musica_rapidin.wav", componer_tema(185, PROGRESION[:8], MELODIA[:8], transposicion=3, rapido=True), pico=0.8)

    guardar("carril.wav", sfx_carril())
    guardar("cuenta.wav", sfx_cuenta())
    guardar("ya.wav", sfx_ya())
    guardar("lanzamiento.wav", sfx_lanzamiento())
    guardar("impacto_botella.wav", sfx_impacto_botella())
    guardar("botella_fallada.wav", sfx_botella_fallada(), pico=0.7)
    guardar("hueco.wav", sfx_hueco())
    guardar("premio.wav", sfx_premio())
    guardar("rapidin_fin.wav", sfx_rapidin_fin())
    guardar("choque.wav", sfx_choque(), pico=0.95)
    guardar("victoria.wav", sfx_victoria())
    guardar("derrota.wav", sfx_derrota())


if __name__ == "__main__":
    main()
