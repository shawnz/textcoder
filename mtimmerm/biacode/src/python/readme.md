# Bijective Arithmetic Coding in Python

## Overview
This is a Python port of the original C++ bijective arithmetic coding implementation by Matt Timmermans.

## Requirements
- Python 3.7+

## Usage
```bash
# Compress a file
python biacode.py input_file output_file

# Decompress a file
python biacode.py -d input_file output_file

# Specify block size (default is 1)
python biacode.py -b 4 input_file output_file
```

## Notes
- This is a learning implementation and may not be as performant as professional compression libraries
- The implementation follows the original C++ code's bijective arithmetic coding approach
- XOR obfuscation is retained from the original implementation

## License
Free for non-commercial purposes as long as this notice remains intact.
For commercial purposes, contact the original author: matt@timmermans.org
