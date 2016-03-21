### DjNN Service
Functions included:
- create: allows creation of an intelligent instance to allow training on
  custom data for DNN based applications from DjiNN. Currently only supports
  facial recognition images. Concretely receives `list<pair<image, label> >`
  for training.
- learn: spins up an instance of Caffe to train. Currently many parameters
  hardcoded, needs upgrade to finetuning as opposed to training from scratch to
  keep training time manageable.
- infer: make inference requests to prior trained model. Currently swaps out
  dummy model for a pretrained model.

Testing:
- `./DjinnTestClient` sends a list of training images (superfake-face-db.txt)
  to DjiNN to train a facial recognition network.
