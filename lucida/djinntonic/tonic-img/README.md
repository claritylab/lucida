### Tonic-IMG
Features:
- IMGService tweaked to still use the infer function from djinn. Currently the
  `../djinn/DjinnTestClient.cpp` is the driver for the create and learn
  functions. Some thought needs to be put into how the IMGService will be
  adapted.

Testing:
- use `./bin --help` to see default arguments. Running the defaults works.
- make sure the djinn service is running:
  - ./DjinnService`
- run the tonic-img service:
  - `./IMGService --svip localhost --port 8080 --djinnip localhost --djinnport 5000`
- make an inference request: `./testClient`. Returns inference for 3 IMC images.
