# DavisSi1000 - Firmware for SiLabs Si1000 ISM radios for AUSTRALIAN Davis ISS weather protocol

This firmware is modified from: https://github.com/tridge/DavisSi1000
to use Australian frequency hops instead of US frequency hops

Frequencies were derived from: https://github.com/dekay/DavisRFM69
and converted as such:
freq = (msb << 16 + mid << 8 + lsb) * 61.03515625

For further information see: https://github.com/tridge/DavisSi1000

Confirmed working [this radio](https://www.amazon.com.au/Readytosky-Telemetry-915Mhz-pixhawk-controller/dp/B01DHV4DVA/ref=sr_1_fkmr0_1?ie=UTF8&qid=1528870379&sr=8-1-fkmr0&keywords=YKS+3DR+Radio+Telemetry+Kit+915Mhz+Module+Open+Source+for+APM+2.6+2.8+Pixhawk+RC+Quadcopter)

Not working with [this cheaper radio](https://www.ebay.com/p/3dr-Radio-Telemetry-Kit-915mhz-Module-Open-Source-for-Apm2-52-2-6-2-8-Pixhawk-US/509699912?iid=263577076048&chn=ps)