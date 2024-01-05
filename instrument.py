#!/usr/bin/python

import os
import time
import sys, argparse
from typing import Literal

from random import seed, randint

from pycparser import c_parser, c_generator, c_ast
from pycparser.c_ast import FuncCall, ExprList

DUMMY = False
BITFLIP_COUNTER_NAME = '__vri_bfcntr'
COVERAGE_MAP_NAME = '__vri_covmap'

def generate_coverage_statement() -> FuncCall:
    rnd_u64 = randint(0, 1 << 64)

    name = c_ast.ID(name = f'{COVERAGE_MAP_NAME}.AddPoint')
    args = [c_ast.Constant(type = "unsigned long", value = hex(rnd_u64))]

    return c_ast.FuncCall(
        name,
        c_ast.ExprList(args),
    )

def generate_toggle_tracker(dst, rvalue) -> c_ast.ExprList:
    counter_inc = c_ast.Assignment(
        op = '+=',
        lvalue = c_ast.StructRef(
            name = c_ast.ID(name = BITFLIP_COUNTER_NAME),
            type = '.',
            field = c_ast.ID(name = 'value'),
        ),
        rvalue = c_ast.FuncCall(
            name = c_ast.ID(name = '__builtin_popcount'),
            args = ExprList([
                c_ast.BinaryOp(
                      op = '^',
                      left = dst,
                      right = rvalue,
                )
            ])
        )
    )

    return c_ast.ExprList([
        counter_inc,
        rvalue,
    ])

def instrument_if(if_statement, fields):
    if not isinstance(if_statement, c_ast.If):
        raise Exception(f'Invalid instrumentation call: {type(if_statement)}')
    
    iftrue = if_statement.iftrue
    iffalse = if_statement.iffalse

    if isinstance(iftrue, c_ast.If):
        instrument_if(iftrue, fields)
    elif isinstance(iftrue, c_ast.Compound):
        instrument_body(iftrue, fields)

    if_statement.iftrue = c_ast.Compound([generate_coverage_statement(), if_statement.iftrue])

    if iffalse != None:
        if isinstance(iffalse, c_ast.If):
            instrument_if(iffalse, fields)
        elif isinstance(iffalse, c_ast.Compound):
            instrument_body(iffalse, fields)

        if_statement.iffalse = c_ast.Compound([generate_coverage_statement(), if_statement.iffalse])
    else:
        if_statement.iffalse = c_ast.Compound([generate_coverage_statement()])

def instrument_expr(expr, fields):
    if isinstance(expr, c_ast.BinaryOp):
        instrument_expr(expr.left, fields)
        instrument_expr(expr.right, fields)

        if expr.op == '&&':
            expr.right = c_ast.ExprList([generate_coverage_statement(), expr.right])
        if expr.op == '||':
            expr.right = c_ast.ExprList([generate_coverage_statement(), expr.right])
    elif isinstance(expr, c_ast.TernaryOp):
        instrument_expr(expr.iftrue, fields)
        instrument_expr(expr.iffalse, fields)

        expr.iftrue = ExprList([generate_coverage_statement(), expr.iftrue])
        expr.iffalse = ExprList([generate_coverage_statement(), expr.iffalse])
    elif isinstance(expr, c_ast.UnaryOp):
        instrument_expr(expr.expr, fields)
    elif isinstance(expr, c_ast.ArrayRef):
        instrument_expr(expr.name, fields)
        instrument_expr(expr.subscript, fields)
    elif isinstance(expr, c_ast.ExprList):
        for expr in expr.exprs:
            instrument_expr(expr, fields)
    elif isinstance(expr, c_ast.Constant):
        return
    elif isinstance(expr, c_ast.StructRef):
        return
    elif isinstance(expr, c_ast.ID):
        return
    elif isinstance(expr, c_ast.FuncCall):
        for i in range(len(expr.args.exprs)):
            instrument_expr(expr.args.exprs[i], fields)
    else:
        generator = c_generator.CGenerator()
        string_repr = generator.visit(expr)
        raise Exception(f'Unsupported expr: {type(expr)}\n"""\n{string_repr}\n"""')

