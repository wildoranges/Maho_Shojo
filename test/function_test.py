#!/usr/bin/env python3
import subprocess
import os

IRBuild_ptn = '"{}" "-nocheck" "-emit-mir" "-o" "{}" "{}"'
codegen_ptn = '"clang" "-o" "{}" "{}" "../lib/libsysy_x86_64.a"'
exe_ptn = '"{}"'

def eval(EXE_PATH, TEST_BASE_PATH):
    print('===========TEST START===========')
    for case in testcases:
        print('Case %s:' % case, end='')
        TEST_PATH = TEST_BASE_PATH + case
        SY_PATH = TEST_BASE_PATH + case + '.sy'
        LL_PATH = TEST_BASE_PATH + case + '.ll'
        INPUT_PATH = TEST_BASE_PATH + case + '.in'
        OUTPUT_PATH = TEST_BASE_PATH + case + '.out'
        need_input = testcases[case]

        result = subprocess.run(IRBuild_ptn.format(EXE_PATH, LL_PATH, SY_PATH), shell=True, stderr=subprocess.PIPE)
        if result.returncode == 0:
            input_option = None
            if need_input:
                with open(INPUT_PATH, "rb") as fin:
                    input_option = fin.read()

            try:
                subprocess.run(codegen_ptn.format(TEST_PATH, LL_PATH), shell=True, stderr=subprocess.PIPE)
                result = subprocess.run(exe_ptn.format(TEST_PATH), shell=True, input=input_option, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=1)
                out = list()
                if result.stdout != b'':
                    out.append(result.stdout.split(b'\n'))
                if result.returncode != b'':
                    out.append(str(result.returncode).encode())
                with open(OUTPUT_PATH, "rb") as fout:
                    i = 0
                    for line in fout.readlines():
                        line = line.strip(b'\n')
                        if line == '':
                            continue
                        if out[i] == line:
                            print('\t\033[32mSuccess\033[0m')
                        else:
                            print(out[i], line)
                            print('\t\033[31mWrong Answer\033[0m')
                        i = i + 1
            except Exception as _:
                print('\t\033[31mCodeGen or Execute Fail\033[0m')
            finally:
                subprocess.call(["rm", "-rf", TEST_PATH, TEST_PATH])
                subprocess.call(["rm", "-rf", TEST_PATH, TEST_PATH + ".o"])
                subprocess.call(["rm", "-rf", TEST_PATH, TEST_PATH + ".ll"])

        else:
            print('\t\033[31mIRBuilder Fail\033[0m')

    print('============TEST END============')


if __name__ == "__main__":
    # { name: need_input }
    testcases = {}
    EXE_PATH = os.path.abspath('../build/compiler')
    # you should only revise this
    TEST_BASE_PATH = './function_test2020/'
    # you should only revise this
    testcase_list = list(map(lambda x: x.split('.'), os.listdir(TEST_BASE_PATH)))
    testcase_list.sort()
    for i in range(len(testcase_list)):
        testcases[testcase_list[i][0]] = False
    for i in range(len(testcase_list)):
        testcases[testcase_list[i][0]] = testcases[testcase_list[i][0]] | (testcase_list[i][1] == 'in')
    eval(EXE_PATH, TEST_BASE_PATH)
