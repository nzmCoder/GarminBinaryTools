# Garmin Binary Tools
GarminBinary is a program to record and experiment with the Garmin binary protocol messages used in older Garmin GPS receivers. In pre-2005 Garmin receivers, before the "high sensitivity" models were introduced, there were undocumented asynchronous messages available that contained the raw satellite measurement data, including pseudo-range, carrier phase, and local clock counters. 

Prior work to document and use these "undocumented by Garmin" messages was done by William Soley, Jose Maria Munoz, Antonio Tabernero Galan, and others. Previous programs were available to record and use the raw satellite data, and included GRINGO from the University of Nottingham, ASYNC/GAR2RNX from Tabernero Galan, and GARBIN/G12RNX/G76RNX from Dennis Milbert. 

Unfortunately all these programs are now either unavailable, or not kept up to date. These programs (the ones that are still available) don't work well on modern computers. 

However there is still fun to be had. First, Garmin receivers that support this are still readily available on the used market, and they are cheap. A GPS 5 or eTrex can be purchased for less than $50. Second, the post-processing step has gotten a lot easier. The Canadian CSRS PPP service works with L1 only (single frequency) Rinex files, and will usually produce a 95% sigma result of accuracy in the sub-meter range. 

This program, used with GAR2RNX, which is included here, can produce Rinex observation files that can be sent to CSRS PPP for precise point position post processing. 


