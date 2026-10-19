"""Check DS EWRAM after all static assets, nine actor caches and packet buffers."""
from pathlib import Path
import json
import re
root = Path(__file__).resolve().parents[1]
link_map = (root / "build/castle-crashers-ds.map").read_text()
end = int(re.search(r"0x([0-9a-f]+)\s+__end__ =", link_map).group(1), 16)
limit = int(re.search(r"0x([0-9a-f]+)\s+__eheap_end =", link_map).group(1), 16)
remaining = limit - end
itcm_size = int(re.search(r"0x([0-9a-f]+)\s+__itcm_size =", link_map).group(1), 16)
assert itcm_size <= 32 * 1024, "ARM9 instruction TCM overflow"
# Installed libdswifi9_noip Wifi_InitDefault uses aligned_alloc(32, 0xa250).
# Round up to 48KiB and require another 48KiB for allocator/runtime overhead.
# This is a link-time budget check, not an on-device heap high-water measurement.
radio_reserve = 48 * 1024
runtime_reserve = 48 * 1024
report = {"ewram_end": hex(end), "ewram_limit": hex(limit),
          "available_before_runtime": remaining,
          "installed_wifi_allocation": 0xa250,
          "radio_reserve": radio_reserve, "additional_runtime_reserve": runtime_reserve,
          "remaining_after_reserves": remaining - radio_reserve - runtime_reserve,
          "characters_already_reserved": "two heroes, six enemies, one ally",
          "packet_bytes": 276, "instruction_tcm_bytes": itcm_size}
(root / "build/memory-budget.json").write_text(json.dumps(report, indent=2))
assert remaining >= radio_reserve + runtime_reserve, report
print(f"PASS: two-player memory budget: {remaining:,} bytes before runtime; "
      f"48KiB radio + 48KiB additional reserve; "
      f"{report['remaining_after_reserves']:,} bytes beyond reserves")
