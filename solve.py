# Mostly comes from:
# http://practicalcryptography.com/cryptanalysis/stochastic-searching/cryptanalysis-simple-substitution-cipher/
# And ideas from https://planetcalc.com/8045/
# Get ~6,350 attempts/s

import math
import random
import string
import time


class Fitness:

    def __init__(self, filename, floor_percentage=0.01, top_ngrams=826):
        with open(filename, 'r') as f:
            lines_split = (line.split(' ') for line in f.readlines())
            self.ngrams = {ngram: int(count) for ngram, count in lines_split}
            total = sum(self.ngrams.values())
            self.n = len(next(iter(self.ngrams.keys())))
            norm = math.log10(total)
            for ngram in self.ngrams:
                # Calculates the log-prob of p(ngram)
                # log (f/total) = log f - log total
                self.ngrams[ngram] = math.log10(self.ngrams[ngram]) - norm
            # `f_normal` from https://planetcalc.com/8045/
            top_ngram_probs = sorted(self.ngrams.values(),
                                     reverse=True)[:top_ngrams]
            self.normalized = sum(top_ngram_probs) / top_ngrams
            self.floor = math.log10(floor_percentage) - norm

    def score(self, text):
        fitness = 0
        for i in range(len(text) - self.n + 1):
            ngram = text[i:i+self.n]
            fitness += self.ngrams.get(ngram, self.floor)
        fitness /= i
        # Note: don't need to divide by `f_normal` since it's a constant.
        return abs(fitness - self.normalized)


def decrypt(ciphertext, key):
    return ''.join(string.ascii_uppercase[key.index(c)] for c in ciphertext)


fit = Fitness("english_quadgrams.txt")

ciphertext = """
SOWFBRKAWFCZFSBSCSBQITBKOWLBFXTBKOWLSOXSOXFZWWIBICFWUQLRXINOCIJLWJFQUNWXLFBSZXFBT
XAANTQIFBFSFQUFCZFSBSCSBIMWHWLNKAXBISWGSTOXLXTSWLUQLXJBUUWLWISTBKOWLSWGSTOXLXTSWL
BSJBUUWLFULQRTXWFXLTBKOWLBISOXSSOWTBKOWLXAKOXZWSBFIQSFBRKANSOWXAKOXZWSFOBUSWJBSBF
TQRKAWSWANECRZAWJ
""".replace("\n", "")

best_key = list(string.ascii_uppercase)
best_score = math.inf

parent_key, parent_score = best_key[:], best_score

start_time = time.time()
total_count = 0
while True:
    random.shuffle(parent_key)
    plaintext = decrypt(ciphertext, parent_key)
    parent_score = fit.score(plaintext)

    count = 0
    while count < 1000:
        if total_count > 0 and total_count % 50000 == 0:
            speed = total_count / (time.time() - start_time)
            print(f"[{speed:.2f} attempts/s]")
            print()

        i, j = random.randint(0, 25), random.randint(0, 25)
        child = parent_key[:]
        child[i], child[j] = child[j], child[i]
        plaintext = decrypt(ciphertext, child)
        score = fit.score(plaintext)
        if score < parent_score:
            parent_score = score
            parent_key = child[:]
            count = 0
        count += 1
        total_count += 1

    if parent_score < best_score:
        print(f"New best score!! {parent_score}")
        print(f"Plaintext: {decrypt(ciphertext, parent_key)}")
        best_print = ''.join(best_key)
        parent_print = ''.join(parent_key)
        print(f"Going from {best_print} to {parent_print}")
        print()
        best_score = parent_score
        best_key = parent_key[:]
