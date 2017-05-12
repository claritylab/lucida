# Weather

## Major Dependencies

- [Weather Underground API](https://www.wunderground.com/weather/api/)
- [Open Weather Map API](https://openweathermap.org/api)

# Structure

- `client/`: implementation of test client
- `server/`: implementation of server which call weather APIs

## Build

- get API keys for [Weather Underground API](https://www.wunderground.com/weather/api/) and [Open Weather Map API](https://openweathermap.org/api)
- place keys in `WeatherConfig.py`

```
make all
```

## Run

```
make start_server
```

## Test

```
make start_test
```

## Clean

```
make clean
```
