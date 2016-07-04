# Garmin Binary Tools
GarminBinary - Gar2Rnx - Async

GarminBinary is a program to view and record the Garmin binary protocol messages used in older Garmin GPS receivers. In certain pre-2005 Garmin receivers, before the "high sensitivity" models were introduced, there were undocumented asynchronous messages available that contained the raw satellite measurement data, including pseudo-range and carrier phase. 

Using these undocumented messages and this software, one can obtain sub-meter or decimeter point accuracy by recording data for 10 to 30 minutes at a stationary point and then post processing the data. Prior work to use these "undocumented by Garmin" messages was done by William Soley, Jose Maria Munoz, Antonio Tabernero Galan, and others. Previous programs were available to record and use the raw satellite data, and included GRINGO from the University of Nottingham, ASYNC/GAR2RNX from Tabernero Galan, and GARBIN/G12RNX/G76RNX from Dennis Milbert. 

Unfortunately these programs are now either unavailable, or not kept up to date. Of the programs still available, they are hard to use or have many communication errors on modern computers. 

However this idea still has value for any application where mid-range position accuracy is needed--better accuracy than a handheld GPS can provide (3 meters), but where a cm or mm accuracy is not required. Garmin receivers that support this are still readily available on the used market, and they are now cheap. A GPS V or eTrex that supports this can be purchased for less than $50. Also, the post-processing step has gotten a lot easier. The Canadian CSRS PPP service works with L1-only (single frequency) Rinex files, and will usually produce a 95% sigma result in the sub-meter range.   

GAR2RNX converts the garmin binary file into a Rinex observation file that can be sent to CSRS PPP for precise point position post processing. This program is an updated version that produces Rinex 2.11 format. 

ASYNC is an updated version of the original code. I have fixed some of the serial I/O problems, but this program is generally made obsolete by the new program, GarminBinary.
