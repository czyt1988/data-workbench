import numpy as np
import pandas as pd
if __name__ == '__main__':
    dtypelist = [np.dtype(np.bool_),
                 np.dtype(np.int8),
                 np.dtype(np.int16),
                 np.dtype(np.int32),
                 np.dtype(np.int64),
                 np.dtype(np.uint8),
                 np.dtype(np.uint16),
                 np.dtype(np.uint32),
                 np.dtype(np.uint64),
                 np.dtype(np.float16),
                 np.dtype(np.float32),
                 np.dtype(np.float64),
                 np.dtype(np.complex64),
                 np.dtype(np.complex128),
                 np.dtype(np.str_),
                 np.dtype(np.datetime64),
                 np.dtype(np.timedelta64),
                 np.dtype(np.bytes_),
                 np.dtype(np.void),
                 np.dtype(np.object_)]
    for v in dtypelist:
        print(
            f'name={v.name},kind={v.kind},char={v.char},str={v.str},num={v.num}')

    for v in dtypelist:
        print(
            f'''combox->addItem("{v.name}","{v.name}");''')