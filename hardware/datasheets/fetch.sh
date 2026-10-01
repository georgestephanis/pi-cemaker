#!/bin/sh
# Downloads datasheets used for the design. PDFs are not committed (third-party copyright).
# BQ25792 SLUSDG1C (TI's own URL needs a browser; this is a mirror - check the revision).
cd "$(dirname "$0")" || exit 1
curl -sL -A "Mozilla/5.0" -o bq25792.pdf "https://download.mikroe.com/documents/datasheets/BQ25792_datasheet.pdf"
pdftotext -layout bq25792.pdf bq25792.txt
# HUSB238 (Hynetek, via Adafruit) and LMR51450 (TI)
curl -sL -A "Mozilla/5.0" -o husb238.pdf "https://cdn-learn.adafruit.com/assets/assets/000/125/150/original/husb238_datasheet_full.pdf"
curl -sL -A "Mozilla/5.0" -o lmr51450.pdf "https://www.ti.com/lit/ds/symlink/lmr51450.pdf"
for n in husb238 lmr51450; do pdftotext -layout $n.pdf $n.txt; done
