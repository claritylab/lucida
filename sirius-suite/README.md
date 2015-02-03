## Sirius-Suite

Johann Hauswald (jahausw@umich.edu)

Vinicius Petrucci (vpetrucci@gmail.com)

University of Michigan, 2014

Each kernel contains:

- input (when applicable, larger inputs not included)
- baseline
- pthread/phi
- GPU (when applicable)
- bits: this folder contains trial or old versions

Each version has its own `Makefile` inside the directory.

Each benchmark is run the following way:
```bash
$ ./executable ../input/input
```

Regex takes 2 inputs:
```bash
$ ./regex ../input/list ../input/questions
```

Download larger inputs at:

http://web.eecs.umich.edu/~jahausw

http://sirius.clarity-lab.org
