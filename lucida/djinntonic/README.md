# Djinn and Tonic

## Major Dependencies

- [Caffe](http://caffe.berkeleyvision.org/)
- [Facebook Thrift](https://github.com/facebook/fbthrift)

# Structure

- `dig/`: implementation of the digit recognition service
- `face/`: implementation of the facial recognition service
- `imc/`: implementation of the image classification service
- `models/`: DNN models necessary for the above services
- `tools/`: dependencies necessary for Djinn and Tonic

## Build

```
make
```

## Run

```
cd dig # or face, or imc
make start_server
```

## Test

```
cd dig # or face, or imc
make start_test
```
