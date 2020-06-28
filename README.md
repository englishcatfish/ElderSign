# Elder Sign
Simulate and calculate success probabilities for the game Elder Sign.  The program can take as input the number of focuses, spells, and clues that are available for use. It will then produce the maximum probability of success when using an optimal strategy for different dice set scenarios.

Scenarios:
1. 5 Green Dice
2. 6 Green Dice
3. 6 Green Dice + Yellow Die
4. 6 Green Dice + Red Die
5. 6 Green Dice + Yellow Die + Red Die
6. 6 Green Dice + Yellow Die + Red Die + White Die

## Compile
Compile with your favorite c++ compiler.

E.g., `clang++-10 ElderSign.cpp -pthread -O3 -o eldersign`

## Run

`./eldersign`

```
Flags:
        --focus <n>             Number of focus events available (default = 0)
        --spell <n>             Number of spells available (default = 0)
        --clue <n>              Number of clues available (default = 0)
        --in-file <cards_file>  Default is cards.csv
        --num-threads <n>       Specify number of cores you want to use.
                                This will greatly improve performance (default = 1).
        --task <task>           Overrides input file, can specify one-off inputs.
        --disabled-saved        Use if you don't wish to use/update the on disk cache
        --clear-saved           Use to reset the on disk cache
        --large-mem             Use when clues > 0 to help cache additional calculations. Uses a lot of memory.
        --no-range              Program iterates all 0..focus and 0..spell combinations. This turns that off.
        --scenarios <n>         Perform calculations for the first n scenarios.
        --help                  This print out.
```

The following example is equivalent to default ./eldersign

`./eldersign --focus 0 --spell 0 --clue 0 --in-file cards.csv --num-threads 1`

A few examples for running one-off tasks

`./eldersign --focus 1 --spell 1 --task PERIL,PERIL\>SCROLL,SCROLL --num-threads 4 --disable-saved`

`./eldersign --clue 1 --task TERROR\|PERIL,SCROLL,SCROLL --num-threads 4 --disable-saved --large-mem`

Note: Running with `clue > 0` will take a **very long time** the more dice used, so it's best not to run the last two scenarios.  Use the following command to impose a limit.

```./eldersign --clue 1 --task TERROR\|PERIL,SCROLL,SCROLL --num-threads 4 --disable-saved --large-mem --scenarios 4```

Output will look like

```
Card Title#Tasks#Focus#Spell#Clue#5G#6G#6G+Y#6G+R#6G+YR#6G+YRW
OneOff#PERIL,PERIL>SCROLL,SCROLL#0 Focus#0 Spell#0 Clue#2.252997#7.413286#17.246428#25.475103#42.755001#60.696999
OneOff#PERIL,PERIL>SCROLL,SCROLL#0 Focus#1 Spell#0 Clue#6.331679#18.872911#37.635907#50.038759#70.198097#84.727274
OneOff#PERIL,PERIL>SCROLL,SCROLL#1 Focus#0 Spell#0 Clue#3.772202#14.146246#31.917385#43.825856#65.468503#81.710824
OneOff#PERIL,PERIL>SCROLL,SCROLL#1 Focus#1 Spell#0 Clue#7.564600#25.074904#49.635519#62.693259#82.712372#93.588553
```

Copy into Excel and split text into columns using `#` as the delimiter.
