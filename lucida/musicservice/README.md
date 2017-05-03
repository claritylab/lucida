# Music Service (MS)
This is a mood-indexed music scanning service.

## Major Dependencies
- [pygn python library](https://github.com/cweichen/pygn)
- [Gracenote Music API](https://www.gracenote.com/)

## Build
- get API key for [Gracenote Music API](https://developer.gracenote.com/user/me/apps)
- place client ID and key in `MusicConfig.py`
```
make all
```

## Run
To run the server
```
make start_server
```

## Test
To test if the server is correct
```
make start_test
```

## Clean
To clean the running environment
```
make clean
```
