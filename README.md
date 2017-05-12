# SyncPerf
SyncPerf detects and categorizes synchronization related performance issues. It is a lightweight profiler. SyncPerf reports existing synchronization related performance bugs in a program along with their categories. 
### EuroSys'17 Paper : http://dl.acm.org/citation.cfm?id=3064186

## Build
- `cd src`
- run `make`

## Run
To use SyncPerf, link your executable with `src\libsyncperf.so` 
or set the `LD_PRELOAD` environment variable:
`export LD_PRELOAD=\path\to\syncperflib`