def instrument_assignment(assignment, fields):
    if not isinstance(assignment, c_ast.Assignment):
        raise Exception(f'Invalid instrumentation call: {type(assignment)}')
    
    instrument_expr(assignment.rvalue, fields)

    lvalue = assignment.lvalue
    while not isinstance(lvalue, c_ast.StructRef):
        if not isinstance(lvalue, c_ast.ArrayRef):
            return
        
        lvalue = lvalue.name

    if lvalue.name.name != "vlSelf":
        return

    if lvalue.field.name not in fields:
        return

    assignment.rvalue = generate_toggle_tracker(
        dst = assignment.lvalue,
        rvalue = assignment.rvalue,
    )

def instrument_body(body, fields):
    if not isinstance(body, c_ast.Compound):
        raise Exception(f'Invalid instrumentation call: {type(body)}')

    for block_item in body.block_items:
        if isinstance(block_item, c_ast.If):
            instrument_if(block_item, fields)
        elif isinstance(block_item, c_ast.Assignment):
            instrument_assignment(block_item, fields)
        elif isinstance(block_item, c_ast.FuncCall):
            instrument_expr(block_item, fields)
        else:
            raise Exception(f"Unsupported block item: {type(block_item)}")

def main():
    time_start = time.perf_counter()

    parser = argparse.ArgumentParser(
        prog = 'VerilatorRegisterInstrument',
        description = 'Instrument Verilator CPP code with register information',
    )

    parser.add_argument('dir')
    parser.add_argument('top_module')

    args = parser.parse_args()

    input_dir = args.dir
    top_module = args.top_module
    top_module = os.path.join(input_dir, top_module)

    if not(os.path.isdir(input_dir)):
        print(f"Unable to access '{input_dir}'. Does the directory exist")
        sys.exit(2)

    if not(os.path.isfile(top_module + '.mk')):
        print(f"Unable to access '{top_module + '.mk'}'. Does the top module exist")
        sys.exit(2)

    if os.path.isfile(os.path.join(input_dir, 'counting.hpp')):
        print('Already instrumented. Delete `counting.hpp` to redo instrumentation')
        sys.exit(1)

    os.popen(f'cp "./counting.cpp" "{input_dir}/"')
    os.popen(f'cp "./counting.hpp" "{input_dir}/"')
    os.popen(f'cp "./coverage.cpp" "{input_dir}/"')
    os.popen(f'cp "./coverage.hpp" "{input_dir}/"')

    header_files = []
    source_files = []

    for root, _, files in os.walk(input_dir):
        for file in files:
            _, ext = os.path.splitext(file)

            if ext == '.h' or ext == '.hpp':
                header_files.append(f'{root}{os.sep}{file}')

            if ext == '.cpp':
                source_files.append(f'{root}{os.sep}{file}')
    
    DATA_TYPES = [
        'CData',
        'SData',
        'IData',
        'QData',
    ]

    TEMPLATE_TYPES = [
        'VlUnpacked',
        'VlWide',
        'IData',
        'QData',
    ]

    instrumented = 0
    data_fields = dict()

    for file in header_files:
        fields = []

        with open(file, 'r') as f:
            lines = f.readlines()

            for i, line in enumerate(lines):
                stripped = line.strip()

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

                    # Now the stripped should contain the field
                    field = stripped
                    fields.append(field)

                for TEMPLATE_TYPE in TEMPLATE_TYPES:
                    if not stripped.startswith('VlUnpacked'):
                        continue

                    stripped = stripped[len(TEMPLATE_TYPE):]
                    stripped = stripped.lstrip()

                    assert stripped[0] == '<'
                    
                    depth = 0

                    for i, c in enumerate(stripped[1:]):
                        if c == '<':
                            depth += 1
                            continue
                        
                        if c == '>':
                            if depth == 0:
                                stripped = stripped[2 + i:]
                                stripped = stripped.lstrip()
                                stripped = stripped[:stripped.find(';')]
                                stripped = stripped.rstrip()

                                # Filter out internal data registers
                                if stripped.startswith('__'):
                                    continue

                                # Now the stripped should contain the field
                                field = stripped
                                fields.append(field)
                                break
                            else:
                                depth -= 1

                    assert depth == 0

        file_basename = os.path.basename(file)
        if file_basename[-2:] == ".h":
            data_fields[file_basename[:-2]] = fields
        elif file_basename[-4:] == ".hpp":
            data_fields[file_basename[:-4]] = fields
        else:
            print("Invalid header filename")
            exit(1)

    for file in source_files:
        bodies = []

        basename = os.path.basename(file)

        fields = None
        for key, dfields in data_fields.items():
            if not basename.startswith(key):
                continue
            fields = dfields
            break

        if fields != None:
            HEADER_INCLUDE = '#include "coverage.hpp"'

            with open(file, 'r') as f:
                lines = f.readlines()

                i = 0
                while i < len(lines):
                    line = lines[i]
                    i += 1

                    stripped = line.strip()

                    if line == HEADER_INCLUDE:
                        break

                    name = '_'

                    DETECT_LINE_STARTS = ['VL_INLINE_OPT void ', 'VL_ATTR_COLD void ', 'end']
                    for DETECT_LINE_START in DETECT_LINE_STARTS:
                        if not stripped.startswith(DETECT_LINE_START):
                            continue

                        line = line[len(DETECT_LINE_START):].lstrip()
                        name = line.split()[0] # Get until first whitespace

                    if name == '_':
                        continue

                    PROCESS_TYPES = ['___nba_', '___act_', '___stl_']
                    if not any(map(lambda x: x in name, PROCESS_TYPES)):
                        continue

                    BODY_START = "// Body"
                    BODY_END   = "}"

                    while lines[i].strip() != BODY_START:
                        i += 1

                    body_start = i + 1

                    while lines[i].rstrip() != BODY_END:
                        i += 1

                    body_end = i + 1
                    i += 1;

                    bodies.append((body_start, body_end))

            if len(bodies) > 0:
                # Insert the include of the `coverage_data` header
                # This is done just before the `#include "verilated.h"` line
                found = False
                for i, line in enumerate(lines):
                    if line.lstrip().startswith('#include'):
                        lines.insert(i, f'{HEADER_INCLUDE}\n#include "counting.hpp"\n')

                        found = True;
                        break;

                if not(found):
                    print("Was not able to find includes for source file. Exiting...")
                    sys.exit(1)

                with open(file, 'r' if DUMMY else 'w') as f:
                    prev_body_end = 0

                    for body_start, body_end in bodies:
                        if not DUMMY:
                            f.writelines(lines[prev_body_end:body_start + 1])

                        body = lines[body_start + 1:body_end]
                        body = ''.join(body)

                        fndecl = f'void x() {{ {body} }}'

                        parser = c_parser.CParser()
                        try:
                            ast = parser.parse(fndecl)
                        except Exception as error:
                            print("Failed to parse AST")
                            print(error)
                            print()
                            print(fndecl)
                            exit(1)

                        instrument_body(ast.ext[0].body, fields)

                        generator = c_generator.CGenerator()
                        generator.visit(ast.ext[0].body)
                        if not DUMMY:
                            f.write(generator.visit(ast.ext[0].body))
                        else:
                            generator.visit(ast.ext[0].body)

                        prev_body_end = body_end

                    if not DUMMY:
                        f.writelines(lines[prev_body_end:])

                print(f"Instrumented {file}")
                instrumented += 1
                continue

        print(f"Skipping     {file}")


    VM_USER_CLASSES_LINE = None
    with open(top_module + '.mk', 'r') as f:
        lines = f.readlines()

        for i, line in enumerate(lines):
            line = line.lstrip()
            if line.startswith('VM_USER_CLASSES = '):
                VM_USER_CLASSES_LINE = i
                break

        if VM_USER_CLASSES_LINE == None:
            print('Failed to find VM_USER_CLASSES line')
            exit(1)

    with open(top_module + '.mk', 'r' if DUMMY else 'w') as f:
        lines.insert(VM_USER_CLASSES_LINE + 1, '\tcounting \\\n\tcoverage \\\n');

        f.writelines(lines)

    time_end = time.perf_counter()
    time_diff = time_end - time_start
    print("")
    print(f"Instrumented {instrumented}/{len(header_files) + len(source_files)} files")
    print(f"Completed in {time_diff:0.4f} seconds")

if __name__ == "__main__":
    main()