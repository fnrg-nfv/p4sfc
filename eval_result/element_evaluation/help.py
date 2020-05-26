#!/usr/bin/python3
import sys


def is_number(s):
    try:
        float(s)
        return True
    except ValueError:
        return False


if __name__ == "__main__":
    if len(sys.argv) == 2:
        filename = sys.argv[1]
        f = open(filename, "r")
        lines = f.readlines()
        for line in lines:
            line = line.replace(';', ' ')
            p = ""
            for s in line.split():
                if is_number(s):
                    p += s + "\t"
            if p:
              print(p)
