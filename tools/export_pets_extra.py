"""Export the remaining ANIMALS pets (2, 9 and 21) the same way export_pet_dig.py exports 17 and 24."""
from export_pet_dig import prepare, export
import concurrent.futures
if __name__ == '__main__':
    jobs = [prepare(f'pet-common-{pet}', 'assets/extracted/pet-common-animals.xml', [(51, 14), (60, 49)], pet) for pet in (2, 9, 21)]
    with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
        list(pool.map(export, jobs))
