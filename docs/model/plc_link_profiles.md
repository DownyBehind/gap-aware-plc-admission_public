# PLC Link Profiles

Auxiliary synthetic profile table. The policy experiments draw only
default frame-size fields from it; effective airtimes and error rates
come from the channel attenuation map, and no headline number in the
paper is generated from this table alone.

|Class|Rule|C_req|C_res|C_slac_frame|B_frame|B_blk|B_pkt|PER req/res/slac|map loss|
|---|---|---|---|---|---|---|---|---|---|
|GOOD|distance <= 10 m or branch depth 0|12|18|11|21|21|21|0.001/0.001/0.002|0.001|
|NOMINAL|distance <= 25 m|15|21|14|21|21|21|0.005/0.005/0.010|0.002|
|DEGRADED|distance <= 45 m|22|32|18|32|42|32|0.020/0.020/0.030|0.005|
|SEVERE|distance > 45 m or branch depth >= 3|30|45|24|45|60|45|0.050/0.050/0.080|0.010|

These values are effective timing profiles, not exact HPGP PHY rates.
