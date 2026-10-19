"""Import owned PC audio into native DS IMA-ADPCM; requires ffmpeg on PATH.

One 32-bit predictor/index header, low nibble first, no WAV block headers.
DS format reference: https://mgba-emu.github.io/gbatek/#ds-sound
Generated assets are local build inputs, not redistributable source assets.
"""
import argparse
import array
import hashlib
import json
import math
from pathlib import Path
import struct
import subprocess
import sys
import wave

RATE = 16000
PEAK_TARGET = 10 ** (-1.0 / 20) * 32767
STEPS = (7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,
         50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,
         279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,
         1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,
         4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,
         12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767)
INDEX = (-1,-1,-1,-1,2,4,6,8)
TRACKS = (
    # David Orr's Four Brave Champions, the original title/menu loop (NG90433).
    # barrackssong is the opening party music, not the main-menu theme.
    ('menu', 'music/NG90433.xma', True),
    ('arena', 'music/Waterflame_TheShow.xma', True),
    ('campaign', 'music/MUDHOLES.xma', True),
    ('swing', 'sounds/sound_swinger1.xma', False),
    ('hit', 'sounds/sound_meatypunch4.xma', False),
    ('block', 'sounds/sound_shield1.xma', False),
    ('magic_blue', 'sounds/sound_magic_ice.xma', False),
    ('magic_orange', 'sounds/sound_magic_fire.xma', False),
    # PLAYER sprite795 frame11 uses the original s_arrowShoot cue on release.
    ('arrow', 'sounds/sound_arrowshoot.xma', False),
    ('magic_red', 'sounds/sound_magic_lightning.xma', False),
    ('magic_green', 'sounds/sound_magic_poison.xma', False),
)

def encode(samples):
    """Encode DS ADPCM with predictor 0; pad to the hardware word boundary."""
    values = list(samples)
    values.extend([0] * ((-len(values)) % 8))
    predictor = index = 0
    packed = bytearray(struct.pack('<hBB', predictor, index, 0))
    low = 0
    for pos, sample in enumerate(values):
        step = STEPS[index]
        delta = sample - predictor
        code = 8 if delta < 0 else 0
        delta = abs(delta)
        change = step >> 3
        if delta >= step:
            code |= 4
            delta -= step
            change += step
        if delta >= step >> 1:
            code |= 2
            delta -= step >> 1
            change += step >> 1
        if delta >= step >> 2:
            code |= 1
            change += step >> 2
        predictor = max(-32767, min(32767, predictor + (-change if code & 8 else change)))
        index = max(0, min(88, index + INDEX[code & 7]))
        if pos & 1:
            packed.append(low | (code << 4))
        else:
            low = code
    return bytes(packed)

def decode(data):
    predictor, index, reserved = struct.unpack('<hBB', data[:4])
    if index > 88 or reserved or len(data) % 4:
        raise ValueError('Invalid DS ADPCM header or length')
    result = array.array('h')
    for byte in data[4:]:
        for code in (byte & 15, byte >> 4):
            step = STEPS[index]
            change = step >> 3
            if code & 1: change += step >> 2
            if code & 2: change += step >> 1
            if code & 4: change += step
            predictor = max(-32767, min(32767, predictor + (-change if code & 8 else change)))
            index = max(0, min(88, index + INDEX[code & 7]))
            result.append(predictor)
    return result

def levels(samples):
    peak = max((abs(x) for x in samples), default=0)
    rms = math.sqrt(sum(x*x for x in samples) / len(samples)) if samples else 0
    return {'peak_dbfs': round(20*math.log10(peak/32768), 3) if peak else None,
            'rms_dbfs': round(20*math.log10(rms/32768), 3) if rms else None,
            'full_scale_samples': sum(abs(x) >= 32767 for x in samples)}

def normalize_and_encode(samples):
    """One gain per clip preserves dynamics; verify the *decoded* codec peaks.

    Input can exceed int16 range (floating-point decoder/downmix overshoot).
    Start at -1 dBFS, then back off only if the ADPCM predictor overshoots.
    """
    peak = max((abs(x) for x in samples), default=0)
    gain = PEAK_TARGET / peak if peak else 1.0
    for attempt in range(8):
        pcm = array.array('h', (round(x*gain) for x in samples))
        encoded = encode(pcm)
        decoded = decode(encoded)
        decoded_peak = max((abs(x) for x in decoded), default=0)
        if decoded_peak <= PEAK_TARGET:
            return encoded, decoded, gain
        gain *= (PEAK_TARGET / decoded_peak) * 0.97
    raise RuntimeError('Could not encode audio within the -1 dBFS peak ceiling')

