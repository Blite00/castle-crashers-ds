"""Import the original LEVEL2 party cue up to its authored frame161 stop."""
import array
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
from convert_audio import speaker_compress,normalize_and_encode, levels

ROOT = Path(__file__).resolve().parents[1]
RATE = 8000
DURATION = 161 / 30

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('game_directory', type=Path)
    parser.add_argument('--ffmpeg', default='ffmpeg')
    args = parser.parse_args()
    source = args.game_directory / 'data/music/barrackssong.xma'
    raw = subprocess.check_output([args.ffmpeg, '-v', 'error', '-i', str(source),
        '-vn', '-ac', '1', '-ar', str(RATE), '-f', 'f32le', '-'])
    samples = [float(x) * 32768 for x in array.array('f', raw)][:round(RATE * DURATION)]
    assert len(samples) == round(RATE * DURATION)
    # Only suppress boundary clicks; preserve the source cue's dynamics.
    fade = RATE // 200
    for i in range(fade):
        samples[i] *= i / fade
        samples[-i-1] *= i / fade
    encoded, decoded, gain = normalize_and_encode(speaker_compress(samples,RATE))
    header = ['#ifndef BARRACKS_AUDIO_H', '#define BARRACKS_AUDIO_H',
        f'#define BARRACKS_AUDIO_RATE {RATE}',
        'static const unsigned char castle_audio_barracks[] __attribute__((aligned(32))) = {']
    header += [','.join(map(str, encoded[i:i+32])) + ',' for i in range(0, len(encoded), 32)]
    header += ['};', '#endif', '']
    (ROOT / 'generated/barracks_audio.h').write_text('\n'.join(header))
    (ROOT / 'generated/audio_barracks.adpcm').write_bytes(encoded)
    report = dict(source='music/barrackssong.xma', source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
        sample_rate=RATE, seconds=len(samples)/RATE, stop_frame=161, loop=False,
        encoded_bytes=len(encoded), gain=gain, decoded_levels=levels(decoded))
    (ROOT / 'generated/barracks-audio-audit.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2))

if __name__ == '__main__':
    main()
