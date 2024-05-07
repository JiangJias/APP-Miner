#!/usr/bin/env python

from sys import exit
from argparse import ArgumentParser

from conf import *
from util import *

from software       import SOFTWARE 

# main
def main():
    # init
    parser = ArgumentParser()
    parser.add_argument("-a", "--app", required=True)
    parser.add_argument("-i", "--instruction", default=None)


    subs = parser.add_subparsers(dest = "cmd")

    sub_checkout = subs.add_parser("checkout")
    sub_config = subs.add_parser("config")
    sub_build = subs.add_parser("build")

    sub_parse = subs.add_parser("parse")

    sub_irgen = subs.add_parser("irgen")
    sub_irgen.add_argument("-i", "--input", action="append", default=None) 
    sub_irgen.add_argument("-f", "--force", action="store_true")

    sub_group = subs.add_parser("group")

    sub_trans = subs.add_parser("trans")
    sub_trans.add_argument("-i", "--input", action="append", default=None)
    sub_trans.add_argument("-f", "--force", action="store_true")

    sub_check = subs.add_parser("check")
    sub_check.add_argument("-i", "--input", action="append", default=None) 
    sub_check.add_argument("-s", "--show", action="store_false")

    sub_stat = subs.add_parser("stat")
    sub_dump = subs.add_parser("dump")
    sub_result = subs.add_parser("result")
    sun_analyze = subs.add_parser("analyze")

    # parse
    args = parser.parse_args()
    APP = SOFTWARE(args.app, args.instruction)
    # exec
    if args.cmd == "checkout":
        APP.checkout()
    elif args.cmd == "config":
        APP.config()
    elif args.cmd == "build":
        APP.build()
    elif args.cmd == "parse":
        APP.parse()
    elif args.cmd == "irgen":
        APP.irgen(args.input, args.force)
    elif args.cmd == "group":
        APP.group()
    elif args.cmd == "trans":
        APP.trans(args.input, args.force)
    elif args.cmd == "check":
        APP.check(args.input, args.show)
    elif args.cmd == "dump":
        APP.dump()
    elif args.cmd == "stat":
        APP.stat()
    elif args.cmd == "result":
        APP.result()
    elif args.cmd == "analyze":
        APP.analyze()
    else:
        parser.print_usage()

# main
if __name__ == "__main__":
    with envpath("LD_LIBRARY_PATH", resolve(DEPS_Z3, "bins", "lib")):
        main()
