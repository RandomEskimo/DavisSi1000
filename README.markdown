# DavisSi1000 - Firmware for SiLabs Si1000 ISM radios for AUSTRALIAN Davis ISS weather protocol

This firmware is modified from: https://github.com/tridge/DavisSi1000
to use Australian frequency hops instead of US frequency hops

Frequencies were derived from: https://github.com/dekay/DavisRFM69
and converted as such:
freq = (msb << 16 + mid << 8 + lsb) * 61.03515625

For further information see: https://github.com/tridge/DavisSi1000
