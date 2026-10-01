#!/bin/sh
# Downloads datasheets used for the design. PDFs are not committed (third-party copyright).
# BQ25792 SLUSDG1C (TI's own URL needs a browser; this is a mirror - check the revision).
cd "$(dirname "$0")" || exit 1
curl -sL -A "Mozilla/5.0" -o bq25792.pdf "https://download.mikroe.com/documents/datasheets/BQ25792_datasheet.pdf"
pdftotext -layout bq25792.pdf bq25792.txt