def speaker_compress(samples, rate):
    """Offline gentle envelope compression; no DS decoding or RAM overhead.

    Scale only the detector input to a consistent peak, then restore that scale.
    The existing encoder still verifies the final ADPCM ceiling after makeup.
    """
    peak=max((abs(x) for x in samples),default=0)
    if not peak:return samples
    scaled=array.array('f',(x/peak*0.891250938 for x in samples))
    raw=subprocess.check_output([__import__('os').environ.get('FFMPEG','ffmpeg'),'-v','error','-f','f32le','-ar',str(rate),'-ac','1','-i','pipe:0',
        '-af','acompressor=threshold=0.12:ratio=3:attack=0.1:release=90:knee=4:makeup=1:detection=rms',
        '-f','f32le','pipe:1'],input=scaled.tobytes())
    result=array.array('f',raw)
    assert len(result)==len(samples)
    return [x*peak/0.891250938 for x in result]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('game_directory', type=Path)
    parser.add_argument('--ffmpeg', default='ffmpeg')
    parser.add_argument('--output', type=Path, default=Path(__file__).resolve().parents[1] / 'generated')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    declarations = ['/* Imported locally from owned Castle Crashers installation. */',
                    '#ifndef CASTLE_AUDIO_ASSETS_H', '#define CASTLE_AUDIO_ASSETS_H',
                    f'#define CASTLE_AUDIO_RATE {RATE}', '#define CASTLE_AUDIO_MENU_RATE 8000', '#define CASTLE_AUDIO_ARENA_RATE 6000', '#define CASTLE_AUDIO_CAMPAIGN_RATE 5512']
    report = {'format': 'Nintendo DS IMA ADPCM, mono, 4-bit', 'sample_rate': RATE,
              'normalization': 'offline 3:1 RMS envelope compression, 0.1ms attack/90ms release, soft knee; decoded ADPCM peak <= -1 dBFS',
              'master_volume':127, 'menu_volume':127, 'arena_volume':112, 'effect_volume':127,
              'arrow_volume':64, 'tracks': []}
    for name, relative, loop in TRACKS:
        rate = {'menu':8000,'arena':6000,'campaign':5512}.get(name,RATE)
        source = args.game_directory / 'data' / relative
        raw = subprocess.check_output([args.ffmpeg, '-v', 'error', '-i', str(source),
            '-vn', '-ac', '1', '-ar', str(rate), '-f', 'f32le', '-'])
        samples = array.array('f', raw)
        if sys.byteorder != 'little': samples.byteswap()
        if not samples: raise RuntimeError(f'Empty audio: {source}')
        # Float decoding avoids clipping before normalization when xWMA decode
        # and stereo-to-mono summation produce peaks above full scale.
        samples = [float(x)*32768 for x in samples]
        source_levels = levels(samples)
        # Gentle edge fades prevent a pop when full source tracks wrap. The entire
        # track is retained; this does not infer original game's authored loops.
        fade = min(rate // 50, len(samples) // 2)
        for i in range(fade):
            samples[i] = samples[i] * i / fade
            samples[-1-i] = samples[-1-i] * i / fade
        encoded, decoded, gain = normalize_and_encode(speaker_compress(samples,rate))
        if len(decoded) < len(samples) or len(decoded) - len(samples) > 7:
            raise RuntimeError('ADPCM sample count mismatch')
        (args.output / f'audio_{name}.adpcm').write_bytes(encoded)
        declarations.append(f'static const unsigned char castle_audio_{name}[] __attribute__((aligned(32))) = {{')
        declarations.extend(','.join(str(v) for v in encoded[i:i+32])+',' for i in range(0,len(encoded),32))
        declarations.append('};')
        with wave.open(str(args.output / f'audio_{name}_preview.wav'), 'wb') as wav:
            wav.setparams((1,2,rate,0,'NONE','not compressed'))
            if sys.byteorder != 'little': decoded.byteswap()
            wav.writeframes(decoded.tobytes())
        entry = {'name': name, 'source': relative, 'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
                 'source_bytes': source.stat().st_size, 'sample_rate': rate, 'seconds': round(len(samples)/rate,3),
                 'encoded_bytes': len(encoded), 'loop': loop, 'loop_point_words': 1 if loop else 0,
                 'source_float_levels': source_levels, 'gain_db': round(20*math.log10(gain),3),
                 'decoded_levels': levels(decoded)}
        report['tracks'].append(entry)
        print(f'{name}: {entry["seconds"]} s -> {len(encoded):,} bytes')
    declarations.append('#endif')
    (args.output / 'audio_assets.h').write_text('\n'.join(declarations)+'\n')
    report['total_encoded_bytes'] = sum(t['encoded_bytes'] for t in report['tracks'])
    (args.output / 'audio_manifest.json').write_text(json.dumps(report, indent=2)+'\n')

if __name__ == '__main__':
    main()
