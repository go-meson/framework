#!/usr/bin/env python

import os
import sys

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))

def main():
    print SOURCE_ROOT

if __name__ == '__main__':
    sys.exit(main())
