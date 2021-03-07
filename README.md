Implementation based on
http://practicalcryptography.com/cryptanalysis/text-characterisation/quadgrams/#a-python-implementation
https://planetcalc.com/8045/

English quadgram frequencies from:
http://practicalcryptography.com/cryptanalysis/letter-frequencies-various-languages/english-letter-frequencies/

- First implemented in Python (`solve.py`), got ~6,350 attempts/s
- Re-implemented in C++ (`main.cpp`) (representing ngram log-probs as `unordered_map<string, float>`), got ~39,700 attempts/s
- Represented n-grams as ints (`WXYZ -> W*26^3 + X*26^2 + Y*26 + Z`), got ~69,000 attempts/s
- Represented n-gram log-probs in a fixed array (of size `26^4`) got **~735,000** attempts/s
- Re-used plaintext string when decrypting, got **~950,000** attempts/s.
