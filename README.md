# wordle solver (written in C++)

## how to build

```
sudo apt install cmake g++
./run.sh
```

## play example

auto play using solver of random select in default dictionary (2315 words)

```
$ out/release/wordle
ruder [..o..]  20
biddy [..o?o]   6
oddly [.?ooo]   2
madly [.oooo]   1
sadly [ooooo]   1

$ out/release/wordle
meaty [.....] 264
cliff [o.?..]   6
choir [o?.?.]   1
cinch [ooooo]   1
```

auto play using solver of max entropy select in default dictionary (2315 words)

```
$ out/release/wordle --solver entropy
raise [.o?..]  14
cleft [?....]   3
pound [o..?.]   1
panic [ooooo]   1

$ out/release/wordle --solver entropy
raise [.?..?]  69
cleat [.??o.]   6
depot [.o?..]   1
penal [ooooo]   1
```

auto play using solver of max entropy select in additional dictionary (2315 + 12972 words)
```
$ out/release/wordle --solver entropy --use-hard
soare [....?] 120
teind [.o...]  13
bylaw [o.o..]   1
belch [ooooo]   1

$ out/release/wordle --solver entropy --use-hard
soare [...?.]  57
glint [?....]   1
rugby [ooooo]   1
```

auto play with specified answer
```
$ out/release/wordle --solver entropy --use-hard --answer sheep
soare [o...?]  35
clipt [...?.]   4
tweed [..oo.]   1
sheep [ooooo]   1
```

show entropy
```
$ out/release/wordle --solver entropy --use-hard --answer sheep --log-level 2
easy.txt : 2315 words
hard.txt : 12972 words
 5.885962 [bits] soare [o...?]  35
 4.272140 [bits] clipt [...?.]   4
 2.000000 [bits] tweed [..oo.]   1
sheep [ooooo]   1
```

interactive play
```
$ out/release/wordle --solver interactive --use-hard
INPUT: raise
raise [.o..?]  20
INPUT: panel
panel [.o.o.]   5
INPUT: camel
camel [oo.o.]   2
INPUT: /help
cagey cadet
INPUT: cagey
cagey [ooooo]   1
```

## License

MIT
