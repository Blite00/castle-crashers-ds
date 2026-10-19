"""Codec contract checks; hardware playback still requires a real DS."""
import math
import json
from pathlib import Path
import struct
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from convert_audio import encode, decode, normalize_and_encode, speaker_compress, PEAK_TARGET

class AudioCodecTests(unittest.TestCase):
    def test_cinematic_nitrofs_banks(self):
        root = Path(__file__).resolve().parents[1]
        audit = json.loads((root / 'generated/cinematic-audio-audit.json').read_text())
        banks = [(root / f'nitrofiles/audio/{name}.bin').read_bytes() for name in ('cinematic', 'barracks')]
        self.assertEqual([len(b) for b in banks], [audit['main_bytes'], audit['barracks_bytes']])
        for cue in audit['cues']:
            self.assertEqual(cue['offset'] % 32, 0)
            self.assertEqual(cue['bytes'] % 4, 0)
            bank = banks[cue['bank']]
            self.assertLessEqual(cue['offset'] + cue['bytes'], len(bank))
            clip = bank[cue['offset']:cue['offset'] + cue['bytes']]
            pcm = decode(clip)
            self.assertLessEqual(abs(len(pcm) - round(cue['retained_seconds'] * cue['rate'])), 7)
            self.assertLessEqual(max(abs(x) for x in pcm), PEAK_TARGET)
            self.assertGreater(sum(x*x for x in pcm) / len(pcm), 100000)
        intro = audit['cues'][0]
        self.assertAlmostEqual(intro['retained_seconds'], 357/30, places=3)
        self.assertLessEqual(len(banks[0]), 177000)
        self.assertLessEqual(len(banks[1]), 27048)

    def test_original_barracks_cue(self):
        encoded = (Path(__file__).resolve().parents[1] / 'generated/audio_barracks.adpcm').read_bytes()
        decoded = decode(encoded)
        self.assertLessEqual(abs(len(decoded) - round(8000 * 161 / 30)), 7)
        self.assertLessEqual(max(abs(x) for x in decoded), PEAK_TARGET)
        self.assertGreater(sum(x*x for x in decoded) / len(decoded), 1000000)

    def test_hardware_nibble_order_and_header(self):
        # Initial step=7: nibble 7 adds 11, index becomes 8 (step=16).
        # Next nibble 15 subtracts 30, giving -19.
        result = decode(struct.pack('<hBB', 0, 0, 0) + bytes([0xf7,0,0,0]))
        self.assertEqual(list(result[:2]), [11,-19])
        self.assertEqual(encode([0]*8), bytes(8))

    def test_alignment_and_signal_quality(self):
        samples = [int(16000*math.sin(i*2*math.pi*440/16000)) for i in range(16003)]
        encoded = encode(samples)
        decoded = decode(encoded)
        self.assertEqual(len(encoded)%4, 0)
        self.assertEqual(len(decoded), 16008)
        signal = sum(s*s for s in samples[100:])
        error = sum((s-d)**2 for s,d in zip(samples[100:],decoded[100:]))
        self.assertGreater(10*math.log10(signal/error), 25)

    def test_clipping_extremes(self):
        decoded = decode(encode([-32768]*100+[32767]*100))
        self.assertGreaterEqual(min(decoded), -32767)
        self.assertLessEqual(max(decoded), 32767)

    def test_normalization_handles_decoder_overshoot(self):
        # Floating point xWMA downmix can exceed int16. Normalize before casting,
        # and include ADPCM overshoot in the final output peak check.
        samples = [60000*math.sin(i*2*math.pi*731/16000) for i in range(16000)]
        encoded, decoded, gain = normalize_and_encode(samples)
        self.assertLess(gain, 1)
        self.assertLessEqual(max(abs(x) for x in decoded), PEAK_TARGET)
        self.assertEqual(len(encoded)%4, 0)
        self.assertFalse(any(abs(x)>=32767 for x in decoded))

    def test_normalization_raises_quiet_material_without_compression(self):
        samples = [1000*math.sin(i*2*math.pi*440/16000) for i in range(16000)]
        _, decoded, gain = normalize_and_encode(samples)
        self.assertGreater(gain, 10)
        self.assertGreater(max(abs(x) for x in decoded), 20000)
        error=sum((s*gain-d)**2 for s,d in zip(samples[100:],decoded[100:]))
        signal=sum((s*gain)**2 for s in samples[100:])
        self.assertGreater(10*math.log10(signal/error), 25)

    def test_speaker_compression_retains_headroom_and_duration(self):
        # Loud transients separated by quieter sustained sound model the source
        # effects that were peak normalized but still hard to hear on speakers.
        samples=[(18000 if i%4000<100 else 3000)*math.sin(i*2*math.pi*440/16000) for i in range(16000)]
        _,old,_=normalize_and_encode(samples)
        compressed=speaker_compress(samples,16000)
        _,new,_=normalize_and_encode(compressed)
        self.assertEqual(len(samples),len(compressed))
        self.assertEqual(len(old),len(new))
        self.assertLessEqual(max(map(abs,new)),PEAK_TARGET)
        self.assertGreater(sum(x*x for x in new),sum(x*x for x in old)*1.4)

    def test_silent_normalization(self):
        _, decoded, gain = normalize_and_encode([0]*16)
        self.assertEqual(gain, 1)
        self.assertEqual(list(decoded), [0]*16)

if __name__ == '__main__': unittest.main()
