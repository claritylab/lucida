def iterfirst(iterator, count=1):
    """Iterate through 'count' first values."""
    iterator = iter(iterator)
    for i in range(count):
        yield iterator.next()

def iterstep(iterator, n):
    """Iterate every nth value."""
    iterator = iter(iterator)
    while True:
        yield iterator.next()
        for dummy in range(n-1):
            iterator.next()

def itergroup(iterator, count, default=None):
    """Iterate in groups of 'count' values. If there
    aren't enough values, the last result is padded with
    the default."""
    iterator = iter(iterator)
    while True:
        moreValues = False
        values = []
        for x in range(count):
            try:
                values.append(iterator.next())
                moreValues = True
            except StopIteration:
                 values.append(default)
        if not moreValues:
            raise StopIteration
        yield tuple(values)
    
def itercat(*iterators):
    """Concatenate several iterators into one."""
    iterators = map(iter, iterators)
    for i in iterators:
        for x in i:
            yield x

def iterzip(*iterators):
    """Iterative version of builtin 'zip'."""
    iterators = map(iter, iterators)
    while True:
        yield tuple([x.next() for x in iterators])

def itermap(func, *iterators):
    """Iterative version of builtin 'map'."""
    iterators = map(iter, iterators)
    while True:
        moreValues = False
        args = []
        for i, iterator in enumerate(iterators):
            if iterator is None:
                args.append(None)
            else:
                try:
                    args.append(iterator.next())
                    moreValues = True
                except StopIteration:
                    iterators[i] = None
                    args.append(None)
        if not moreValues:
            raise StopIteration
        yield func is None and tuple(args) or func(*args)

def iterfilter(func, iterator):
    """Iterative version of builtin 'filter'."""
    iterator = iter(iterator)
    while True:
        next = iterator.next()
        if func is None and next or func(next):
            yield next

def iterfile(f, retainNewlines=False):
    """Returns an iterator to the lines of the file.

    f may be a file name or a file-like object with a readline method.
    retainNewlines must evaluate to a boolean. If it evaluates to False, the
      newline characters at the end of each line will be removed, otherwise,
      they will be retained.
    """

    # get the file object
    if type(f) == file:
        fileObject = f
    elif type(f) == str:
        fileObject = file(f)
    else:
        raise TypeError, "f must be a file or fileName"

    # iterate through the lines of the file...
    line = fileObject.readline()
    while line:
        if not retainNewlines and line.endswith('\n'):
            yield line[:-1]
        else:
            yield line
        line = fileObject.readline()
