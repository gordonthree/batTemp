# batTemp
Simple esp8266 based remote temperature probe, for battery life experiments.

Eagle Cad schematic includes a "generic" 78xx voltage regulator symbol, which is not used in this experiment. Input and output are bridged on the regulator.

Originally I tried using a 3 terminal boost converter, to get more runtime from 2x AA batteries. Leaving the converter powered up 24x7 quickly killed the batteries. Current experiment uses 3x AA NiMH cells (standard cells, not Eneloop).
