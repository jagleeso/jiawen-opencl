#!/usr/bin/env python
from jinja2 import Environment, FileSystemLoader
import argparse
import os
import sys

import config

def main():
    parser = argparse.ArgumentParser(description="render a file using jinja2")
    parser.add_argument('jinja_file')
    parser.add_argument('-o', '--output', required=False)
    parser.add_argument('-e', '--extension', default="out")

    args = parser.parse_args()

    outfile = None
    if args.output is not None:
        outfile = args.output
    else:
        fname, ext = os.path.splitext(args.jinja_file)
        if ext == '.jinja':
            # strip jinja extension
            outfile = fname
        else:
            # add .out extension
            outfile = fname + '.' + args.extension

    out = None
    if outfile == '-':
        out = sys.stdout
    else:
        out = open(outfile, 'w')

    env = Environment(loader=FileSystemLoader('./'))
    template = env.get_template(args.jinja_file)
    out.write(template.render(config=config))
    out.close()

if __name__ == '__main__':
    main()
