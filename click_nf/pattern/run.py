#!/usr/bin/python3
import argparse
import os


def getargs():
    parser = argparse.ArgumentParser()
    parser.add_argument('file',  help="click router pattern file")
    parser.add_argument("-i", "--id", type=int, help="instance id for NF")
    parser.add_argument('-d', '--debug', action='store_true',
                        help='output result file for debugging')
    parser.add_argument('nargs', nargs='+', help="pattern variables")
    args = parser.parse_args()
    return args


if __name__ == "__main__":
    args = getargs()
    f = open(args.file)
    s = f.read()

    if "id" in args:
        s = s.replace('$id', str(args.id))

    i = 1
    for arg in args.nargs:
        variable = '$' + str(i)
        s = s.replace(variable, arg)
        i += 1
    cmd = "click -e \"" + s + '\"'

    if args.debug:
        print(cmd)

    os.system(cmd)
    # result = os.popen(cmd).read()
    # print(result)
