#!/usr/bin/python

import os
import time
import sys, argparse

def main():
    time_start = time.perf_counter()

    parser = argparse.ArgumentParser(
        prog = 'VerilatorRegisterInstrument',
        description = 'Instrument Verilator CPP code with register information',
    )

    parser.add_argument('dir')

    args = parser.parse_args()

    input_dir = args.dir

    if not(os.path.isdir(input_dir)):
        print(f"Unable to access '{input_dir}'. Does the directory exist")
        sys.exit(2)

    header_files = []

    for root, dirs, files in os.walk(input_dir):
        path = root.split(os.sep)
        for file in files:
            _, ext = os.path.splitext(file)

            if file == 'counting.h':
                continue
            
            if ext == '.h':
                header_files.append(f'{root}{os.sep}{file}')
    
    DATA_TYPES = [
        'CData',
        'SData',
        'IData',
        'QData',
    ]

    instrumented = 0
    for file in header_files:
        with open(file, 'r') as f:
            updated = False
            lines = f.readlines()

            for i, line in enumerate(lines):
                stripped = line.strip()
                lwhitespace_len = len(line) - len(line.lstrip())

                # Find the lines with registers
                for DATA_TYPE in DATA_TYPES:
                    if not(stripped.startswith(DATA_TYPE)):
                        continue

                    stripped = stripped[len(DATA_TYPE):]
                    stripped = stripped.lstrip()

                    if stripped.startswith('/*'):
                        stripped = stripped[stripped.find('*/', 2)+2:]
                        stripped = stripped.lstrip()

                    stripped = stripped[:stripped.find(';')]
                    stripped = stripped.rstrip()

                    # Filter out internal data registers
                    if stripped.startswith('__'):
                        continue

                    # Change their type from XData to CountingXData
                    lines[i] = f"{line[:lwhitespace_len]}Counting{line[lwhitespace_len:]}"
                    updated = True

        if updated:
            # Insert the include of the `counting_data` header
            # This is done just before the `#include "verilated.h"` line
            found = False
            for i, line in enumerate(lines):
                if line.strip() == '#include "verilated.h"':
                    lines.insert(i, f'#include "counting.hpp"\n')

                    found = True;
                    break;

            if not(found):
                print("Was not able to find includes for module file. Exiting...")
                sys.exit(1)
            
            with open(file, 'w') as f:
                f.writelines(lines)

            print(f"Instrumented {file}")
            instrumented += 1
        else:
            print(f"Skipping     {file}")

    time_end = time.perf_counter()
    time_diff = time_end - time_start
    print("")
    print(f"Instrumented {instrumented}/{len(header_files)} header files")
    print(f"Completed in {time_diff:0.4f} seconds")

if __name__ == "__main__":
    main()