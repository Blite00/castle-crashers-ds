"""Export the original PLAYER1680 buried timeline in all four knight colors."""
from export_pet_dig import prepare,export,COLORS
from concurrent.futures import ThreadPoolExecutor
if __name__=='__main__':
    jobs=[prepare(f'buried-common-{c}',f'assets/extracted/{c}.xml',[(1680,41)]) for c in COLORS]
    with ThreadPoolExecutor(max_workers=2) as pool:list(pool.map(export,jobs))
